/*
 *   File:   iopins.h
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


#ifndef __IOPINS_H__
#define __IOPINS_H__

#define IO_INPUT(pin) pin##_DDR &= ~_BV(pin)
#define IO_OUTPUT(pin) pin##_DDR |= _BV(pin)

#define IO_HIGH(pin) pin##_PORT |= _BV(pin)
#define IO_LOW(pin) (pin##_PORT) &= ~_BV(pin)

#define IO_TOGGLE(pin) (pin##_PORT) ^= _BV(pin)

#define IO_IN_HIGH(pin) ((pin##_PIN & _BV(pin)) == _BV(pin))
#define IO_IN_LOW(pin) ((pin##_PIN & _BV(pin)) == 0x00)

#define IO_OUT_HIGH(pin) ((pin##_PORT & _BV(pin)) == _BV(pin))
#define IO_OUT_LOW(pin) ((pin##_PORT & _BV(pin)) == 0x00)

#define LD                  PORT4
#define CLOCK               PORT5
#define DATA                PORT6
#define LE                  PORT7

#define LD_PIN              PORTA.IN
#define CLOCK_PIN           PORTA.IN
#define DATA_PIN            PORTA.IN
#define LE_PIN              PORTA.IN

#define LE_PORT             PORTA.OUT
#define CLOCK_PORT          PORTA.OUT
#define DATA_PORT           PORTA.OUT
#define LE_PORT             PORTA.OUT

#define LD_DDR              PORTA.DIR
#define CLOCK_DDR           PORTA.DIR
#define DATA_DDR            PORTA.DIR
#define LE_DDR              PORTA.DIR

#define SDA_PINCTRL         PORTB.PIN1CTRL
#define SDA_DDR             PORTB.OUT
#define SDA_PIN             PORTB.IN
#define SDA_PORT            PORTB.OUT
#define SDA                 PORT1

#define SCL_PINCTRL         PORTB.PIN0CTRL
#define SCL_DDR             PORTB.OUT
#define SCL_PIN             PORTB.IN
#define SCL_PORT            PORTB.OUT
#define SCL                 PORT0

#define USART0_DDR          PORTB.DIR
#define USART0_TX           PORT2
#define USART0_RX           PORT3
#define USART0_XCK          PORT1

#define USART1_DDR          PORTA.DIR
#define USART1_TX           PORT1
#define USART1_RX           PORT2
#define USART1_XCK          PORT3

#endif /* __IOPINS_H__ */