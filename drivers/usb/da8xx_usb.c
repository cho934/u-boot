/*
 * TI's DA8xx platform specific usb wrapper functions.
 *
 * Copyright (c) 2004 Texas Instruments
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Thomas Abraham t-abraham@ti.com, Texas Instruments
 *
 * Copyright (c) 2003 Wolfgang Denk, wd@denx.de
 *
 */

#include <common.h>

#ifdef CONFIG_USB_DA8XX

#include "da8xx_usb.h"

/* extern functions */
extern void lpsc_on(unsigned int id);

/* Timeout for DA8xx usb module */
#define DA8XX_USB_TIMEOUT 0x3FFFF


/* This function writes to a 32-bit register of platform usb wrapper */
inline void pusb_writel( u32 offset , u32 value )
{
	*(volatile u32*)(DA8XX_USB0_BASE+offset) = value;
}

/* This function reads a 32-bit register of platform usb wrapper */
inline u32 pusb_readl(u32 offset)
{
	return(*(volatile u32*)(DA8XX_USB0_BASE+offset));
}

/* This function writes to a 16-bit register of platform musb core */
inline void musb_writew(u32 offset, u16 value)
{
	*(volatile u16*)(MENTOR_USB0_BASE+offset) = value;
}

/* This function writes to a 8-bit register of platform musb core */
inline void musb_writeb(u32 offset, u8 value)
{
	*(volatile u8*)(MENTOR_USB0_BASE+offset) = value;
}

/* This function reads a 16-bit register of platform usb wrapper */
inline u16 musb_readw(u32 offset)
{
	return(*(volatile u16*)(MENTOR_USB0_BASE + offset));
}

/* This function reads a 8-bit register of platform usb wrapper */
inline u8 musb_readb(u32 offset)
{
	return(*(volatile u8*)(MENTOR_USB0_BASE+offset));
}

/*
 * This function enables VBUS by driving the GPIO Bank4 Pin 15 high.
 */
static void enable_vbus(void)
{
	u32 value;

	/* configure GPIO bank4 pin 15 in output direction */
	value = *(volatile u32 *)GPIO_BANK4_REG_DIR_ADDR;
	value &= ~0x8000;
	*(volatile u32 *)GPIO_BANK4_REG_DIR_ADDR = value;

	/* set GPIO bank4 pin 15 high to drive VBUS */
	value = *(volatile u32 *)GPIO_BANK4_REG_SET_ADDR;
	value |= 0x8000;
	*(volatile u32 *)GPIO_BANK4_REG_SET_ADDR = value;
}

/*
 * Enable the usb0 phy. This initialization procedure is explained in
 * the DA8xx USB user guide document.
 */
static u8 phy_on( void )
{
	u32 timeout;
	u8	result = 1;
#if 0
	/* write access keys */
	*(volatile u32 *)(KICK0) = 0x83e70b13;
	*(volatile u32 *)(KICK1) = 0x95a4f1e0;
#endif

#ifdef CONFIG_USE_PINMUX

	/* set PINMUX[7:4] = 1 */
	*(volatile u32 *)(PINMUX9) |= ( 1 << 4 );
	*(volatile u32 *)(PINMUX9) &= 0xFFFFFF7F;

#endif

	/* reset the usb controller */
	pusb_writel( DA8XX_USB_CTRL_REG , 0x1 );
	udelay( 5000 );

	/* enable and then disable the usb phy reset */
	*(volatile u32*)CFGCHIP2 |= 0x00008000;
	udelay( 5000 );
	*(volatile u32*)CFGCHIP2 &= 0xFFFF7FFF;
	udelay( 5000 );

	/* configure phy (refer to da8xx usb user guide use case section) */
	*(volatile u32*)CFGCHIP2 &= 0xFFFF9FFF;
	*(volatile u32*)CFGCHIP2 &= 0xFFFFFBFF;
	*(volatile u32*)CFGCHIP2 &= 0xFFFFFDFF;
	*(volatile u32*)CFGCHIP2 |= 0x00000100;
	*(volatile u32*)CFGCHIP2 |= 0x00000020;
	*(volatile u32*)CFGCHIP2 |= 0x00000010;
	*(volatile u32*)CFGCHIP2 |= 0x00000002;
	*(volatile u32*)CFGCHIP2 &= 0xFFFFE7FF;
	*(volatile u32*)CFGCHIP2 |= 0x00000800;

	*(volatile u32*)CFGCHIP2 &= 0xFFFF9FFF;
	*(volatile u32*)CFGCHIP2 |= 0x00002000;
	*(volatile u32*)CFGCHIP2 |= 0x00000040;

	/* wait until the usb phy pll locks */
	timeout = DA8XX_USB_TIMEOUT;
	do {
		timeout--;
		if (timeout == 0)
			break;
	}
	while(((*(volatile u32*)CFGCHIP2) & 0x00020000) == 0);
#if 0
	/* disable register access by writing invalid keys */
	*(volatile u32 *)(0x01C14038) = 0x0;
	*(volatile u32 *)(0x01C1403C) = 0x0;
#endif
	if (timeout == 0)
		result = 0;

	return(result);
}

/*
 * Disable the usb phy
 */
static void phy_off(void)
{
#if 0
	/* write access keys */
	*(volatile u32 *)(KICK0) =	0;
	*(volatile u32 *)(KICK1) =	0;
#endif
	/* powerdown the on-chip PHY and its oscillator */
	*(volatile u32*)(CFGCHIP2) = 0x8600;
}


/*
 * This function performs DA8xx platform specific initialization for usb0.
 */
int musb_platform_init(void)
{
	u32  revision;

	/* enable psc for usb2.0 */
	lpsc_on(33);

	/* enable usb vbus */
	enable_vbus();

	/* start the on-chip usb phy and its pll */
	if (phy_on() == 0)
		return(-1);

	/* reset the controller */
	pusb_writel(DA8XX_USB_CTRL_REG, 0x1);
	udelay(5000);

	/* Returns zero if e.g. not clocked */
	revision = pusb_readl(DA8XX_USB_VERSION_REG);
	if (revision == 0)
		return(-1);

	/* Disable all interrupts */
	pusb_writel(DA8XX_USB_INT_MASK_SET_REG, 0x01FF1F1F);
	return(0);
}


/*
 * This function performs DA8xx platform specific deinitialization for usb0.
 */
void musb_platform_deinit(void)
{
	/* Turn of the phy */
	phy_off();

	/* flush any interrupts */
	pusb_writel(DA8XX_USB_INT_MASK_CLR_REG, DA8XX_USB_USBINT_MASK|DA8XX_USB_TXINT_MASK|DA8XX_USB_RXINT_MASK);
	pusb_writel(DA8XX_USB_EOI_REG, 0);
}

#endif /* CONFIG_USB_DA8XX */

