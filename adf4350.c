/*
 *   File:   adf4350.c
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
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "adf4350.h"
#include "iopins.h"
#include "util.h"

/* Registers */
#define ADF4350_REG0    0
#define ADF4350_REG1    1
#define ADF4350_REG2    2
#define ADF4350_REG3    3
#define ADF4350_REG4    4
#define ADF4350_REG5    5

/* REG0 Bit Definitions */
#define ADF4350_REG0_FRACT(x)            (((x) & 0xFFF) << 3)
#define ADF4350_REG0_INT(x)            (((x) & 0xFFFF) << 15)

/* REG1 Bit Definitions */
#define ADF4350_REG1_MOD(x)            (((x) & 0xFFF) << 3)
#define ADF4350_REG1_PHASE(x)            (((x) & 0xFFF) << 15)
#define ADF4350_REG1_PRESCALER            (1UL << 27)

/* REG2 Bit Definitions */
#define ADF4350_REG2_COUNTER_RESET_EN        (1UL << 3)
#define ADF4350_REG2_CP_THREESTATE_EN        (1UL << 4)
#define ADF4350_REG2_POWER_DOWN_EN        (1UL << 5)
#define ADF4350_REG2_PD_POLARITY_POS        (1UL << 6)
#define ADF4350_REG2_LDP_6ns            (1UL << 7)
#define ADF4350_REG2_LDP_10ns            (0UL << 7)
#define ADF4350_REG2_LDF_FRACT_N        (0UL << 8)
#define ADF4350_REG2_LDF_INT_N            (1UL << 8)
#define ADF4350_REG2_CHARGE_PUMP_CURR_uA(x)    (((((x)-312) / 312) & 0xF) << 9)
#define ADF4350_REG2_DOUBLE_BUFF_EN        (1UL << 13)
#define ADF4350_REG2_10BIT_R_CNT(x)        ((x) << 14)
#define ADF4350_REG2_RDIV2_EN            (1UL << 24)
#define ADF4350_REG2_RMULT2_EN            (1UL << 25)
#define ADF4350_REG2_MUXOUT(x)            ((x##UL) << 26)
#define ADF4350_REG2_NOISE_MODE(x)        (((unsigned)(x##UL)) << 29)
#define ADF4350_MUXOUT_THREESTATE        0
#define ADF4350_MUXOUT_DVDD            1
#define ADF4350_MUXOUT_GND            2
#define ADF4350_MUXOUT_R_DIV_OUT        3
#define ADF4350_MUXOUT_N_DIV_OUT        4
#define ADF4350_MUXOUT_ANALOG_LOCK_DETECT    5
#define ADF4350_MUXOUT_DIGITAL_LOCK_DETECT    6

/* REG3 Bit Definitions */
#define ADF4350_REG3_12BIT_CLKDIV(x)        ((x##UL) << 3)
#define ADF4350_REG3_12BIT_CLKDIV_MODE(x)    ((x##UL) << 16)
#define ADF4350_REG3_12BIT_CSR_EN        (1 << 18)
#define ADF4351_REG3_CHARGE_CANCELLATION_EN    (1 << 21)
#define ADF4351_REG3_ANTI_BACKLASH_3ns_EN    (1 << 22)
#define ADF4351_REG3_BAND_SEL_CLOCK_MODE_HIGH    (1 << 23)

/* REG4 Bit Definitions */
#define ADF4350_REG4_OUTPUT_PWR(x)        ((x) << 3)
#define ADF4350_REG4_RF_OUT_EN            (1 << 5)
#define ADF4350_REG4_AUX_OUTPUT_PWR(x)        ((x) << 6)
#define ADF4350_REG4_AUX_OUTPUT_EN        (1 << 8)
#define ADF4350_REG4_AUX_OUTPUT_FUND        (1 << 9)
#define ADF4350_REG4_AUX_OUTPUT_DIV        (0 << 9)
#define ADF4350_REG4_MUTE_TILL_LOCK_EN        (1 << 10)
#define ADF4350_REG4_VCO_PWRDOWN_EN        (1 << 11)
#define ADF4350_REG4_8BIT_BAND_SEL_CLKDIV(x)    ((x) << 12)
#define ADF4350_REG4_RF_DIV_SEL(x)        ((x) << 20)
#define ADF4350_REG4_FEEDBACK_DIVIDED        (0 << 23)
#define ADF4350_REG4_FEEDBACK_FUND        (1 << 23)

/* REG5 Bit Definitions */
#define ADF4350_REG5_LD_PIN_MODE_LOW        (0 << 22)
#define ADF4350_REG5_LD_PIN_MODE_DIGITAL    (1 << 22)
#define ADF4350_REG5_LD_PIN_MODE_HIGH        (3 << 22)

/* Specifications */
#define ADF4350_MAX_OUT_FREQ        4400000000ULL /* Hz */
#define ADF4350_MIN_OUT_FREQ        137500000 /* Hz */
#define ADF4351_MIN_OUT_FREQ        34375000 /* Hz */
#define ADF4350_MIN_VCO_FREQ        2200000000ULL /* Hz */
#define ADF4350_MAX_FREQ_45_PRESC    3000000000ULL /* Hz */
#define ADF4350_MAX_FREQ_PFD        32000000 /* Hz */
#define ADF4350_MAX_BANDSEL_CLK        125000 /* Hz */
#define ADF4350_MAX_FREQ_REFIN        250000000 /* Hz */
#define ADF4350_MAX_MODULUS        4095
#define ADF4350_MAX_R_CNT        1023

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

typedef struct {
    adf4350_platform_data_t pdata;
    uint32_t       clkin;
    uint32_t       chspc; /* Channel Spacing */
    double         fpfd;  /* Phase Frequency Detector */
    uint32_t       min_out_freq;
    unsigned       r0_fract;
    unsigned       r0_int;
    unsigned       r1_mod;
    unsigned       r4_rf_div_sel;
    uint32_t       regs[6];
} adf4350_state_t;

static uint32_t adf_4350_gcd(uint32_t a, uint32_t b);
static uint32_t adf_4350_do_div(uint64_t *n, uint32_t base);
static int adf4350_tune_r_cnt(adf4350_state_t *st, uint16_t r_cnt);

static adf4350_state_t _g_state;

void adf4350_init(adf4350_platform_data_t *settings)
{
	adf4350_state_t *st = &_g_state;

	memset(&_g_state, 0x00, sizeof(adf4350_state_t));
	memcpy(&_g_state.pdata, settings, sizeof(adf4350_platform_data_t));

	st->clkin = settings->clkin;
	st->chspc = settings->channel_spacing;
	st->min_out_freq = ADF4350_MIN_OUT_FREQ;

	adf4350_set_freq(439852000);
	//adf4350_set_freq(st, 4001000000);

	printf("R0: 0x%08X\r\n", st->regs[0] + 0);
	printf("R1: 0x%08X\r\n", st->regs[1] + 1);
	printf("R2: 0x%08X\r\n", st->regs[2] + 2);
	printf("R3: 0x%08X\r\n", st->regs[3] + 3);
	printf("R4: 0x%08X\r\n", st->regs[4] + 4);
	printf("R5: 0x%08X\r\n", st->regs[5] + 5);
}


bool adf4350_set_freq(uint64_t freq)
{
    adf4350_state_t *st = &_g_state;
    adf4350_platform_data_t *pdata = &st->pdata;
    uint64_t tmp;
    double actual_freq;
    uint64_t in_freq = freq;
    uint32_t div_gcd, prescaler, chspc;
    uint16_t mdiv, r_cnt = 0;
    uint8_t band_sel_div;

    if (freq > ADF4350_MAX_OUT_FREQ || freq < st->min_out_freq)
        return false;

    if (freq > ADF4350_MAX_FREQ_45_PRESC) {
        prescaler = ADF4350_REG1_PRESCALER;
        mdiv = 75;
    }
    else {
        prescaler = 0;
        mdiv = 23;
    }

    st->r4_rf_div_sel = 0;

    while (freq < ADF4350_MIN_VCO_FREQ) {
        freq <<= 1;
        st->r4_rf_div_sel++;
    }

    chspc = st->chspc;

	do {
		do {
			do {
				r_cnt = adf4350_tune_r_cnt(st, r_cnt);
				st->r1_mod = st->fpfd / chspc;
				if (r_cnt > ADF4350_MAX_R_CNT) {
					/* try higher spacing values */
					chspc++;
					r_cnt = 0;
				}
			} while ((st->r1_mod > ADF4350_MAX_MODULUS) && r_cnt &&
				(pdata->max_r_value == 0 || r_cnt < pdata->max_r_value));
		} while (r_cnt == 0);

		tmp = freq * (uint64_t)st->r1_mod + ((uint32_t)st->fpfd >> 1);
		adf_4350_do_div(&tmp, st->fpfd); /* Div round closest (n + d/2)/d */
		st->r0_fract = adf_4350_do_div(&tmp, st->r1_mod);
		st->r0_int = tmp;
	} while (mdiv > st->r0_int);

    band_sel_div = DIV_ROUND_UP(st->fpfd, ADF4350_MAX_BANDSEL_CLK);

    if (st->r0_fract && st->r1_mod) {
        div_gcd = gcd(st->r1_mod, st->r0_fract);
        st->r1_mod /= div_gcd;
        st->r0_fract /= div_gcd;
    }
    else {
        st->r0_fract = 0;
        st->r1_mod = 1;
    }

	actual_freq = (((double)st->r0_int + ((double)st->r0_fract / (double)st->r1_mod)) * st->fpfd / (1 << st->r4_rf_div_sel)) / 1000000;

    printf("actual_freq: %.6f MHz VCO: %llu Hz, PFD %.3f Hz\n"
        "REF_DIV %d, R0_INT %d, R0_FRACT %d\n"
        "R1_MOD %d, RF_DIV %d\nPRESCALER %s, BAND_SEL_DIV %d\n",
		actual_freq, freq, st->fpfd, r_cnt, st->r0_int, st->r0_fract, st->r1_mod,
        1 << st->r4_rf_div_sel, prescaler ? "8/9" : "4/5",
        band_sel_div);

    st->regs[ADF4350_REG0] = ADF4350_REG0_INT(st->r0_int) |
        ADF4350_REG0_FRACT(st->r0_fract);

    st->regs[ADF4350_REG1] = ADF4350_REG1_PHASE(1) |
        ADF4350_REG1_MOD(st->r1_mod) |
        prescaler;

    st->regs[ADF4350_REG2] =
        ADF4350_REG2_10BIT_R_CNT(r_cnt) |
        /*ADF4350_REG2_DOUBLE_BUFF_EN |*/
        (pdata->ref_doubler_en ? ADF4350_REG2_RMULT2_EN : 0) |
        (pdata->ref_div2_en ? ADF4350_REG2_RDIV2_EN : 0) |
        (pdata->r2_user_settings & (ADF4350_REG2_PD_POLARITY_POS |
            ADF4350_REG2_LDP_6ns | ADF4350_REG2_LDF_INT_N |
            ADF4350_REG2_CHARGE_PUMP_CURR_uA(5000) |
            ADF4350_REG2_MUXOUT(0x7) | ADF4350_REG2_NOISE_MODE(0x3)));

    st->regs[ADF4350_REG3] = pdata->r3_user_settings &
        (ADF4350_REG3_12BIT_CLKDIV(0xFFF) |
            ADF4350_REG3_12BIT_CLKDIV_MODE(0x3) |
            ADF4350_REG3_12BIT_CSR_EN |
            ADF4351_REG3_CHARGE_CANCELLATION_EN |
            ADF4351_REG3_ANTI_BACKLASH_3ns_EN |
            ADF4351_REG3_BAND_SEL_CLOCK_MODE_HIGH);

    st->regs[ADF4350_REG4] =
        ADF4350_REG4_FEEDBACK_FUND |
        ADF4350_REG4_RF_DIV_SEL(st->r4_rf_div_sel) |
        ADF4350_REG4_8BIT_BAND_SEL_CLKDIV(band_sel_div) |
        ADF4350_REG4_RF_OUT_EN |
        (pdata->r4_user_settings &
        (ADF4350_REG4_OUTPUT_PWR(0x3) |
            ADF4350_REG4_AUX_OUTPUT_PWR(0x3) |
            ADF4350_REG4_AUX_OUTPUT_EN |
            ADF4350_REG4_AUX_OUTPUT_FUND |
            ADF4350_REG4_MUTE_TILL_LOCK_EN));

    st->regs[ADF4350_REG5] = ADF4350_REG5_LD_PIN_MODE_DIGITAL | 0x180000 /* Reserved bits */;

    return true;
}

static int adf4350_tune_r_cnt(adf4350_state_t *st, uint16_t r_cnt)
{
	adf4350_platform_data_t *pdata = &st->pdata;

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