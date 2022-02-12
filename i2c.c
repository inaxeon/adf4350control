/*
 * I2C.c
 *
 * I2C Master
 *
 * Created: 10/05/2019
 * Author: Dieter Reinhardt
 *
 * Tested with Alternate Pinout
 *
 * This software is covered by a modified MIT License, see paragraphs 4 and 5
 *
 * Copyright (c) 2019 Dieter Reinhardt
 *
 * 1. Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * 2. The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * 3. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * 4. This software is strictly NOT intended for safety-critical or life-critical applications.
 * If the user intends to use this software or parts thereof for such purpose additional 
 * certification is required.
 *
 * 5. Parts of this software are adapted from Microchip I2C polled master driver sample code.
 * Additional license restrictions from Microchip may apply.
 */ 

#include "project.h"

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>

#include "iopins.h"
#include "i2c.h"
#include "counters.h"

#define ADDR_TIMEOUT	5			// 5 ms timeout
#define READ_TIMEOUT	5
#define WRITE_TIMEOUT	5

#define I2C_READ    1
#define I2C_WRITE   0

#define TWI0_BAUD(F_SCL, T_RISE) ((((((float)20000000.0 / (float)F_SCL)) - 10 - ((float)20000000.0 * T_RISE / 1000000))) / 2)
//#define TWI0_BAUD(F_SCL, T_RISE) 0x0014

void i2c_init(uint16_t freq_khz)
{
    IO_OUTPUT(SDA);
    IO_INPUT(SCL);

    IO_LOW(SDA);
    IO_LOW(SCL);

    SDA_PINCTRL = 0x00;
    SCL_PINCTRL = 0x00;

	TWI0.MBAUD = (uint8_t)TWI0_BAUD(freq_khz / 1000, 0);
	TWI0.MCTRLB = TWI_FLUSH_bm;										// clear the internal state of the master
	TWI0.MCTRLA =	  1 << TWI_ENABLE_bp							// Enable TWI Master: enabled
					| 0 << TWI_QCEN_bp								// Quick Command Enable: disabled
					| 0 << TWI_RIEN_bp								// Read Interrupt Enable: disabled
					| 0 << TWI_SMEN_bp								// Smart Mode Enable: disabled
					| TWI_TIMEOUT_DISABLED_gc						// Bus Timeout Disabled (inoperative, see errata)
					| 0 << TWI_WIEN_bp;								// Write Interrupt Enable: disabled

    IO_OUTPUT(SCL);

	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;							// force bus idle
	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm | TWI_BUSERR_bm);		// clear flags	
}

#ifdef _I2C_BRUTEFORCE_RESET_

void i2c_bruteforce_reset(void)										// clock out I2C bus if in invalid state (e.g. after incomplete transaction)
{
	uint8_t	i;
	TWI0.MCTRLB |= TWI_FLUSH_bm;									// clear the internal state of the master
	TWI0.MCTRLA = 0;												// disable TWI Master

	IO_INPUT(SDA);								// SDA tri-stated high

	for (i = 0; i < 9; i++)											// SCL, 9 x bit-banging
	{
		IO_OUTPUT(SCL);							// 5 us low
		_delay_us(5);
		IO_INPUT(SCL);							// 5 us tri-stated high
		_delay_us(5);
	}
	
// re-enable master twice
// for unknown reasons the master might get stuck if re-enabled only once
// second re-enable will fail if SDA not enabled beforehand

	TWI0.MCTRLB = TWI_FLUSH_bm;										// clear the internal state of the master
	TWI0.MCTRLA =	  1 << TWI_ENABLE_bp							// Enable TWI Master: enabled
					| 0 << TWI_QCEN_bp								// Quick Command Enable: disabled
					| 0 << TWI_RIEN_bp								// Read Interrupt Enable: disabled
					| 0 << TWI_SMEN_bp								// Smart Mode Enable: disabled
					| TWI_TIMEOUT_DISABLED_gc						// Bus Timeout Disabled (inoperative, see errata)
					| 0 << TWI_WIEN_bp;								// Write Interrupt Enable: disabled			

    IO_OUTPUT(SDA);

	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;							// force bus idle
	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm | TWI_BUSERR_bm);		// clear flags

	TWI0.MCTRLB = TWI_FLUSH_bm;										// clear the internal state of the master (glitch on SDA)
	TWI0.MCTRLA =	  1 << TWI_ENABLE_bp							// Enable TWI Master: enabled
					| 0 << TWI_QCEN_bp								// Quick Command Enable: disabled 
					| 0 << TWI_RIEN_bp								// Read Interrupt Enable: disabled
					| 0 << TWI_SMEN_bp								// Smart Mode Enable: disabled
					| TWI_TIMEOUT_DISABLED_gc						// Bus Timeout Disabled (inoperative, see errata)
					| 0 << TWI_WIEN_bp;								// Write Interrupt Enable: disabled

    IO_OUTPUT(SCL);

	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;							// force bus idle
	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm | TWI_BUSERR_bm);		// clear flags
}

#endif /* _I2C_BRUTEFORCE_RESET_ */

static uint8_t i2c_start(uint8_t device_addr)	// device_addr LSB set if READ
{
	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);	// clear Read and Write interrupt flags

	if (TWI0.MSTATUS & TWI_BUSERR_bm)
		return 4;						// Bus Error, abort

	TWI0.MADDR = device_addr;

	return 0;
}

uint8_t i2c_wait_ack(void)
{
	_g_counters.i2c_timeout_cnt = 0;	// reset timeout counter, will be incremented by ms tick interrupt

	while (!(TWI0.MSTATUS & TWI_RIF_bm) && !(TWI0.MSTATUS & TWI_WIF_bm)) // wait for RIF or WIF set
	{
		if (_g_counters.i2c_timeout_cnt > ADDR_TIMEOUT)
		{
			_g_counters.i2c_timeouts++;
			return 0xff;				// return timeout error
		}
	}

	TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);	// clear Read and Write interrupt flags

	if (TWI0.MSTATUS & TWI_BUSERR_bm)
		return 4;		// Bus Error, abort

	if (TWI0.MSTATUS & TWI_ARBLOST_bm)
		return 2;		// Arbitration Lost, abort

	if (TWI0.MSTATUS & TWI_RXACK_bm)
		return 1;		// Slave replied with NACK, abort

	return 0;			// no error
}

// the Atmel device documentation mentions a special command for repeated start TWI_MCMD_REPSTART_gc,
// but this is not used in Atmel's demo code, so we don't use it either

static void i2c_rep_start(uint8_t device_addr)
{
	TWI0.MADDR = device_addr;	
}

static uint8_t i2c_read_byte_internal(uint8_t *data, uint8_t ack_flag)
{
	_g_counters.i2c_timeout_cnt = 0;	// reset timeout counter, will be incremented by ms tick interrupt

    // Matt: This was checking if the bus was owned, but in the case of the first transaction
    // being a read, the silicon appears to kick the read off its self, and may be done by now
    // transitioning the bus back to idle by the time this check is performed. It looks likely
    // that it's better to just check for 'busy' here, as both 'owned' and 'idle' appear to be valid
	if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) != TWI_BUSSTATE_BUSY_gc)	// if master controls bus
	{		
		while (!(TWI0.MSTATUS & TWI_RIF_bm))	// wait for RIF set (data byte received)
		{
			if (_g_counters.i2c_timeout_cnt > READ_TIMEOUT)
			{
				_g_counters.i2c_timeouts++;
				return 0xff;			// return timeout error
			}
		}

		TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);	// clear Read and Write interrupt flags	

		if (TWI0.MSTATUS & TWI_BUSERR_bm)
			return 4;					// Bus Error, abort

		if (TWI0.MSTATUS & TWI_ARBLOST_bm)
			return 2;				// Arbitration Lost, abort

		if (TWI0.MSTATUS & TWI_RXACK_bm)
			return 1;					// Slave replied with NACK, abort				

		if (ack_flag == 0)
			TWI0.MCTRLB &= ~(1 << TWI_ACKACT_bp);	// setup ACK
		else
			TWI0.MCTRLB |= TWI_ACKACT_NACK_gc;		// setup NACK (last byte read)

		*data = TWI0.MDATA;

		if (ack_flag == 0)
			TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;	// send ACK, more bytes to follow					

		return 0;
	}
	else
	{
		return 8;						// master does not control bus
	}
}

static uint8_t i2c_write_byte_internal(uint8_t *data)
{
	_g_counters.i2c_timeout_cnt = 0;	// reset timeout counter, will be incremented by ms tick interrupt

	if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc)	// if master controls bus
	{
		TWI0.MDATA = *data;		

		while (!(TWI0.MSTATUS & TWI_WIF_bm))	// wait until WIF set, status register contains ACK/NACK bit
		{
			if (_g_counters.i2c_timeout_cnt > WRITE_TIMEOUT)
			{
				_g_counters.i2c_timeouts++;
				return 0xff;			// return timeout error
			}
		}

		if (TWI0.MSTATUS & TWI_BUSERR_bm)
			return 4;					// Bus Error, abort

		if (TWI0.MSTATUS & TWI_RXACK_bm)
			return 1;					// Slave replied with NACK, abort

		return 0;						// no error	
	}
	else
	{
		return 8;						// master does not control bus
	}
}

static void i2c_stop()
{
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}

#ifdef _I2C_XFER_BYTE_

bool i2c_read_byte(uint8_t slave_addr, uint8_t *addr_ptr)
{
	uint8_t status;

	status = i2c_start(slave_addr << 1 | I2C_READ);
	
	if (status != 0)
		goto error;

	status = i2c_read_byte_internal(addr_ptr, 1);

	if (status != 0)
		goto error;
		
	i2c_stop();
	return true;

error:
	i2c_stop();
	return false;
}

bool i2c_write_byte(uint8_t slave_addr, uint8_t data)
{
	uint8_t status;

	status = i2c_start(slave_addr << 1 | I2C_WRITE);

	if (status != 0)
		goto error;

	status = i2c_wait_ack();

	if (status == 1)
		goto error;

	status = i2c_write_byte_internal(&data);

	if (status != 0)
		goto error;

	i2c_stop();
	return true;

error:
	i2c_stop();
	return false;
}

#endif /* _I2C_XFER_BYTE_ */

#ifdef _I2C_XFER_

bool i2c_read(uint8_t slave_addr, uint8_t slave_reg, uint8_t *addr_ptr)
{
	uint8_t status;

	status = i2c_start(slave_addr << 1 | I2C_WRITE);

	if (status != 0)
		goto error;

	status = i2c_wait_ack();

	if (status == 1)
		goto error;

	if (status != 0)
		goto error;

	status = i2c_write_byte_internal(&slave_reg);
	
	if (status != 0)
		goto error;

	i2c_rep_start(slave_addr << 1 | I2C_READ);
	status = i2c_read_byte_internal(addr_ptr, 1);

	if (status != 0)
		goto error;

	i2c_stop();
	return true;
	
error:
	i2c_stop();
	return false;
}

bool i2c_write(uint8_t slave_addr, uint8_t slave_reg, uint8_t data)
{
	uint8_t status;
	status = i2c_start(slave_addr << 1 | I2C_WRITE);

	if (status != 0)
		goto error;

	status = i2c_wait_ack();

	if (status == 1)
		goto error;

	if (status != 0)
		goto error;

	status = i2c_write_byte_internal(&slave_reg);
	
	if (status != 0)
		goto error;
	
	status = i2c_write_byte_internal(&data);
	
	if (status != 0)
		goto error;
	
	i2c_stop();
	return true;

error:
	return false;
}

#endif /* _I2C_XFER_ */

#ifdef _I2C_DS2482_SPECIAL_

bool i2c_await_flag(uint8_t slave_addr, uint8_t mask, uint8_t *ret, uint8_t attempts)
{
	uint8_t status;
	uint8_t reg;

	status = i2c_start(slave_addr << 1 | I2C_READ);
	
	if (status != 0)
		goto error;

	while (attempts--)
	{
		status = i2c_read_byte_internal(&reg, 0);

		if (status != 0)
			goto error;

        if (!(reg & mask))
            break;
	}

	status = i2c_read_byte_internal(&reg, 1);

	if (status != 0)
		goto error;

	i2c_stop();
	
	*ret = reg;
    return !(reg & mask);
	
error:
	i2c_stop();
	return false;
}

#endif /* _I2C_DS2482_SPECIAL_ */

#ifdef _I2C_XFER_MANY_

bool i2c_read_buf(uint8_t slave_addr, uint8_t slave_reg, uint8_t *addr_ptr, uint8_t num_bytes)
{
	uint8_t status;

	status = i2c_start(slave_addr << 1 | I2C_WRITE);

	if (status != 0)
		goto error;

	status = i2c_wait_ack();

	if (status == 1)
		goto error;

	if (status != 0)
		goto error;

	status = i2c_write_byte_internal(&slave_reg);

	if (status != 0)
		goto error;

	i2c_rep_start(slave_addr << 1 | I2C_READ);
	
	while (num_bytes > 1)
	{
		status = i2c_read_byte_internal(addr_ptr, 0);

		if (status != 0)
			goto error;

		addr_ptr++;
		num_bytes--;
	}

	status = i2c_read_byte_internal(addr_ptr, 1);

	if (status != 0)
		goto error;

	i2c_stop();
	return true;
	
error:
	i2c_stop();
	return false;
}

bool i2c_write_buf(uint8_t slave_addr, uint8_t slave_reg, uint8_t *addr_ptr, uint8_t num_bytes)
{
	uint8_t status;

	status = i2c_start(slave_addr << 1 | I2C_WRITE);
	
	if (status != 0)
		goto error;

	status = i2c_wait_ack();

	if (status == 1)
		goto error;

	if (status != 0)
		goto error;

	status = i2c_write_byte_internal(&slave_reg);

	if (status != 0)
		goto error;

	while (num_bytes > 0)
	{
		status = i2c_write_byte_internal(addr_ptr);

		if (status != 0)
			goto error;

		addr_ptr++;
		num_bytes--;
	}

	i2c_stop();
	return true;

error:
	i2c_stop();
	return false;
}

#endif /* _I2C_XFER_MANY_ */