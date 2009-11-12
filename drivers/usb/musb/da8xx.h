/*
 * da8xx.h -- TI's DA8xx platform specific usb wrapper definitions.
 *
 * Author: Ajay Kumar Gupta <ajay.gupta@ti.com>
 *
 * Based on drivers/usb/musb/davinci.h
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
#ifndef __DA8XX_MUSB_H__
#define __DA8XX_MUSB_H__

#include "musb_core.h"

/* extern functions */
extern void lpsc_on(unsigned domain, unsigned int id);

/* Base address of da8xx usb0 wrapper */
#define DA8XX_USB_OTG_BASE  0x01E00000

/* Base address of da8xx musb core */
#define DA8XX_USB_OTG_CORE_BASE (DA8XX_USB_OTG_BASE + 0x400)

/* Timeout for DA8xx usb module */
#define DA8XX_USB_OTG_TIMEOUT 0x3FFFFFF

/* USB OTG wrapper register offsets */
#define DA8XX_USB_VERSION_REG			0x00
#define DA8XX_USB_CTRL_REG			0x04
#define DA8XX_USB_INT_MASK_SET_REG		0x30
#define DA8XX_USB_INT_MASK_CLR_REG		0x34
#define DA8XX_USB_EOI_REG			0x3c

#define DA8XX_USB_TX_ENDPTS_MASK  0x1f		  /* ep0 + 4 tx */
#define DA8XX_USB_RX_ENDPTS_MASK  0x1e		  /* 4 rx */
#define DA8XX_USB_TXINT_SHIFT	  0
#define DA8XX_USB_RXINT_SHIFT	  8

#define DA8XX_USB_USBINT_MASK	  0x01ff0000  /* 8 Mentor, DRVVBUS */
#define DA8XX_USB_TXINT_MASK \
		(DA8XX_USB_TX_ENDPTS_MASK << DA8XX_USB_TXINT_SHIFT)
#define DA8XX_USB_RXINT_MASK \
		(DA8XX_USB_RX_ENDPTS_MASK << DA8XX_USB_RXINT_SHIFT)

/* DA8xx CFGCHIP2 (USB 2.0 PHY Control) register bits */
#define CFGCHIP2_PHYCLKGD	(1 << 17)
#define CFGCHIP2_VBUSSENSE	(1 << 16)
#define CFGCHIP2_RESET		(1 << 15)
#define CFGCHIP2_OTGMODE	(3 << 13)
#define CFGCHIP2_NO_OVERRIDE	(0 << 13)
#define CFGCHIP2_FORCE_HOST	(1 << 13)
#define CFGCHIP2_FORCE_DEVICE 	(2 << 13)
#define CFGCHIP2_FORCE_HOST_VBUS_LOW (3 << 13)
#define CFGCHIP2_USB1PHYCLKMUX	(1 << 12)
#define CFGCHIP2_USB2PHYCLKMUX	(1 << 11)
#define CFGCHIP2_PHYPWRDN	(1 << 10)
#define CFGCHIP2_OTGPWRDN	(1 << 9)
#define CFGCHIP2_DATPOL 	(1 << 8)
#define CFGCHIP2_USB1SUSPENDM	(1 << 7)
#define CFGCHIP2_PHY_PLLON	(1 << 6)	/* override PLL suspend */
#define CFGCHIP2_SESENDEN	(1 << 5)	/* Vsess_end comparator */
#define CFGCHIP2_VBDTCTEN	(1 << 4)	/* Vbus comparator */
#define CFGCHIP2_REFFREQ	(0xf << 0)
#define CFGCHIP2_REFFREQ_12MHZ	(1 << 0)
#define CFGCHIP2_REFFREQ_24MHZ	(2 << 0)
#define CFGCHIP2_REFFREQ_48MHZ	(3 << 0)

#define USB_VBUS_GPIO		(1 << 15)

#define DA830_USB0_DRVVBUS_MASK	(0xf << 4)
#define DA830_USB0_DRVVBUS_EN	(1 << 4)
#endif	/* __DA8XX_MUSB_H__ */

