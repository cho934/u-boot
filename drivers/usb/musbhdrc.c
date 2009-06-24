/*
 * Mentor USB Core host controller driver.
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

/* Include usb hcd code if usb support is included in u-boot */
#ifdef CONFIG_MUSB

#include <usb.h>
#include "musbhdrc.h"

/* extern functions */
extern int musb_platform_init(void);
extern void musb_platform_deinit(void);
extern inline void musb_writew(u32 offset, u16 value);
extern inline void musb_writeb(u32 offset, u8 value);
extern inline u16 musb_readw(u32 offset);
extern inline u8 musb_readb(u32 offset);

/* The controller driver polls for changes in the state. This defines a timeout
   for cases where the states do not change so the appropriate error can be
   returned. */
#define MUSB_USB_TIMEOUT 0x3FFFFF

/* This defines the endpoint number used for control transfers */
#define MUSB_CONTROL_EP  0

/* This defines the endpoint number used for bulk transfer */
#define MUSB_BULK_EP	 1

/* Determine the operating speed of MUSB core */
#define musb_ishighspeed() \
	((musb_readb(MGC_O_HDRC_POWER) & MGC_M_POWER_HSMODE) >> 4)

/* speed negotiated with the connected device */
static u8 musb_speed;

/*
 * This function configures all the endpoint FIFOs. Endpoint 1 is used for bulk
 * transfers and so the fifo size of EP1 Tx and Rx is set to 512 bytes. The
 * other endpoints 2,3 & 4 are configured for default fifo size of 64 bytes but
 * these endpoints are not used.
 */
void musb_configure_ep(void)
{
	/* Select Endpoint 1 for bulk transfer */
	musb_writeb(MGC_O_HDRC_INDEX, 1);

	/* Configure FIFO for endpoint 1 */
	musb_writeb(MGC_O_HDRC_TXFIFOSZ, 0x06);
	musb_writeb(MGC_O_HDRC_RXFIFOSZ, 0x06);
	musb_writew(MGC_O_HDRC_TXFIFOADD, 0x08);
	musb_writew(MGC_O_HDRC_RXFIFOADD, 0x48);

	/* Select Endpoint 2 for bulk transfer */
	musb_writeb(MGC_O_HDRC_INDEX, 2);

	/* Configure FIFO for endpoint 2 */
	musb_writeb(MGC_O_HDRC_TXFIFOSZ, 0x03);
	musb_writeb(MGC_O_HDRC_RXFIFOSZ, 0x03);
	musb_writew(MGC_O_HDRC_TXFIFOADD, 0x88);
	musb_writew(MGC_O_HDRC_RXFIFOADD, 0x90);

	/* Select Endpoint 3 for bulk transfer */
	musb_writeb(MGC_O_HDRC_INDEX, 3 );

	/* Configure FIFO for endpoint 2 */
	musb_writeb(MGC_O_HDRC_TXFIFOSZ, 0x03);
	musb_writeb(MGC_O_HDRC_RXFIFOSZ, 0x03);
	musb_writew(MGC_O_HDRC_TXFIFOADD, 0x98);
	musb_writew(MGC_O_HDRC_RXFIFOADD, 0xA0);

	/* Select Endpoint 3 for bulk transfer */
	musb_writeb(MGC_O_HDRC_INDEX, 4 );

	/* Configure FIFO for endpoint 2 */
	musb_writeb(MGC_O_HDRC_TXFIFOSZ, 0x03);
	musb_writeb(MGC_O_HDRC_RXFIFOSZ, 0x03);
	musb_writew(MGC_O_HDRC_TXFIFOADD, 0xA8);
	musb_writew(MGC_O_HDRC_RXFIFOADD, 0xB0);
}

/*
 * program the HDRC to start (enable interrupts, dma, etc.)
 */
void musb_start( void )
{
	u8 devctl;

	/* disable all interrupts */
	musb_writew(MGC_O_HDRC_INTRTXE, 0x0000);
	musb_writew(MGC_O_HDRC_INTRRXE, 0x0000);
	musb_writeb(MGC_O_HDRC_INTRUSBE, 0x00);
	musb_writeb(MGC_O_HDRC_TESTMODE, 0);

	/* put into basic highspeed mode and start session */
	musb_writeb(MGC_O_HDRC_POWER, (MGC_M_POWER_HSENAB));
	devctl = musb_readb(MGC_O_HDRC_DEVCTL);
	devctl |= MGC_M_DEVCTL_SESSION;
	musb_writeb(MGC_O_HDRC_DEVCTL, devctl);
}

/*
 * This function writes data to endpoint fifo
 */
static void write_fifo(u8 ep, u32 length, void *fifo_data)
{
	u32  address;
	u8	 *data = (u8*)fifo_data;

	/* select the endpoint index */
	musb_writeb(MGC_O_HDRC_INDEX, ep);
	address = MUSB_FIFO_OFFSET(ep)+0x20;

	/* write the data to the fifo */
	while(length) {
		musb_writeb(address, *data);
		data++;
		length--;
	}
}

/*
 * This function reads data from endpoint fifo
 */
static void read_fifo(u8 ep, u32 length, void *fifo_data)
{
	u32  address;
	u8	 *data = (u8*)fifo_data;

	/* select the endpoint index */
	musb_writeb(MGC_O_HDRC_INDEX, ep);
	address = MUSB_FIFO_OFFSET(ep)+0x20;

	/* read the data to the fifo */
	while(length) {
		*data = musb_readb(address);
		data++;
		length--;
	}
}

/*
 * This function performs all intializations required for setting up the
 * bulk endpoint.
 */
static void setup_bulk_ep(u8 bulkep)
{
	u16  csr;

	/* select bulk endpoint */
	musb_writeb(MGC_O_HDRC_INDEX, bulkep);

	/* clear the data toggle bit of bluk endpoint */
	csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR));
	csr = csr | MGC_M_TXCSR_CLRDATATOG;
	musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR), csr);

	csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR));
	csr = csr | MGC_M_RXCSR_CLRDATATOG;
	musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR), csr);

	/* also, flush the Tx and Rx FIFO of endpoint 1 */
	if ((musb_readb(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR)) & MGC_M_TXCSR_TXPKTRDY) == MGC_M_TXCSR_TXPKTRDY) {
		csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR));
		csr = csr | MGC_M_TXCSR_FLUSHFIFO;
		musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR), csr);
	}

	if ((musb_readb(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR)) & MGC_M_RXCSR_RXPKTRDY) == MGC_M_RXCSR_RXPKTRDY) {
		csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR));
		csr = csr | MGC_M_RXCSR_FLUSHFIFO;
		musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR), csr);
	}
}

/*
 * This function checks if RxStall has occured on the endpoint. If a RxStall has
 * occured, the RxStall is cleared and 1 is returned. If RxStall has not occured,
 * 0 is returned.
 */
static u8 check_stall(u8 ep, u8 dir_out)
{
	u16 csr;

	/* For endpoint 0 */
	if (ep == 0) {
		csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0));
		if ((csr & MGC_M_CSR0_H_RXSTALL ) == MGC_M_CSR0_H_RXSTALL) {
			csr = csr & ~(MGC_M_CSR0_H_RXSTALL|MGC_M_CSR0_H_STATUSPKT|MGC_M_CSR0_RXPKTRDY);
			musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), csr);
			return(1);
		}
	} else { /* For non-ep0 */
		if (dir_out == 1) { /* is it tx ep */
			csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR));
			if ((csr & MGC_M_TXCSR_H_RXSTALL) == MGC_M_TXCSR_H_RXSTALL) {
				csr = csr & ~(MGC_M_CSR0_H_RXSTALL|MGC_M_CSR0_H_STATUSPKT|MGC_M_CSR0_RXPKTRDY);
				musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), csr);
				return(1);
			}
		} else { /* is it rx ep */
			csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR));
			if ((csr & MGC_M_RXCSR_H_RXSTALL) == MGC_M_RXCSR_H_RXSTALL) {
				csr = csr & ~(MGC_M_CSR0_H_RXSTALL|MGC_M_CSR0_H_STATUSPKT|MGC_M_CSR0_RXPKTRDY);
				musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), csr);
				return(1);
			}
		}
	}

	/* There is no RxStall at the endpoint */
	return(0);
}


/*
 * waits until ep0 is ready.
 */
static int wait_until_ep0_ready(struct usb_device *dev, u32 bit_mask)
{
	u32 timeout;

	timeout = MUSB_USB_TIMEOUT;
	do {
		/* is there a stall */
		if (check_stall(MUSB_CONTROL_EP, 0)) {
			dev->status = USB_ST_STALLED;
			return( -2 );
		}

		switch(bit_mask) {
			case MGC_M_CSR0_TXPKTRDY:
				/* check if TXPKTRDY bit is cleared */
				if ((musb_readw( MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0)) & MGC_M_CSR0_TXPKTRDY) != MGC_M_CSR0_TXPKTRDY)
					return(0);
				break;

			case MGC_M_CSR0_RXPKTRDY:
				/* check if RXPKTRDY bit is set */
				if ((musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0)) & MGC_M_CSR0_RXPKTRDY) == MGC_M_CSR0_RXPKTRDY)
					return(0);
				break;

			case MGC_M_CSR0_H_REQPKT:
				/* check if the request has been sent */
				if ((musb_readw( MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0)) & MGC_M_CSR0_H_REQPKT) != MGC_M_CSR0_H_REQPKT)
					return(0);
				break;
		}
		timeout--;
	}
	while(timeout > 0);

	/* timed-out */
	dev->status = USB_ST_CRC_ERR;
	return(-1);
}

/*
 * This function performs the setup phase of the control transfer
 */
static int ctrlreq_setup_phase(struct usb_device *dev, struct devrequest *setup)
{
	int  result = -1;
	u16  csr;

	/* write the control request to ep0 fifo */
	write_fifo(MUSB_CONTROL_EP, sizeof(struct devrequest), (void*)setup);

	/* enable transfer of setup packet */
	csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0));
	csr = csr | (MGC_M_CSR0_TXPKTRDY|MGC_M_CSR0_H_SETUPPKT);
	musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), csr);

	/* wait until the setup packet is transmitted */
	result = wait_until_ep0_ready(dev, MGC_M_CSR0_TXPKTRDY);
	dev->act_len = 0;

	/* control transfer setup phase completes */
	return(result);
}

/*
 * This function handles the control transfer in data phase
 */
static int ctrlreq_in_data_phase(struct usb_device *dev, u32 len, void *buffer)
{
	u16  csr;
	u32  rxlen = 0;
	u32  nextlen = 0;
	u8	 maxpktsize = ( 1 << dev->maxpacketsize ) * 8;
	u8*  rxbuff = (u8*)buffer;
	u8	 rxedlength;
	int  result;

	do {
		/* Determine the next read length */
		nextlen = (( len-rxlen) > maxpktsize) ? maxpktsize:(len-rxlen);

		/* Set the ReqPkt bit */
		csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0));
		csr = csr | MGC_M_CSR0_H_REQPKT;
		musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), csr);

		result = wait_until_ep0_ready(dev, MGC_M_CSR0_RXPKTRDY);
		if (result < 0)
			return(result);

		/* Actual number of bytes received by usb */
		rxedlength = musb_readb(MGC_INDEXED_OFFSET(MGC_O_HDRC_COUNT0));

		/* Read the data from the RxFIFO */
		read_fifo(MUSB_CONTROL_EP, rxedlength, &rxbuff[rxlen]);

		/* Clear the RxPktRdy Bit */
		csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0));
		csr = csr & (~MGC_M_CSR0_RXPKTRDY);
		musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), csr);

		if (rxedlength != nextlen) {
			dev->act_len += rxedlength;
			break;
		}

		rxlen = rxlen + nextlen;
		dev->act_len = rxlen;
	}
	while(rxlen < len);

	/* done reading the data */
	return(0);
}

/*
 * This function handles the control transfer out data phase
 */
static int ctrlreq_out_data_phase(struct usb_device *dev, u32 len, void *buffer)
{
	u16  csr;
	u32  txlen = 0;
	u32  nextlen = 0;
	u8	 maxpktsize = ( 1 << dev->maxpacketsize ) * 8;
	u8*  txbuff = (u8*)buffer;
	int  result;

	do {
		/* Determine the next write length */
		nextlen = ((len-txlen) > maxpktsize) ? maxpktsize:(len-txlen);

		/* Load the data to send in FIFO */
		write_fifo(MUSB_CONTROL_EP, txlen, &txbuff[txlen]);

		/* Set TXPKTRDY bit */
		csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0));
		csr = csr | (MGC_M_CSR0_H_DIS_PING|MGC_M_CSR0_TXPKTRDY);
		musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), csr);

		result = wait_until_ep0_ready(dev, MGC_M_CSR0_TXPKTRDY);
		if ( result < 0 )
			return(result);

		txlen = txlen + nextlen;
		dev->act_len = txlen;
	}
	while(txlen < len);

	/* done writing the data */
	return(0);
}


/*
 * This function handles the control transfer out status phase
 */
static int ctrlreq_out_status_phase(struct usb_device *dev)
{
	u16 csr;
	int result;

	/* Set the StatusPkt bit */
	csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0));
	csr = csr | (MGC_M_CSR0_H_DIS_PING|MGC_M_CSR0_TXPKTRDY|MGC_M_CSR0_H_STATUSPKT);
	musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), csr);

	/* Wait until TXPKTRDY bit is cleared */
	result = wait_until_ep0_ready(dev, MGC_M_CSR0_TXPKTRDY);
	return(result);
}


/*
 * This function handles the control transfer in status phase
 */
static int ctrlreq_in_status_phase(struct usb_device *dev)
{
	u16  csr;
	int  result;

	/* Set the StatusPkt bit and ReqPkt bit */
	csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0));
	csr = csr | (MGC_M_CSR0_H_DIS_PING|MGC_M_CSR0_H_REQPKT|MGC_M_CSR0_H_STATUSPKT);
	musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), csr);

	result = wait_until_ep0_ready(dev, MGC_M_CSR0_H_REQPKT);
	if (result < 0)
		return(result);

	/* clear StatusPkt bit and RxPktRdy bit */
	csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0));
	csr = csr & (~((MGC_M_CSR0_RXPKTRDY|MGC_M_CSR0_H_STATUSPKT)));
	musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), csr);

	return(0);
}


/*
 * determines the speed of the device (High/Full/Slow)
 */
static u8 get_dev_speed(struct usb_device *dev)
{
	if (dev->high == 1)
		return(MGC_TYPE_SPEED_HIGH);
	else
		if (dev->slow == 1)
			return(MGC_TYPE_SPEED_LOW);
		else
			return(MGC_TYPE_SPEED_FULL);
}

/*
 * configure the hub address and the port address.
 */
static void config_hub_port(struct usb_device *dev, u8 ep)
{
	u8 chid;
	u8 hub;

	/* Find out the nearest parent which is high speed */
	while(dev->parent->parent != NULL) {
		if (get_dev_speed(dev->parent) !=  MGC_TYPE_SPEED_HIGH)
			dev = dev->parent;
		else
			break;
	}

	/* determine the port address at that hub */
	hub = dev->parent->devnum;
	for (chid = 0; chid < USB_MAXCHILDREN; chid++)
		if (dev->parent->children[chid] == dev)
			break;

	/* configure the hub address and the port address */
	musb_writeb(MGC_BUSCTL_OFFSET(ep, MGC_O_HDRC_TXHUBADDR), hub );
	musb_writeb(MGC_BUSCTL_OFFSET(ep, MGC_O_HDRC_TXHUBPORT), (chid+1) );
	musb_writeb(MGC_BUSCTL_OFFSET(ep, MGC_O_HDRC_RXHUBADDR), hub );
	musb_writeb(MGC_BUSCTL_OFFSET(ep, MGC_O_HDRC_RXHUBPORT), (chid+1) );
}

/*
 * do a control transfer
 */
int submit_control_msg( struct usb_device *dev , unsigned long pipe , void *buffer ,
						int len , struct devrequest *setup )
{
	int  devnum = usb_pipedevice(pipe);
	int  ep = usb_pipeendpoint(pipe);
	u16  csr;
	u16  wIntrTxE;
	u8	 devspeed;

	/* select control endpoint */
	musb_writeb(MGC_O_HDRC_INDEX, MUSB_CONTROL_EP);
	csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0));

	/* disable interrupt in case we flush */
	wIntrTxE = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_INTRTXE));
	musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_INTRTXE), (wIntrTxE & ~(1 << ep)));

	/* endpoint 0: just flush */
	musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), (csr|MGC_M_CSR0_FLUSHFIFO));
	musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_CSR0), (csr|MGC_M_CSR0_FLUSHFIFO));

	/* target addr and (for multipoint) hub addr/port */
	musb_writeb(MGC_BUSCTL_OFFSET(MUSB_CONTROL_EP, MGC_O_HDRC_TXFUNCADDR), devnum);
	musb_writeb(MGC_BUSCTL_OFFSET(MUSB_CONTROL_EP, MGC_O_HDRC_RXFUNCADDR), devnum);


	/* configure the hub address and the port number as required */
	if (( musb_ishighspeed()) && (dev->parent != NULL)) {
		devspeed = get_dev_speed(dev);
		if (devspeed != MGC_TYPE_SPEED_HIGH) {
			config_hub_port(dev, MUSB_CONTROL_EP);
			musb_writeb(MGC_INDEXED_OFFSET(MGC_O_HDRC_TYPE0), devspeed << 6);
		}
	} else {
		musb_writeb(MGC_INDEXED_OFFSET(MGC_O_HDRC_TYPE0),  musb_speed << 6);
		musb_writeb(MGC_BUSCTL_OFFSET(MUSB_CONTROL_EP, MGC_O_HDRC_TXHUBADDR), 0);
		musb_writeb(MGC_BUSCTL_OFFSET(MUSB_CONTROL_EP, MGC_O_HDRC_TXHUBPORT), 0);
		musb_writeb(MGC_BUSCTL_OFFSET(MUSB_CONTROL_EP, MGC_O_HDRC_RXHUBADDR), 0);
		musb_writeb(MGC_BUSCTL_OFFSET(MUSB_CONTROL_EP, MGC_O_HDRC_RXHUBPORT), 0);
	}

	/* Control transfer setup phase */
	if (ctrlreq_setup_phase(dev, setup) < 0)
		return(-1);

	if ((setup->request == 0x06) || /* GET_DESCRIPTOR	 */
		(setup->request == 0x08) || /* GET_CONFIGURATION */
		(setup->request == 0x0A) || /* GET_INTERFACE	 */
		(setup->request == 0x00)) { /* GET_STATUS		 */
		/* control transfer in-data-phase */
		if (ctrlreq_in_data_phase(dev, len, buffer) < 0)
			return(-1);

		/* control transfer out-status-phase */
		if (ctrlreq_out_status_phase(dev) < 0)
			return(-1);
	} else {
		if ((setup->request == 0x05) || /* SET_ADDRESS			  */
			(setup->request == 0x09) || /* SET_CONFIGURATION	  */
			(setup->request == 0x03) || /* SET_FEATURE			  */
			(setup->request == 0x03) || /* SET_FEATURE			  */
			(setup->request == 0x0B) || /* SET_INTERFACE		  */
			(setup->request == 0x01) || /* CLEAR_FEATURE		  */
			(setup->request == 0xFF)) { /* USB Mass Stroage Reset */

			/* control transfer in status phase */
			if (ctrlreq_in_status_phase(dev) < 0)
				return(-1);
		} else {
			if (setup->request == 0x07) {	/* SET_DESCRIPTOR */
				/* control transfer out data phase */
				if (ctrlreq_out_data_phase(dev, len, buffer) < 0)
					return(-1);

				/* control transfer in status phase */
				if ( ctrlreq_in_status_phase(dev) < 0 )
					return(-1);
			} else {
				/* unhandled control transfer */
				return(-1);
			}
		}
	}

	/* end of control transfer */
	dev->status = 0;
	dev->act_len = len;
	return len;
}

/*
 * do a bulk transfer
 */
int submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer, int len)
{
	int dir_out = usb_pipeout(pipe);
	int ep = usb_pipeendpoint(pipe);
	int devnum = usb_pipedevice(pipe);
	u8	type;
	u16 csr;
	u32 txLen = 0;
	u32 nextLen = 0;
	u8	devspeed;
	u32 timeout;

	/* select bulk endpoint */
	musb_writeb(MGC_O_HDRC_INDEX, MUSB_BULK_EP);

	/* write the address of the device */
	if ( dir_out == 1 )
		musb_writeb(MGC_BUSCTL_OFFSET(MUSB_BULK_EP, MGC_O_HDRC_TXFUNCADDR), devnum);
	else
		musb_writeb(MGC_BUSCTL_OFFSET(MUSB_BULK_EP, MGC_O_HDRC_RXFUNCADDR), devnum);

	/* configure the hub address and the port number as required */
	if ((musb_ishighspeed()) && (dev->parent != NULL)) {
		devspeed = get_dev_speed(dev);
		if (devspeed != MGC_TYPE_SPEED_HIGH) {
			/* MUSB is in high speed and the destination device is full speed device.
			   So configure the hub address and port address registers. */
			config_hub_port(dev, MUSB_BULK_EP);
		}
	} else {
		if (dir_out == 1) {
			musb_writeb(MGC_BUSCTL_OFFSET(MUSB_BULK_EP, MGC_O_HDRC_TXHUBADDR), 0);
			musb_writeb(MGC_BUSCTL_OFFSET(MUSB_BULK_EP, MGC_O_HDRC_TXHUBPORT), 0);
		} else {
			musb_writeb(MGC_BUSCTL_OFFSET(MUSB_BULK_EP, MGC_O_HDRC_RXHUBADDR), 0);
			musb_writeb(MGC_BUSCTL_OFFSET(MUSB_BULK_EP, MGC_O_HDRC_RXHUBPORT), 0);
		}
		devspeed = musb_speed;
	}

	if (dir_out == 1) { /* bulk-out transfer */
		/* Write the old data toggle value */
		if (usb_gettoggle(dev, ep, dir_out) == 0) {
			csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR));
			csr = MGC_M_TXCSR_CLRDATATOG;
			musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR), csr);
		} else {
			csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR));
			csr = csr | MGC_M_TXCSR_H_WR_DATATOGGLE;
			musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR), csr);

			csr = csr | (usb_gettoggle(dev, ep, dir_out)<<8);
			musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR), csr);
		}

		/* Program the TxType register */
		type = (devspeed << MGC_S_TYPE_SPEED) |
			   (0x2 << MGC_S_TYPE_PROTO) |
			   (ep & 0xF);
		musb_writeb(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXTYPE), type);

		/* Write maximum packet size to the TxMaxp register */
		musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXMAXP), dev->epmaxpacketout[ep]);

		while(txLen < len) {
			nextLen = ((len-txLen) < dev->epmaxpacketout[ep]) ? (len-txLen):dev->epmaxpacketout[ep];

			/* Write the data to the FIFO */
			write_fifo(1, nextLen, (void*)(((u8*)buffer)+txLen));

			/* Set the TxPktRdy bit */
			csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR));
			csr = csr | MGC_M_TXCSR_TXPKTRDY;
			musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR), csr);

			/* Wait until the TxPktRdy bit is cleared */
			timeout = MUSB_USB_TIMEOUT;
			do {
				csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR));
				if ((csr & MGC_M_TXCSR_H_RXSTALL) == MGC_M_TXCSR_H_RXSTALL) {
					musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR), csr & (~(MGC_M_TXCSR_H_RXSTALL)));

					/* Keep a copy of the data toggle bit */
					usb_settoggle(dev, ep, dir_out, (musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR)) >> 8) & 0x01);

					dev->status = USB_ST_STALLED;
					dev->act_len = txLen;
					return(0);
				}

				if ((csr & MGC_M_TXCSR_H_ERROR) == MGC_M_TXCSR_H_ERROR) {
					/* keep a copy of the data toggle bit */
					usb_settoggle(dev, ep, dir_out, (musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR)) >> 8) & 0x01);
					dev->status = USB_ST_CRC_ERR;
					dev->act_len = txLen;
					return(0);
				}

				/* maintain a timeout */
				if (timeout-- == 0) {
					/* keep a copy of the data toggle bit */
					usb_settoggle(dev, ep, dir_out, (musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR)) >> 8) & 0x01);
					dev->status = USB_ST_CRC_ERR;
					dev->act_len = txLen;

					/* clear the TxPktRdy bit */
					csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR));
					csr = csr & (~MGC_M_TXCSR_TXPKTRDY);
					musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR), csr);
					return(0);
				}
			}
			while((csr & MGC_M_TXCSR_TXPKTRDY) == MGC_M_TXCSR_TXPKTRDY);

			txLen = txLen + nextLen;
			#if 0
			if ((txLen == len) && (nextLen == dev->epmaxpacketout[ep])) {
				/* Set the TxPktRdy bit */
				csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR));
				csr = csr | MGC_M_TXCSR_TXPKTRDY;
				musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR), csr);

				/* Wait until the TxPktRdy bit is cleared */
				while(( musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR) & MGC_M_TXCSR_TXPKTRDY)) == MGC_M_TXCSR_TXPKTRDY);
			}
			#endif
		}

		/* Keep a copy of the data toggle bit */
		usb_settoggle(dev, ep, dir_out, (musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_TXCSR)) >> 8) & 0x01);
	} else { /* bulk-in transfer */
		/* Write the old data toggle value */
		if (usb_gettoggle(dev, ep, dir_out) == 0) {
			csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR));
			csr = MGC_M_RXCSR_CLRDATATOG;
			musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR), csr);
		} else {
			csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR));
			csr = csr | MGC_M_RXCSR_H_WR_DATATOGGLE;
			musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR), csr);

			csr = csr | (usb_gettoggle(dev, ep, dir_out)<<9);
			musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR), csr);
		}

		/* Program the RxType register */
		type = (devspeed << MGC_S_TYPE_SPEED) |
			   (0x2 << MGC_S_TYPE_PROTO) |
			   (ep & 0xF);
		musb_writeb(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXTYPE), type);

		/* Write the maximum packet size to the RxMaxp register */
		musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXMAXP), dev->epmaxpacketin[ep]);

		while(txLen < len) {
			nextLen = ((len-txLen) < dev->epmaxpacketin[ep]) ? (len-txLen):dev->epmaxpacketin[ep];

			/* Set the ReqPkt bit */
			csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR));
			csr = csr | MGC_M_RXCSR_H_REQPKT;
			musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR), csr);

			/* Wait until the RxPktRdy bit is cleared */
			timeout = MUSB_USB_TIMEOUT;
			do {
				csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR));
				if ((csr & MGC_M_RXCSR_H_RXSTALL) == MGC_M_RXCSR_H_RXSTALL) {
					/* Keep a copy of the data toggle bit */
					usb_settoggle(dev, ep, dir_out, (musb_readw( MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR) ) >> 9) & 0x01);

					musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR), csr & (~(MGC_M_RXCSR_H_RXSTALL)));
					dev->status = USB_ST_STALLED;
					dev->act_len = txLen;
					return(0);
				}

				if ((csr & MGC_M_RXCSR_H_ERROR) == MGC_M_RXCSR_H_ERROR) {
					/* Keep a copy of the data toggle bit */
					usb_settoggle(dev, ep, dir_out, (musb_readw( MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR) ) >> 9) & 0x01);
					dev->status = USB_ST_CRC_ERR;
					dev->act_len = txLen;
					return(0);
				}

				if (timeout-- == 0) {
					/* Keep a copy of the data toggle bit */
					usb_settoggle(dev, ep, dir_out, (musb_readw( MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR) ) >> 9) & 0x01);
					dev->status = USB_ST_CRC_ERR;
					dev->act_len = txLen;

					/* clear the ReqPkt bit */
					csr = musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR));
					csr = csr & (~MGC_M_RXCSR_H_REQPKT);
					musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR), csr);
					return(0);
				}
			}
			while((csr & MGC_M_RXCSR_RXPKTRDY) != MGC_M_RXCSR_RXPKTRDY);

			/* Read the data from the FIFO */
			read_fifo(1, nextLen, (void*)(((u8*)buffer) + txLen));

			/* Clear the RxPktRdy bit */
			csr =  musb_readw(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR));
			csr = csr & ~(MGC_M_RXCSR_RXPKTRDY);
			musb_writew(MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR), csr);

			txLen = txLen + nextLen;
		}

		/* Keep a copy of the data toggle bit */
		usb_settoggle(dev, ep, dir_out, (musb_readw( MGC_INDEXED_OFFSET(MGC_O_HDRC_RXCSR) ) >> 9) & 0x01);
	}

	/* bulk transfer is complete */
	dev->status = 0;
	dev->act_len = len;
	return 0;
}

/*
 * This function initializes the usb controller module.
 */
int usb_lowlevel_init(void)
{
	u8	 power;
	u32  timeout;

	if (musb_platform_init() == -1)
	   return(-1);

	/* Configure all the endpoint FIFO's and start usb controller */
	musb_configure_ep();
	musb_start();

	/* Wait until musb is enabled in host mode with a timeout. No hot
	   plug support. So there should be a usb device connected. */
	timeout = MUSB_USB_TIMEOUT*0x10;
	do {
		/* wait until the musb core moves into host mode */
		if (( musb_readb(MGC_O_HDRC_DEVCTL) & MGC_M_DEVCTL_HM) == MGC_M_DEVCTL_HM)
			break;

		/* maintain a timeout */
		timeout--;
	}
	while( timeout > 0 );

	/* if musb core is not in host mode, then return */
	if (timeout == 0)
		return( -1 );

	/* start usb bus reset */
	power = musb_readb(MGC_O_HDRC_POWER);
	power = power | MGC_M_POWER_RESET;
	musb_writeb(MGC_O_HDRC_POWER, power);

	/* After initiating a usb reset, wait for about 20ms to 30ms */
	udelay(30000);

	/* stop usb bus reset */
	power = musb_readb(MGC_O_HDRC_POWER);
	power = power & (~MGC_M_POWER_RESET);
	musb_writeb(MGC_O_HDRC_POWER, power);

	/* Determine if the connected device is a high/full/low speed device */
	if ((musb_readb(MGC_O_HDRC_POWER) & MGC_M_POWER_HSMODE) == MGC_M_POWER_HSMODE) {
		/* High speed device is connected */
		musb_speed = MGC_TYPE_SPEED_HIGH;
	} else {
		if ((musb_readb(MGC_O_HDRC_DEVCTL) & MGC_M_DEVCTL_FSDEV) == MGC_M_DEVCTL_FSDEV) {
			/* Full speed device is connected */
			musb_speed = MGC_TYPE_SPEED_FULL;
		} else {
			/* Low speed device is connected */
			musb_speed = MGC_TYPE_SPEED_LOW;
		}
	}

	/* setup the bulk endpoint */
	setup_bulk_ep(MUSB_BULK_EP);

	/* usb low level intialization is complete */
	return(0);
}

/*
 * This function stops the operation of the davinci usb module.
 */
int usb_lowlevel_stop(void)
{
	/* Reset the USB module */
	musb_platform_deinit();
	musb_writeb(MGC_O_HDRC_DEVCTL, 0);

	/* All done */
	return 0;
}

/*
 * This function supports usb interrupt transfers. Currently, usb interrupt transfers
 * are not supported.
 */
int submit_int_msg( struct usb_device *dev, unsigned long pipe ,
					void *buffer , int len, int interval )
{
	return(-1);
}

#endif	/* CONFIG_MUSB */



