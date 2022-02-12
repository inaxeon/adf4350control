/*
 *   File:   project.h
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


#ifndef __PROJECT_H__
#define __PROJECT_H__

#define _I2C_XFER_

#define CONFIG_MAGIC        0x4144

#define CLRWDT() asm("wdr")

#define g_irq_disable cli
#define g_irq_enable sei

#define UART0_BAUD           9600

#define _USART0_

#define console_busy         usart0_busy
#define console_put          usart0_put
#define console_data_ready   usart0_data_ready
#define console_get          usart0_get
#define console_clear_oerr   usart0_clear_oerr

#endif /* __PROJECT_H__ */