/*
 *   File:   timer.c
 *   Author: Matt
 *
 *   Created on 19 May 2018, 18:02
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
#include <avr/io.h>
#include <avr/interrupt.h>

#include "timer.h"
#include "counters.h"

void timer_tcb0_init(void)
{
    
    //TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | _BV(TCB_ENABLE_bp);
    TCB0.CTRLA = TCB_CLKSEL_DIV1_gc | _BV(TCB_ENABLE_bp);
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.INTCTRL = _BV(TCB_CAPT_bp);
    //TCB0.CCMP = 20135; // Every 1ms. 20Mhz / 1000 with fudge factor
    TCB0.CCMP = 20000; // Every 1ms. 20Mhz / 1000
}

ISR(TCB0_INT_vect)
{
    _g_counters.tick_count++;
    TCB0.INTFLAGS = _BV(TCB_CAPT_bp);
}
