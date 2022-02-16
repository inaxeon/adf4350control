/*
 *   File:   adf4350.h
 *   Author: Matt
 *
 *   Created on 12 Feb 2021, 20:29
 *
 *   ADF4350 Driver. Mostly stolen from the Linux Kernel.
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

#ifndef __ADF4350_H__
#define __ADF4350_H__

/**
 * struct adf4350_platform_data - platform specific information
 * @clkin:              REFin frequency in Hz.
 * @channel_spacing:    Channel spacing in Hz (influences MODULUS).
 * @max_r_value:            Optional, if set the driver skips dynamic calculation
 *                          and uses this default value instead.
 * @ref_doubler_en:     Enables reference doubler.
 * @ref_div2_en:        Enables reference divider.
 * @r2_user_settings:   User defined settings for ADF4350/1 REGISTER_2.
 * @r3_user_settings:   User defined settings for ADF4350/1 REGISTER_3.
 * @r4_user_settings:   User defined settings for ADF4350/1 REGISTER_4.
 */

typedef struct
{
    uint32_t        clkin;
    uint32_t        channel_spacing;
    uint16_t        max_r_value; /* 10-bit R counter */
    bool            ref_doubler_en;
    bool            ref_div2_en;
	uint32_t        r2_user_settings;
	uint32_t        r3_user_settings;
	uint32_t        r4_user_settings;
} adf4350_platform_data_t;

typedef struct
{
    uint64_t vco;
    double pfd;
    uint16_t r_cnt;
    uint16_t intv;
    uint16_t fract;
    uint16_t rf_div;
    double actual_freq;
    bool prescaler;
    uint16_t band_sel_div;
    uint32_t regs[6];
} adf4350_calculated_parameters_t;

bool adf4350_set_freq(uint64_t freq, adf4350_platform_data_t *settings, adf4350_calculated_parameters_t *params);

#endif /* __ADF4350_H__ */