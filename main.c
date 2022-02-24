/*
 *   File:   main.c
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
#include <util/delay.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "iopins.h"
#include "config.h"
#include "i2c.h"
#include "timer.h"
#include "counters.h"
#include "usart.h"
#include "cmd.h"
#include "util.h"
#include "adf4350.h"

#define NO_READING_AVAILABLE        0x7FFFFFF
#define SENSOR_STATE_ONLINE         0
#define SENSOR_STATE_ERROR          1
#define SENSOR_STATE_OFFLINE        2

#define T_TOGGLE_BUTTON             0x01
#define T_BUTTON_STATUS             0x02
#define T_BUTTON_UPDATE             0x04
#define T_GPIO_STATUS               0x08

typedef struct
{
    sys_config_t *config;
} sys_runstate_t;

sys_config_t _g_cfg;
sys_runstate_t _g_rs;
sys_counters_t _g_counters;

FILE uart_str = FDEV_SETUP_STREAM(print_char, NULL, _FDEV_SETUP_RW);

static void io_init(void);
static void clock_init(void);

int main(void)
{
    sys_runstate_t *rs = &_g_rs;
    sys_config_t *config = &_g_cfg;

    memset((void *)&_g_counters, 0x00, sizeof(sys_counters_t));

    rs->config = config;

    clock_init();
    io_init();
    timer_tcb0_init();
    g_irq_enable();

    usart0_open(USART_CONT_RX, USART_BAUD_RATE(UART0_BAUD)); // Console

    stdout = &uart_str;

    printf("\r\nStarting up...\r\n");

    load_configuration(config);

    do_freq(config);

    // Idle loop
    for (;;)
    {
        cmd_process(config);
    }
}

void do_freq(sys_config_t *config)
{
    adf4350_calculated_parameters_t params;
    adf4350_platform_data_t settings;

    settings.clkin = 25000000;
    settings.channel_spacing = 1000;
    settings.max_r_value = config->r_value;
	settings.ref_div2_en = false;
	settings.ref_doubler_en = false;
	settings.r2_user_settings = ADF4350_REG2_NOISE_MODE(0) | ADF4350_REG2_LDP_10ns | ADF4350_REG2_MUXOUT(0UL)
		| ADF4350_REG2_PD_POLARITY_POS | ADF4350_REG2_CHARGE_PUMP_CURR_uA(2500) | ADF4350_REG2_LDF_FRACT_N;
	settings.r3_user_settings = ADF4350_REG3_12BIT_CLKDIV(150UL) | ADF4350_REG3_12BIT_CLKDIV_MODE(0UL);
	settings.r4_user_settings = ADF4350_REG4_OUTPUT_PWR(3UL) | ADF4350_REG4_RF_OUT_EN;

    if (!adf4350_set_freq(config->freq * 1000 /* Hz from here on */, &settings, &params))
    {
        printf("Set frequency command failed\r\n");
        return;
    }

    uint32_t pfd = (uint32_t)(params.pfd / 1000);
    uint32_t actual_freq = (uint32_t)(params.actual_freq / 1000000000);
    uint32_t actual_freq_rem = (uint32_t)(params.actual_freq % 1000000000) / 1000;
    uint32_t vco = (uint32_t)(params.vco);

    printf("\r\nCalculated state:\r\n\r\n"
           "\tActual frequency ..: %lu.%lu MHz\r\n"
           "\tVCO ...............: %lu Hz\r\n"
           "\tPFD ...............: %lu Hz\r\n"
           "\tREF_DIV ...........: %d\r\n"
           "\tR0_INT ............: %d\r\n"
           "\tR0_FRACT ..........: %d\r\n"
           "\tR1_MOD ............: %d\r\n"
           "\tRF_DIV ............: %d\r\n"
           "\tPRESCALER .........: %s\r\n"
           "\tBAND_SEL_DIV ......: %d\r\n"
           "\tR0 ................: 0x%08lX\r\n"
           "\tR1 ................: 0x%08lX\r\n"
           "\tR2 ................: 0x%08lX\r\n"
           "\tR3 ................: 0x%08lX\r\n"
           "\tR4 ................: 0x%08lX\r\n"
           "\tR5 ................: 0x%08lX\r\n\r\n",
		actual_freq, actual_freq_rem, vco,
        pfd, params.r_cnt, params.intv,
        params.fract, params.mod, params.rf_div,
        params.prescaler ? "8/9" : "4/5",
        params.band_sel_div,
        params.regs[0], params.regs[1], params.regs[2], params.regs[3], params.regs[4], params.regs[5]);
}

static void clock_init(void)
{
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0 << CLKCTRL_PEN_bp); // Disable prescaler
}

static void io_init(void)
{
    IO_INPUT(LD);
    IO_OUTPUT(LE);
    IO_OUTPUT(DATA);
    IO_OUTPUT(CLOCK);

    IO_LOW(LE);
    IO_LOW(CLOCK);
    IO_LOW(DATA);
}

int print_char(char byte, FILE *stream)
{
    while (console_busy());
    console_put(byte);
    return 0;
}