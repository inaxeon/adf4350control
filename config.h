/* 
 *   File:   config.h
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

typedef struct {
    uint16_t magic;
    uint64_t freq;
    uint16_t r_value;
    uint8_t power;
    bool out_on;
} sys_config_t;

void load_configuration(sys_config_t *config);
void default_configuration(sys_config_t *config);
void save_configuration(sys_config_t *config);

#endif /* __CONFIG_H__ */
