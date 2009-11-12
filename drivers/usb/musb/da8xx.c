/*
 * da8xx.c - TI's DA8xx platform specific usb wrapper functions.
 *
 * Author: Ajay Kumar Gupta <ajay.gupta@ti.com>
 *
 * Based on drivers/usb/musb/davinci.c
 *
 * Copyright (c) 2009 Texas Instruments Incorporated
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include <common.h>

#ifdef CONFIG_USB_DA8XX

#include "da8xx.h"

/* MUSB platform configuration */
struct musb_config musb_cfg = {
	(struct	musb_regs *)DA8XX_USB_OTG_CORE_BASE,
	DA8XX_USB_OTG_TIMEOUT,
	0
};

/*
 * This function enables VBUS by driving the GPIO Bank4 Pin 15 high.
 */
static void enable_vbus(void)
{
	u32 value;

	/* configure GPIO bank4 pin 15 in output direction */
	value = readl(GPIO_BANK4_REG_DIR_ADDR);
	writel((value & (~USB_VBUS_GPIO)), GPIO_BANK4_REG_DIR_ADDR);

	/* set GPIO bank4 pin 15 high to drive VBUS */
	value = readl(GPIO_BANK4_REG_SET_ADDR);
	writel((value | USB_VBUS_GPIO), GPIO_BANK4_REG_SET_ADDR);
}

/*
 * Enable the usb0 phy. This initialization procedure is explained in
 * the DA8xx USB user guide document.
 */
static u8 phy_on(void)
{
	u32 timeout;
	u32 cfgchip2;

	cfgchip2 = readl(CFGCHIP2);

	cfgchip2 &= ~(CFGCHIP2_RESET | CFGCHIP2_PHYPWRDN | CFGCHIP2_OTGPWRDN |
			CFGCHIP2_OTGMODE | CFGCHIP2_REFFREQ);
	cfgchip2 |= CFGCHIP2_SESENDEN | CFGCHIP2_VBDTCTEN | CFGCHIP2_PHY_PLLON |
			CFGCHIP2_REFFREQ_24MHZ;

	writel(cfgchip2, CFGCHIP2);

	/* wait until the usb phy pll locks */
	timeout = musb_cfg.timeout;
	while (timeout--)
		if (readl(CFGCHIP2) & CFGCHIP2_PHYCLKGD)
			return 1;

	/* USB phy was not turned on */
	return 0;
}

/*
 * Disable the usb phy
 */
static void phy_off(void)
{
	u32 cfgchip2;

	/*
	 * Power down the on-chip PHY.
	 */
	cfgchip2 = readl(CFGCHIP2);

	cfgchip2 &= ~CFGCHIP2_PHY_PLLON;
	cfgchip2 |= CFGCHIP2_PHYPWRDN | CFGCHIP2_OTGPWRDN;
	writel(cfgchip2, CFGCHIP2);
}

/*
 * This function performs DA8xx platform specific initialization for usb0.
 */
int musb_platform_init(void)
{
	u32  revision;

	/* enable psc for usb2.0 */
	lpsc_on(0, 33);

#ifdef CONFIG_DA830_EVM
	/* Configure pinmux for DA830 USB0_DRVVBUS */
	writel((readl(PINMUX9) & (~DA830_USB0_DRVVBUS_MASK)), PINMUX9);
	writel((readl(PINMUX9) | DA830_USB0_DRVVBUS_EN), PINMUX9);
#endif

	/* enable usb vbus */
	enable_vbus();

	/* reset the controller */
	writel(0x1, (DA8XX_USB_OTG_BASE + DA8XX_USB_CTRL_REG));
	udelay(5000);

	/* start the on-chip usb phy and its pll */
	if (phy_on() == 0)
		return -1;

	/* returns zero if e.g. not clocked */
	revision = readl(DA8XX_USB_OTG_BASE + DA8XX_USB_VERSION_REG);
	if (revision == 0)
		return -1;

	/* disable all interrupts */
	writel((DA8XX_USB_USBINT_MASK | DA8XX_USB_TXINT_MASK
			| DA8XX_USB_RXINT_MASK),
		(DA8XX_USB_OTG_BASE + DA8XX_USB_INT_MASK_SET_REG));
	return 0;
}

/*
 * This function performs DA8xx platform specific deinitialization for usb0.
 */
void musb_platform_deinit(void)
{
	/* turn off the phy */
	phy_off();

	/* flush any interrupts */
	writel((DA8XX_USB_USBINT_MASK | DA8XX_USB_TXINT_MASK |
			DA8XX_USB_RXINT_MASK),
			(DA8XX_USB_OTG_BASE + DA8XX_USB_INT_MASK_CLR_REG));
	writel(0, (DA8XX_USB_OTG_BASE + DA8XX_USB_EOI_REG));
}

#endif /* CONFIG_USB_DA8XX */
