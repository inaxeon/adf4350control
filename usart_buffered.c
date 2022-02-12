/*
 *   File:   usart_buffered.c
 *   Author: Matt
 *
 *   Created on 12 Feb 2021, 20:29
 *
 *   This is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *   This software is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   You should have received a copy of the GNU General Public License
 *   along with this software.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "project.h"

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "usart.h"
#include "iopins.h"

#define UART_BUFFER_OVERFLOW  0x02

#define UART_TX_BUFFER_SIZE 64
#define UART_RX_BUFFER_SIZE 64

/* size of RX/TX buffers */
#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1)
#define UART_TX_BUFFER_MASK (UART_TX_BUFFER_SIZE - 1)

#if (UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK)
#error RX buffer size is not a power of 2
#endif
#if (UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK)
#error TX buffer size is not a power of 2
#endif

#ifdef _USART0_

static volatile uint8_t _g_usart0_txbuf[UART_TX_BUFFER_SIZE];
static volatile uint8_t _g_usart0_rxbuf[UART_RX_BUFFER_SIZE];
static volatile uint8_t _g_usart0_txhead;
static volatile uint8_t _g_usart0_txtail;
static volatile uint8_t _g_usart0_rxhead;
static volatile uint8_t _g_usart0_rxtail;
static volatile uint8_t _g_usart0_last_rx_error;

#endif /* _USART0_ */

#ifdef _USART1_

static volatile uint8_t _g_usart1_txbuf[UART_TX_BUFFER_SIZE];
static volatile uint8_t _g_usart1_rxbuf[UART_RX_BUFFER_SIZE];
static volatile uint8_t _g_usart1_txhead;
static volatile uint8_t _g_usart1_txtail;
static volatile uint8_t _g_usart1_rxhead;
static volatile uint8_t _g_usart1_rxtail;
static volatile uint8_t _g_usart1_last_rx_error;

#endif /* _USART1_ */

#ifdef _USART0_

ISR(USART0_RXC_vect)
{
    uint8_t tmphead;
    uint8_t data;
    uint8_t usr;
    uint8_t lastRxError;
 
    usr  = USART0.RXDATAH;
    data = USART0.RXDATAL;
    
    lastRxError = (usr & (_BV(USART_BUFOVF_bp) | _BV(USART_FERR_bp)));
    tmphead = (_g_usart0_rxhead + 1) & UART_RX_BUFFER_MASK;
    
    if (tmphead == _g_usart0_rxtail)
    {
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }
    else
    {
        _g_usart0_rxhead = tmphead;
        _g_usart0_rxbuf[tmphead] = data;
    }

    _g_usart0_last_rx_error = lastRxError;   
}

ISR(USART0_DRE_vect)
{
    uint8_t tmptail;
    
    if (_g_usart0_txhead != _g_usart0_txtail)
    {
        tmptail = (_g_usart0_txtail + 1) & UART_TX_BUFFER_MASK;
        _g_usart0_txtail = tmptail;
        USART0.TXDATAL = _g_usart0_txbuf[tmptail];
    }
    else
    {
        USART0.CTRLA &= ~_BV(USART_DREIE_bp);
    }
}

void usart0_open(uint8_t flags, uint16_t brg)
{
    uint8_t ctrla = 0;
    uint8_t ctrlb = 0;
    uint8_t ctrlc = 0;

    _g_usart0_txhead = 0;
    _g_usart0_txtail = 0;
    _g_usart0_rxhead = 0;
    _g_usart0_rxtail = 0;

	USART0.BAUD = brg;

    ctrlb = _BV(USART_TXEN_bp);
    ctrla = _BV(USART_RXCIE_bp);

    if (flags & USART_SYNC)
    {
        ctrlc |= USART_CMODE_SYNCHRONOUS_gc;

        if (flags & USART_SYNC_MASTER)
            USART1_DDR |= _BV(USART1_XCK);
        else
            USART1_DDR &= ~_BV(USART1_XCK);
    }
    else
    {
        ctrlc |= USART_CMODE_ASYNCHRONOUS_gc;
    }

    if (flags & USART_9BIT)
        ctrlc |= USART_CHSIZE_9BITL_gc;
    else
        ctrlc |= USART_CHSIZE_8BIT_gc;

    if (flags & USART_CONT_RX)
        ctrlb |= _BV(USART_RXEN_bp);
    else
        ctrlb &= ~_BV(USART_RXEN_bp);

    USART0.CTRLA = ctrla;
    USART0.CTRLB = ctrlb;
    USART0.CTRLC = ctrlc;

    USART0_DDR |= _BV(USART0_TX);
    USART0_DDR &= ~_BV(USART0_RX);
}

bool usart0_data_ready(void)
{
    if (_g_usart0_rxhead == _g_usart0_rxtail)
        return false;
    return true;
}

char usart0_get(void)
{
    uint8_t tmptail;

    if (_g_usart0_rxhead == _g_usart0_rxtail)
        return 0x00;
    
    tmptail = (_g_usart0_rxtail + 1) & UART_RX_BUFFER_MASK;
    _g_usart0_rxtail = tmptail;
    
    return _g_usart0_rxbuf[tmptail];
}

void usart0_put(char c)
{
    uint8_t tmphead = (_g_usart0_txhead + 1) & UART_TX_BUFFER_MASK;
    
    while (tmphead == _g_usart0_txtail);
    
    _g_usart0_txbuf[tmphead] = c;
    _g_usart0_txhead = tmphead;

    USART0.CTRLA |= _BV(USART_DREIE_bp);
}

bool usart0_busy(void)
{
    return (_g_usart0_txhead != _g_usart0_txtail || (USART0.STATUS & _BV(USART_DREIF_bp)) == 0);
}

uint8_t usart0_get_last_rx_error(void)
{
    return _g_usart0_last_rx_error;
}

#endif /* _USART0_ */

#ifdef _USART1_

ISR(USART1_RXC_vect)
{
    uint8_t tmphead;
    uint8_t data;
    uint8_t usr;
    uint8_t lastRxError;
 
    usr  = USART1.RXDATAH;
    data = USART1.RXDATAL;
    
    lastRxError = (usr & (_BV(USART_BUFOVF_bp) | _BV(USART_FERR_bp)));
    tmphead = (_g_usart1_rxhead + 1) & UART_RX_BUFFER_MASK;
    
    if (tmphead == _g_usart1_rxtail)
    {
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }
    else
    {
        _g_usart1_rxhead = tmphead;
        _g_usart1_rxbuf[tmphead] = data;
    }

    _g_usart1_last_rx_error = lastRxError;   
}

ISR(USART1_DRE_vect)
{
    uint8_t tmptail;
    
    if (_g_usart1_txhead != _g_usart1_txtail)
    {
        tmptail = (_g_usart1_txtail + 1) & UART_TX_BUFFER_MASK;
        _g_usart1_txtail = tmptail;
        USART1.TXDATAL = _g_usart1_txbuf[tmptail];
    }
    else
    {
        USART1.CTRLA &= ~_BV(USART_DREIE_bp);
    }
}

void usart1_open(uint8_t flags, uint16_t brg)
{
    uint8_t ctrla = 0;
    uint8_t ctrlb = 0;
    uint8_t ctrlc = 0;

    _g_usart1_txhead = 0;
    _g_usart1_txtail = 0;
    _g_usart1_rxhead = 0;
    _g_usart1_rxtail = 0;

	USART1.BAUD = brg;

    ctrlb = _BV(USART_TXEN_bp);
    ctrla = _BV(USART_RXCIE_bp);

    if (flags & USART_SYNC)
    {
        ctrlc |= USART_CMODE_SYNCHRONOUS_gc;

        if (flags & USART_SYNC_MASTER)
            USART1_DDR |= _BV(USART1_XCK);
        else
            USART1_DDR &= ~_BV(USART1_XCK);
    }
    else
    {
        ctrlc |= USART_CMODE_ASYNCHRONOUS_gc;
    }

    if (flags & USART_9BIT)
        ctrlc |= USART_CHSIZE_9BITL_gc;
    else
        ctrlc |= USART_CHSIZE_8BIT_gc;

    if (flags & USART_CONT_RX)
        ctrlb |= _BV(USART_RXEN_bp);
    else
        ctrlb &= ~_BV(USART_RXEN_bp);

    USART1.CTRLA = ctrla;
    USART1.CTRLB = ctrlb;
    USART1.CTRLC = ctrlc;

    USART1_DDR |= _BV(USART1_TX);
    USART1_DDR &= ~_BV(USART1_RX);
}

bool usart1_data_ready(void)
{
    if (_g_usart1_rxhead == _g_usart1_rxtail)
        return false;
    return true;
}

char usart1_get(void)
{
    uint8_t tmptail;

    if (_g_usart1_rxhead == _g_usart1_rxtail)
        return 0x00;
    
    tmptail = (_g_usart1_rxtail + 1) & UART_RX_BUFFER_MASK;
    _g_usart1_rxtail = tmptail;
    
    return _g_usart1_rxbuf[tmptail];
}

void usart1_put(char c)
{
    uint8_t tmphead = (_g_usart1_txhead + 1) & UART_TX_BUFFER_MASK;
    
    while (tmphead == _g_usart1_txtail);
    
    _g_usart1_txbuf[tmphead] = c;
    _g_usart1_txhead = tmphead;

    USART1.CTRLA |= _BV(USART_DREIE_bp);
}

bool usart1_busy(void)
{
    return (_g_usart1_txhead != _g_usart1_txtail || (USART1.STATUS & _BV(USART_DREIF_bp)) == 0);
}

uint8_t usart1_get_last_rx_error(void)
{
    return _g_usart1_last_rx_error;
}

#endif /* _USART1_ */
