/* 
 *   File:   counters.h
 *   Author: Matt
 *
 *   Created on 29 October 2021, 09:02
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

#ifndef __COUNTERS_H__
#define	__COUNTERS_H__

typedef struct
{
    int32_t tick_count;
    volatile uint16_t i2c_timeout_cnt;
    uint16_t i2c_timeouts;
    volatile uint16_t xbee_timeout_cnt;
    uint16_t xbee_timeouts;
} sys_counters_t;

extern sys_counters_t _g_counters;

#endif	/* __COUNTERS_H__ */

