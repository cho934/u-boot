/*
 * Copyright (C) 2008 Sekhar Nori, Texas Instruments, Inc <www.ti.com>
 *
 * Driver for SPI controller on DaVinci. Based on atmel_spi.c 
 * by Atmel Corporation
 * 
 * Copyright (C) 2007 Atmel Corporation
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <spi.h>
#include <malloc.h>

#include <asm/io.h>

#include <asm/arch/hardware.h>

#include "davinci_spi.h"

static unsigned int data1_reg_val;


void spi_init()
{
	/* do nothing */
			
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
			unsigned int max_hz, unsigned int mode)
{
	struct davinci_spi_slave	*ds;
	void			*regs;
	unsigned int fmt0;

	ds = malloc(sizeof(struct davinci_spi_slave));
	if (!ds)
		return NULL;

	ds->slave.bus = bus;
	ds->slave.cs = cs;
	ds->regs = CFG_SPI_BASE;
	ds->freq = max_hz;

	return &ds->slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	struct davinci_spi_slave *ds = to_davinci_spi(slave);

	free(ds);
}

int spi_claim_bus(struct spi_slave *slave)
{
	struct davinci_spi_slave *ds = to_davinci_spi(slave);
	unsigned int scalar;

	/* Enable the SPI hardware */
	spi_writel(ds, GCR0, 0);
	udelay(1000);
	spi_writel(ds, GCR0, 1);

	/* Set master mode, powered up and not activated */
	spi_writel(ds, GCR1, 0x3);

	/* CS, CLK, SIMO and SOMI are functional pins */
	spi_writel(ds, PC0, (1 << 0) | (1 << 9) | (1 << 10) | (1 << 11));

	/* setup format */
	scalar = ((CFG_SPI_CLK / ds->freq) - 1 ) & 0xFF;

	spi_writel(ds, FMT0, 8 |	/* character length */					
					(scalar << 8) | 
					(1 << 16) | /* clock signal delayed by half clk cycle */
					(0 << 17) | /* clock low in idle state - Mode 0 */
					(0 << 20)); /* MSB shifted out first */

	/* hold cs active at end of transfer until explicitly de-asserted */
	data1_reg_val = (1 << 28) | (slave->cs << 16);
	spi_writel(ds, DAT1, data1_reg_val);

	/* including a minor delay. No science here. Should be good even with
	 * no delay
	 */
	spi_writel(ds, DELAY, (50 << 24) | (50 << 16));

	/* default chip select register */
	spi_writel(ds, DEF, 1);

	/* no interrupts */
	spi_writel(ds, INT0, 0);
	spi_writel(ds, LVL, 0);

	/* enable SPI */
	spi_writel(ds, GCR1, spi_readl(ds, GCR1) | (1 << 24));

	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	struct davinci_spi_slave *ds = to_davinci_spi(slave);

	/* Disable the SPI hardware */
	spi_writel(ds, GCR0, 0);
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	struct davinci_spi_slave *ds = to_davinci_spi(slave);
	unsigned int	len_tx;
	unsigned int	len_rx;
	unsigned int	len;
	int		ret, i;
	u32		status;
	const u8	*txp = dout;
	u8		*rxp = din;
	u8		value, dummy = 0;

	ret = 0;
	
	if (bitlen == 0)
		/* Finish any previously submitted transfers */
		goto out;

	/*
	 * It's not clear how non-8-bit-aligned transfers are supposed to be
	 * represented as a stream of bytes...this is a limitation of
	 * the current SPI interface - here we terminate on receiving such a 
	 * transfer request.
	 */
	if (bitlen % 8) {
		/* Errors always terminate an ongoing transfer */
		flags |= SPI_XFER_END;
		goto out;
	}

	len = bitlen / 8;

	/* do an empty read to clear the current contents */
	spi_readl(ds, BUF);

	/* keep writing and reading 1 byte until done */
	for (i = 0; i < len; i++) {

		/* wait till TXFULL is asserted */
		while(spi_readl(ds, BUF) & (1 << 29));

		/* write the data */
		data1_reg_val &= ~0xFFFF;
		if(txp) {
			data1_reg_val |= *txp & 0xFF;
			txp++;
		}

		/* write to DAT1 is required to keep the serial transfer going */
		/* we just terminate when we reach the end */
		if((i == (len -1)) && (flags & SPI_XFER_END)) {
			spi_writel(ds, DAT1, data1_reg_val & ~(1 << 28));	/* clear CS hold */				
		} else {
			spi_writel(ds, DAT1, data1_reg_val);
		}
		

		/* read the data - wait for data availability */
		while(spi_readl(ds, BUF) & (1 << 31));

		if(rxp) {
			*rxp = spi_readl(ds, BUF) & 0xFF;			
			rxp++;
		} else {
			spi_readl(ds, BUF); /* simply drop the read character */
		}

	}

	return 0;

out:
	if (flags & SPI_XFER_END) {
		spi_writel(ds, DAT1, data1_reg_val & ~(1 << 28));
	}

	return 0;
}


#ifdef CONFIG_CMD_EEPROM

/* ------------------------------------------------------------------------ *
 *  SPI ROM Definitions                                                     *
 * ------------------------------------------------------------------------ */
#define SPIROM_SIZE             0x00008000
#define SPIROM_BASE             0x00000000
#define SPIROM_PAGESIZE         32
#define SPIROM_PAGEMASK         0xffffffc0

/* ------------------------------------------------------------------------ *
 *  SPI ROM Commands                                                        *
 * ------------------------------------------------------------------------ */
#define SPIROM_CMD_WRSR         0x01
#define SPIROM_CMD_WRITE        0x02
#define SPIROM_CMD_READ         0x03
#define SPIROM_CMD_WRDI         0x04
#define SPIROM_CMD_RDSR         0x05
#define SPIROM_CMD_WREN         0x06

static struct spi_slave *slave;

void spi_init_f(void)
{
	slave = spi_setup_slave(0, 0, 1*1024*1024, 0);
	spi_claim_bus(slave);
}

static char spirombuf[3];

/* ------------------------------------------------------------------------ *
 *  spirom_status( )                                                        *
 * ------------------------------------------------------------------------ */
static unsigned char spi_get_status( )
{
    /* Issue read status command */
    spirombuf[0] = SPIROM_CMD_RDSR;
    spirombuf[1] = 0;

	spi_xfer(slave, (2)*8, spirombuf, spirombuf, SPI_XFER_BEGIN | SPI_XFER_END);		
	
    return spirombuf[1];
}

ssize_t spi_write(uchar *addr, int alen, uchar *buffer, int len)
{

	spirombuf[0] = SPIROM_CMD_WREN;
    spi_xfer(slave, 1*8, spirombuf, NULL, SPI_XFER_BEGIN | SPI_XFER_END);

	/* Create command block for program operation */
    spirombuf[0] = SPIROM_CMD_WRITE;
    spirombuf[1] = addr[0];
    spirombuf[2] = addr[1];

    spi_xfer(slave, 3 * 8, spirombuf, NULL, SPI_XFER_BEGIN);
    spi_xfer(slave, len * 8, buffer, NULL, SPI_XFER_END);

    /* Wait while busy */
    while( (spi_get_status( ) & 0x01 ) );

	return len;
}

ssize_t spi_read(uchar *addr, int alen, uchar *buffer, int len)
{
	spirombuf[0] = 0x3;
    spirombuf[1] = addr[0];
    spirombuf[2] = addr[1];

	spi_xfer(slave, 3*8, spirombuf, NULL, SPI_XFER_BEGIN);		
	spi_xfer(slave, len*8, NULL, buffer, SPI_XFER_END);		

	return len;
}

#endif
