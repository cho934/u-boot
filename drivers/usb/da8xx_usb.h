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

#ifndef  __DA8XX_MUSB_H__
#define  __DA8XX_MUSB_H__

#include "musbhdrc.h"

/* Base address of da8xx usb0 wrapper */
#define DA8XX_USB0_BASE  0x01E00000

/* Base address of da8xx musb core */
#define MENTOR_USB0_BASE (DA8XX_USB0_BASE+0x400)

/* For now include usb OTG module registers here */
#define DA8XX_USB_VERSION_REG			0x00
#define DA8XX_USB_CTRL_REG				0x04
#define DA8XX_USB_STAT_REG				0x08
#define DA8XX_MODE_TGCR_REG 			0x10 /* Mode reg RNDIS */
#define DA8XX_AUTOREQ_REG				0x14
#define DA8XX_SRP_FIXTIME_REG			0x18 /* SRP Fixtime reg */
#define DA8XX_TEARDOWN_REG				0x1C /* TearDown Register*/
#define DA8XX_USB_INT_SOURCE_REG		0x20
#define DA8XX_USB_INT_SET_REG			0x24
#define DA8XX_USB_INT_SRC_CLR_REG		0x28
#define DA8XX_USB_INT_MASK_REG			0x2c
#define DA8XX_USB_INT_MASK_SET_REG		0x30
#define DA8XX_USB_INT_MASK_CLR_REG		0x34
#define DA8XX_USB_INT_SRC_MASKED_REG	0x38
#define DA8XX_USB_EOI_REG				0x3c
#define DA8XX_USB_EOI_INTVEC			0x40
#define DA8XX_GRNDIS_EP1SIZE_REG		0x50
#define DA8XX_GRNDIS_EP2SIZE_REG		0x54
#define DA8XX_GRNDIS_EP3SIZE_REG		0x58
#define DA8XX_GRNDIS_EP4SIZE_REG		0x5C


#define DA8XX_USB_TX_ENDPTS_MASK  0x1f		  /* ep0 + 4 tx */
#define DA8XX_USB_RX_ENDPTS_MASK  0x1e		  /* 4 rx */

#define DA8XX_USB_USBINT_SHIFT	  16
#define DA8XX_USB_TXINT_SHIFT	  0
#define DA8XX_USB_RXINT_SHIFT	  8

#define DA8XX_INTR_DRVVBUS		  0x0100

#define DA8XX_USB_USBINT_MASK	  0x01ff0000  /* 8 Mentor, DRVVBUS */
#define DA8XX_USB_TXINT_MASK \
		(DA8XX_USB_TX_ENDPTS_MASK << DA8XX_USB_TXINT_SHIFT)
#define DA8XX_USB_RXINT_MASK \
		(DA8XX_USB_RX_ENDPTS_MASK << DA8XX_USB_RXINT_SHIFT)

#define MGC_BUSCTL_OFFSET(_bEnd, _bOffset) \
		(0x80 + (8*(_bEnd)) + (_bOffset))

#endif	/* __DA8XX_MUSB_H__ */

