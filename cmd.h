/*
 *   File:   cmd.h
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

#include <stdint.h>

#ifndef __CMD_H__
#define __CMD_H__

#define CONSOLE_1               0x00

#define CMD_MAX_LINE            64

#define SO_SEND_UPDATE          0x01
#define SO_MASK_IRQ             0x04
#define SO_UNMASK_IRQ           0x08

void cmd_process(sys_config_t *config);
void cmd_init(void);
bool command_prompt_handler(char *text, sys_config_t *config);
bool set_output(bool state, uint8_t flags);
#ifdef _HAVE_SWITCH_ON_SCK_
bool set_gpio_output(bool state, uint8_t flags);
#endif /* _HAVE_SWITCH_ON_SCK_ */
void dump_state(void);
void set_suspend(bool suspended);

void do_freq(sys_config_t *config);

#endif /* __CMD_H__ */