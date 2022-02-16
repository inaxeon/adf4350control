/*
 *   File:   adf4350.c
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

#include "project.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "adf4350.h"
#include "iopins.h"
#include "util.h"

/* Registers */
#define ADF4350_REG0                            0
#define ADF4350_REG1                            1
#define ADF4350_REG2                            2
#define ADF4350_REG3                            3
#define ADF4350_REG4                            4
#define ADF4350_REG5                            5

/* REG0 Bit Definitions */
#define ADF4350_REG0_FRACT(x)                   (((x) & 0xFFF) << 3)
#define ADF4350_REG0_INT(x)                     (((x) & 0xFFFF) << 15)

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

/* Specifications */
#define ADF4350_MAX_OUT_FREQ                    4400000000ULL /* Hz */
#define ADF4350_MIN_OUT_FREQ                    137500000 /* Hz */
#define ADF4351_MIN_OUT_FREQ                    34375000 /* Hz */
#define ADF4350_MIN_VCO_FREQ                    2200000000ULL /* Hz */
#define ADF4350_MAX_FREQ_45_PRESC               3000000000ULL /* Hz */
#define ADF4350_MAX_FREQ_PFD                    32000000 /* Hz */
#define ADF4350_MAX_BANDSEL_CLK                 125000 /* Hz */
#define ADF4350_MAX_FREQ_REFIN                  250000000 /* Hz */
#define ADF4350_MAX_MODULUS                     4095
#define ADF4350_MAX_R_CNT                       1023

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

typedef struct
{
    adf4350_platform_data_t *pdata;
    uint32_t                clkin;
    uint32_t                chspc; /* Channel Spacing */
    double                  fpfd;  /* Phase Frequency Detector */
    uint16_t                r0_fract;
    uint16_t                r0_int;
    uint16_t                r1_mod;
    uint32_t                r4_rf_div_sel;
} adf4350_state_t;

static uint32_t adf_4350_gcd(uint32_t a, uint32_t b);
static uint32_t adf_4350_do_div(uint64_t *n, uint32_t base);
static int adf4350_tune_r_cnt(adf4350_state_t *st, uint16_t r_cnt);

bool adf4350_set_freq(uint64_t freq, adf4350_platform_data_t *settings, adf4350_calculated_parameters_t *params)
{
    adf4350_state_t st;
    uint32_t chspc;
    uint64_t tmp;
    uint32_t div_gcd;
    uint32_t prescaler;
    uint16_t mdiv;
    uint16_t r_cnt = 0;
    uint8_t band_sel_div;
    uint32_t regs[6];

    memset(&st, 0x00, sizeof(adf4350_state_t));

    st.pdata = settings;
    st.clkin = settings->clkin;
    chspc = settings->channel_spacing;

    if (freq > ADF4350_MAX_OUT_FREQ || freq < ADF4350_MIN_OUT_FREQ)
        return false;

    if (freq > ADF4350_MAX_FREQ_45_PRESC) {
        prescaler = ADF4350_REG1_PRESCALER;
        mdiv = 75;
    }
    else {
        prescaler = 0;
        mdiv = 23;
    }

    while (freq < ADF4350_MIN_VCO_FREQ) {
        freq <<= 1;
        st.r4_rf_div_sel++;
    }

	do {
		do {
			do {
				r_cnt = adf4350_tune_r_cnt(&st, r_cnt);
				st.r1_mod = st.fpfd / chspc;
				if (r_cnt > ADF4350_MAX_R_CNT) {
					/* try higher spacing values */
					chspc++;
					r_cnt = 0;
				}
			} while ((st.r1_mod > ADF4350_MAX_MODULUS) && r_cnt &&
				(settings->max_r_value == 0 || r_cnt < settings->max_r_value));
		} while (r_cnt == 0);

		tmp = freq * (uint64_t)st.r1_mod + ((uint32_t)st.fpfd >> 1);
		adf_4350_do_div(&tmp, st.fpfd); /* Div round closest (n + d/2)/d */
		st.r0_fract = adf_4350_do_div(&tmp, st.r1_mod);
		st.r0_int = tmp;
	} while (mdiv > st.r0_int);

    band_sel_div = DIV_ROUND_UP(st.fpfd, ADF4350_MAX_BANDSEL_CLK);

    if (st.r0_fract && st.r1_mod) {
        div_gcd = adf_4350_gcd(st.r1_mod, st.r0_fract);
        st.r1_mod /= div_gcd;
        st.r0_fract /= div_gcd;
    }
    else {
        st.r0_fract = 0;
        st.r1_mod = 1;
    }

    regs[ADF4350_REG0] = ADF4350_REG0_INT(st.r0_int) |
        ADF4350_REG0_FRACT(st.r0_fract);

    regs[ADF4350_REG1] = ADF4350_REG1_PHASE(1) |
        ADF4350_REG1_MOD(st.r1_mod) |
        prescaler;

    regs[ADF4350_REG2] =
        ADF4350_REG2_10BIT_R_CNT(r_cnt) |
        /*ADF4350_REG2_DOUBLE_BUFF_EN |*/
        (settings->ref_doubler_en ? ADF4350_REG2_RMULT2_EN : 0) |
        (settings->ref_div2_en ? ADF4350_REG2_RDIV2_EN : 0) |
        (settings->r2_user_settings & (ADF4350_REG2_PD_POLARITY_POS |
            ADF4350_REG2_LDP_6ns | ADF4350_REG2_LDF_INT_N |
            ADF4350_REG2_CHARGE_PUMP_CURR_uA(5000) |
            ADF4350_REG2_MUXOUT(0x7UL) | ADF4350_REG2_NOISE_MODE(0x3UL)));

    regs[ADF4350_REG3] = settings->r3_user_settings &
        (ADF4350_REG3_12BIT_CLKDIV(0xFFF) |
            ADF4350_REG3_12BIT_CLKDIV_MODE(0x3UL) |
            ADF4350_REG3_12BIT_CSR_EN |
            ADF4351_REG3_CHARGE_CANCELLATION_EN |
            ADF4351_REG3_ANTI_BACKLASH_3ns_EN |
            ADF4351_REG3_BAND_SEL_CLOCK_MODE_HIGH);

    regs[ADF4350_REG4] =
        ADF4350_REG4_FEEDBACK_FUND |
        ADF4350_REG4_RF_DIV_SEL(st.r4_rf_div_sel) |
        ADF4350_REG4_8BIT_BAND_SEL_CLKDIV(band_sel_div) |
        ADF4350_REG4_RF_OUT_EN |
        (settings->r4_user_settings &
        (ADF4350_REG4_OUTPUT_PWR(0x3) |
            ADF4350_REG4_AUX_OUTPUT_PWR(0x3) |
            ADF4350_REG4_AUX_OUTPUT_EN |
            ADF4350_REG4_AUX_OUTPUT_FUND |
            ADF4350_REG4_MUTE_TILL_LOCK_EN));

    regs[ADF4350_REG5] = ADF4350_REG5_LD_PIN_MODE_DIGITAL | 0x180000 /* Reserved bits */;

    if (params)
    {
        params->vco = freq;
        params->pfd = st.fpfd;
        params->r_cnt = r_cnt;
        params->intv = st.r0_int;
        params->fract = st.r0_fract;
        params->rf_div = (1 << st.r4_rf_div_sel);
        params->actual_freq = (((double)st.r0_int + ((double)st.r0_fract / (double)st.r1_mod)) * st.fpfd / (1 << st.r4_rf_div_sel)) / 1000000;
        params->prescaler = prescaler;
        params->band_sel_div = band_sel_div;

        for (int i = 0; i < 6; i++)
            params->regs[i] = regs[i];
    }

    return true;
}

static int adf4350_tune_r_cnt(adf4350_state_t *st, uint16_t r_cnt)
{
	adf4350_platform_data_t *pdata = st->pdata;

    do {
        r_cnt++;
        st->fpfd = (st->clkin * (pdata->ref_doubler_en ? 2 : 1)) / ((double)r_cnt * (pdata->ref_div2_en ? 2 : 1));
    } while (st->fpfd > ADF4350_MAX_FREQ_PFD);

    return r_cnt;
}

static uint32_t adf_4350_do_div(uint64_t *n, uint32_t base)
{
    uint32_t remainder = *n % base;
    *n = *n / base;
    return remainder;
}

static uint32_t adf_4350_gcd(uint32_t a, uint32_t b)
{
    uint32_t r = a | b;

    if (!a || !b)
        return r;

    /* Isolate lsbit of r */
    r &= -((long)r);

    while (!(b & r))
        b >>= 1;
    if (b == r)
        return r;

    for (;;) {
        while (!(a & r))
            a >>= 1;
        if (a == r)
            return r;
        if (a == b)
            return a;

        if (a < b)
        {
            uint32_t tmp = a;
            a = b;
            b = tmp;
        }
        a -= b;
        a >>= 1;
        if (a & r)
            a += b;
        a >>= 1;
    }
}