/*
 *   File:   main.c
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "iopins.h"
#include "config.h"
#include "i2c.h"
#include "timer.h"
#include "counters.h"
#include "usart.h"
#include "cmd.h"
#include "util.h"

#define NO_READING_AVAILABLE        0x7FFFFFF
#define SENSOR_STATE_ONLINE         0
#define SENSOR_STATE_ERROR          1
#define SENSOR_STATE_OFFLINE        2

#define T_TOGGLE_BUTTON             0x01
#define T_BUTTON_STATUS             0x02
#define T_BUTTON_UPDATE             0x04
#define T_GPIO_STATUS               0x08

typedef struct
{
    sys_config_t *config;
} sys_runstate_t;

sys_config_t _g_cfg;
sys_runstate_t _g_rs;
sys_counters_t _g_counters;

FILE uart_str = FDEV_SETUP_STREAM(print_char, NULL, _FDEV_SETUP_RW);

static void io_init(void);
static void clock_init(void);

int main(void)
{
    sys_runstate_t *rs = &_g_rs;
    sys_config_t *config = &_g_cfg;

    memset((void *)&_g_counters, 0x00, sizeof(sys_counters_t));

    rs->config = config;

    clock_init();
    io_init();
    timer_tcb0_init();
    g_irq_enable();

    usart0_open(USART_CONT_RX, USART_BAUD_RATE(UART0_BAUD)); // Console

    stdout = &uart_str;

    printf("\r\nStarting up...\r\n");

    // Idle loop
    for (;;)
    {
        cmd_process(config);
    }
}

static void clock_init(void)
{
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0 << CLKCTRL_PEN_bp); // Disable prescaler
}

static void io_init(void)
{
    IO_INPUT(LD);
    IO_OUTPUT(LE);
    IO_OUTPUT(DATA);
    IO_OUTPUT(CLOCK);

    IO_LOW(LE);
    IO_LOW(CLOCK);
    IO_LOW(DATA);
}

int print_char(char byte, FILE *stream)
{
    while (console_busy());
    console_put(byte);
    return 0;
}