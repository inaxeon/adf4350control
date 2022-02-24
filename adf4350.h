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

/* Registers */
#define ADF4350_REG0                            0
#define ADF4350_REG1                            1
#define ADF4350_REG2                            2
#define ADF4350_REG3                            3
#define ADF4350_REG4                            4
#define ADF4350_REG5                            5

/* REG0 Bit Definitions */
#define ADF4350_REG0_FRACT(x)                   ((((uint32_t)x) & 0xFFF) << 3)
#define ADF4350_REG0_INT(x)                     ((((uint32_t)x) & 0xFFFF) << 15)

/* REG1 Bit Definitions */
#define ADF4350_REG1_MOD(x)                     (((x) & 0xFFF) << 3)
#define ADF4350_REG1_PHASE(x)                   (((x) & 0xFFF) << 15)
#define ADF4350_REG1_PRESCALER                  (1UL << 27)

/* REG2 Bit Definitions */
#define ADF4350_REG2_COUNTER_RESET_EN           (1UL << 3)
#define ADF4350_REG2_CP_THREESTATE_EN           (1UL << 4)
#define ADF4350_REG2_POWER_DOWN_EN              (1UL << 5)
#define ADF4350_REG2_PD_POLARITY_POS            (1UL << 6)
#define ADF4350_REG2_LDP_6ns                    (1UL << 7)
#define ADF4350_REG2_LDP_10ns                   (0UL << 7)
#define ADF4350_REG2_LDF_FRACT_N                (0UL << 8)
#define ADF4350_REG2_LDF_INT_N                  (1UL << 8)
#define ADF4350_REG2_CHARGE_PUMP_CURR_uA(x)     (((((x)-312) / 312) & 0xF) << 9)
#define ADF4350_REG2_DOUBLE_BUFF_EN             (1UL << 13)
#define ADF4350_REG2_10BIT_R_CNT(x)             ((x) << 14)
#define ADF4350_REG2_RDIV2_EN                   (1UL << 24)
#define ADF4350_REG2_RMULT2_EN                  (1UL << 25)
#define ADF4350_REG2_MUXOUT(x)                  ((x) << 26)
#define ADF4350_REG2_NOISE_MODE(x)              (((uint32_t)(x)) << 29)
#define ADF4350_MUXOUT_THREESTATE               0
#define ADF4350_MUXOUT_DVDD                     1
#define ADF4350_MUXOUT_GND                      2
#define ADF4350_MUXOUT_R_DIV_OUT                3
#define ADF4350_MUXOUT_N_DIV_OUT                4
#define ADF4350_MUXOUT_ANALOG_LOCK_DETECT       5
#define ADF4350_MUXOUT_DIGITAL_LOCK_DETECT      6

/* REG3 Bit Definitions */
#define ADF4350_REG3_12BIT_CLKDIV(x)            ((x) << 3)
#define ADF4350_REG3_12BIT_CLKDIV_MODE(x)       ((x) << 16)
#define ADF4350_REG3_12BIT_CSR_EN               (1UL << 18)
#define ADF4351_REG3_CHARGE_CANCELLATION_EN     (1UL << 21)
#define ADF4351_REG3_ANTI_BACKLASH_3ns_EN       (1UL << 22)
#define ADF4351_REG3_BAND_SEL_CLOCK_MODE_HIGH   (1UL << 23)

/* REG4 Bit Definitions */
#define ADF4350_REG4_OUTPUT_PWR(x)              ((x) << 3)
#define ADF4350_REG4_RF_OUT_EN                  (1UL << 5)
#define ADF4350_REG4_AUX_OUTPUT_PWR(x)          ((x) << 6)
#define ADF4350_REG4_AUX_OUTPUT_EN              (1UL << 8)
#define ADF4350_REG4_AUX_OUTPUT_FUND            (1UL << 9)
#define ADF4350_REG4_AUX_OUTPUT_DIV             (0UL << 9)
#define ADF4350_REG4_MUTE_TILL_LOCK_EN          (1UL << 10)
#define ADF4350_REG4_VCO_PWRDOWN_EN             (1UL << 11)
#define ADF4350_REG4_8BIT_BAND_SEL_CLKDIV(x)    ((x) << 12)
#define ADF4350_REG4_RF_DIV_SEL(x)              ((x) << 20)
#define ADF4350_REG4_FEEDBACK_DIVIDED           (0UL << 23)
#define ADF4350_REG4_FEEDBACK_FUND              (1UL << 23)

/* REG5 Bit Definitions */
#define ADF4350_REG5_LD_PIN_MODE_LOW            (0UL << 22)
#define ADF4350_REG5_LD_PIN_MODE_DIGITAL        (1UL << 22)
#define ADF4350_REG5_LD_PIN_MODE_HIGH           (3UL << 22)

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
    uint64_t pfd;
    uint16_t r_cnt;
    uint16_t intv;
    uint16_t fract;
    uint16_t mod;
    uint16_t rf_div;
    uint64_t actual_freq;
    bool prescaler;
    uint16_t band_sel_div;
    uint32_t regs[6];
} adf4350_calculated_parameters_t;

bool adf4350_set_freq(uint64_t freq, adf4350_platform_data_t *settings, adf4350_calculated_parameters_t *params);

#endif /* __ADF4350_H__ */