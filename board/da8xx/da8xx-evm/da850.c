/*
 * Copyright (C) 2008 Sekhar Nori, Texas Instruments, Inc.  <nsekhar@ti.com>
 * 
 * Modified for DA8xx EVM. 
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Parts are shamelessly stolen from various TI sources, original copyright
 * follows:
 * -----------------------------------------------------------------
 *
 * Copyright (C) 2004 Texas Instruments.
 *
 * ----------------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 */

#include <common.h>
#include <i2c.h>
#include <spi.h>
#include <spi_flash.h>
#include <net.h>
#include <asm/errno.h>
#include <asm/arch/hardware.h>
#include <asm/arch/emac_defs.h>

#define MACH_TYPE_DA850_EVM		2157

DECLARE_GLOBAL_DATA_PTR;

extern void	timer_init(void);
extern int	eth_hw_init(void);

/* Works on Always On power domain only (no PD argument) */
void lpsc_on(unsigned int id)
{
	dv_reg_p	mdstat, mdctl, ptstat, ptcmd;

	if (id >= 64)
		return;	

	if(id < 32) {
	mdstat = REG_P(PSC0_MDSTAT + (id * 4));
		mdctl = REG_P(PSC0_MDCTL + (id * 4));
		ptstat = REG_P(PSC0_PTSTAT);
		ptcmd = REG_P(PSC0_PTCMD);
	} else {
		id -= 32;
		mdstat = REG_P(PSC1_MDSTAT + (id * 4));
		mdctl = REG_P(PSC1_MDCTL + (id * 4));
		ptstat = REG_P(PSC1_PTSTAT);
		ptcmd = REG_P(PSC1_PTCMD);
	}
	
	while (*ptstat & 0x01) {;}

	if ((*mdstat & 0x1f) == 0x03)
		return;			/* Already on and enabled */

	*mdctl |= 0x03;

	*ptcmd = 0x01;

	while (*ptstat & 0x01) {;}
	while ((*mdstat & 0x1f) != 0x03) {;}	/* Probably an overkill... */
}

int board_init(void)
{

	dv_reg_p intc;		

	/*-------------------------------------------------------*
	 * Mask all IRQs by clearing the global enable and setting
	 * the enable clear for all the 90 interrupts. This code is
	 * also included in low level init. Including it here in case
	 * low level init is skipped. Not removing it from low level
	 * init in case some of the low level init code generates 
	 * interrupts... Not expected... but you never know...
	 *-------------------------------------------------------*/
		
#ifndef CONFIG_USE_IRQ
	intc = REG_P(INTC_GLB_EN);
	intc[0] = 0;	

	intc = REG_P(INTC_HINT_EN);
	intc[0] = 0;
	intc[1] = 0;
	intc[2] = 0;			

	intc = REG_P(INTC_EN_CLR0);
	intc[0] = 0xFFFFFFFF;
	intc[1] = 0xFFFFFFFF;
	intc[2] = 0xFFFFFFFF;
#endif

	/* arch number of the board */
	gd->bd->bi_arch_number = MACH_TYPE_DA850_EVM;

	/* address of boot parameters */
	gd->bd->bi_boot_params = LINUX_BOOT_PARAM_ADDR;

	/* setup the SUSPSRC for ARM to control emulation suspend */
	REG(SUSPSRC) &= ~( (1 << 27) 	/* Timer0 */
			| (1 << 21) 	/* SPI0 */
			| (1 << 18) 	/* UART0 */ 
			| (1 << 5) 	/* EMAC */
			| (1 << 16) 	/* I2C0 */
			);	

	/* Power on required peripherals 
	 * ARM does not have acess by default to PSC0 and PSC1
	 * assuming here that the DSP bootloader has set the IOPU
	 * such that PSC access is available to ARM
	 */
	lpsc_on(DAVINCI_LPSC_AEMIF);	/* NAND, NOR */
	lpsc_on(DAVINCI_LPSC_SPI0);	 /* Serial Flash */
	lpsc_on(DAVINCI_LPSC_EMAC);	 /* image download */
	lpsc_on(DAVINCI_LPSC_UART0);	/* console */
	lpsc_on(DAVINCI_LPSC_GPIO);

	/* Pin Muxing support */
	
#ifdef CONFIG_SPI_FLASH
	/* SPI0, use CLK, SOMI, SIMO, CS[0] */
	REG(PINMUX3) &= 0xFFFF00F0;
	REG(PINMUX3) |= 0x00001101;
	REG(PINMUX4) &= 0xFFFFFF0F;
	REG(PINMUX4) |= 0x00000010;
#endif

#ifdef CONFIG_DRIVER_TI_EMAC

	/* Assumes RMII clock sourced externally */
#ifdef CONFIG_DRIVER_TI_EMAC_USE_RMII
	REG(PINMUX14) &= 0x000000FF; 
	REG(PINMUX14) |= 0x88888800; 
	REG(PINMUX15) &= 0xFFFFFF00; 
	REG(PINMUX15) |= 0x00000080; 

	/* set cfgchip3 to selct RMII */
	REG(CFGCHIP3) |= (1 << 8);

#else	/* Use MII */
	REG(PINMUX2) &= 0x0000000F;	
	REG(PINMUX2) |= 0x88888880;	
	REG(PINMUX3) = 0x88888880;	

	/* set cfgchip3 to selct MII */
	REG(CFGCHIP3) &= ~(1 << 8);
#endif
	/* MDIO */
	REG(PINMUX4)  &= 0xFFFFFF00; 
	REG(PINMUX4)  |= 0x00000088; 
#endif

	/* Async EMIF */
#if defined(CONFIG_SYS_USE_NAND) || defined(CONFIG_SYS_USE_NOR)
	REG(PINMUX6)  =  0x11111111;
	REG(PINMUX7)  =  0x11111111;
	REG(PINMUX8)  =  0x11111111;
	REG(PINMUX9)  =  0x11111111;
	REG(PINMUX10) =  0x11111111;
	REG(PINMUX11) =  0x11111111;
	REG(PINMUX12) =  0x11111111;
	REG(PINMUX5)  &= 0x00FFFFFF;
	REG(PINMUX5)  |= 0x11000000;
#endif

	/* UART0 Muxing and enabling */
	REG(PINMUX3) &= 0x0000FFFF; 
	REG(PINMUX3) |= 0x22220000;

	REG(DAVINCI_UART0_BASE + 0x30) = 1 | (1 << 13) | (1 << 14);

	/* I2C muxing */
	REG(PINMUX4) &= 0xFFFF00FF;
	REG(PINMUX4) |= 0x00002200;

	return(0);
}

#define CFG_MAC_ADDR_SPI_BUS	0
#define CFG_MAC_ADDR_SPI_CS	0
#define CFG_MAC_ADDR_SPI_MAX_HZ	1000000
#define CFG_MAC_ADDR_SPI_MODE	SPI_MODE_3

#define CFG_MAC_ADDR_OFFSET	(flash->size - SZ_4K)

static int  get_mac_addr(u8 *addr)
{
	int ret;
	struct spi_flash *flash;

	flash = spi_flash_probe(CFG_MAC_ADDR_SPI_BUS, CFG_MAC_ADDR_SPI_CS,
			CFG_MAC_ADDR_SPI_MAX_HZ, CFG_MAC_ADDR_SPI_MODE);
	if (!flash) {
		printf(" Error - unable to probe SPI flash.\n");
		goto err_probe;
	}

	ret = spi_flash_read(flash, CFG_MAC_ADDR_OFFSET, 6, addr);
	if (ret) {
		printf("Error - unable to read MAC address from SPI flash.\n");
		goto err_read;
	}

err_read:
	/* cannot call free currently since the free function calls free() for
	 * spi_flash structure though it is not directly allocated through 
	 * malloc()
	 */
	/* spi_flash_free(flash); */
err_probe:
	return ret;
}

int misc_init_r (void)
{
	u_int8_t	tmp[20], addr[10];

	printf ("ARM Clock : %d Hz\n", clk_get(DAVINCI_ARM_CLKID));

	if (getenv("ethaddr") == NULL) {
		/* Set Ethernet MAC address from EEPROM */
		get_mac_addr(addr);

		if(is_multicast_ether_addr(addr) || is_zero_ether_addr(addr)) {
			printf("Invalid MAC address read.\n");
			return -EINVAL;	
		}
		sprintf((char *)tmp, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0],
			addr[1], addr[2], addr[3], addr[4], addr[5]);

		setenv("ethaddr", (char *)tmp);
	}

	if (!eth_hw_init()) {
		printf("Error: Ethernet init failed!\n");
	}

	return(0);
}

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return(0);
}
