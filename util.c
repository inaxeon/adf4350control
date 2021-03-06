/*
 *   File:   util.c
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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h> 
#include <avr/pgmspace.h>

#include "util.h"
#include "usart.h"
#include "config.h"

void reset(void)
{
    /* Uses the watch dog timer to reset */
    //g_irq_disable();
    //WDT.CTRLA = WDT_PERIOD_8CLK_gc;
    //while (1);

    _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
}

void putch(char byte)
{
    while (console_busy());
    console_put(byte);
}

char wdt_getch(void)
{
    while (!console_data_ready())
        CLRWDT();
    return console_get();
}

void format_fixedpoint(char *buf, int16_t value, uint8_t type)
{
    char sign[2];

    sign[1] = 0;

    if ((type == I_1DP || type == I_2DP) && value < 0)
        sign[0] = '-';
    else
        sign[0] = 0;

    if (type == I_1DP || type == U_1DP)
        sprintf(buf, "%s%u.%u", sign, abs(value) / _1DP_BASE, abs(value) % _1DP_BASE);
    if (type == I_2DP || type == U_2DP)
        sprintf(buf, "%s%u.%02u", sign, abs(value) / _2DP_BASE, abs(value) % _2DP_BASE);
}

void eeprom_write_data(uint8_t addr, uint8_t *bytes, uint8_t len)
{
    uint16_t dest = addr;
    eeprom_update_block(bytes, (void *)dest, len);
}

void eeprom_read_data(uint8_t addr, uint8_t *bytes, uint8_t len)
{
    uint16_t dest = addr;
    eeprom_read_block(bytes, (void *)dest, len);
}
