/*
 * Low-level board setup code for TI DA8xx SoC based boards.
 *
 * Copyright (C) 2008 Texas Instruments, Inc <www.ti.com>
 * Sekhar Nori <nsekhar@ti.com>
 * 
 * Based on TI DaVinci low level init code. Original copyrights follow.
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Partially based on TI sources, original copyrights follow:
 */

/*
 * Board specific setup info
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * Modified for OMAP 1610 H2 board by Nishant Kamat, Jan 2004
 *
 * Modified for OMAP 5912 OSK board by Rishi Bhattacharya, Apr 2004
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * Modified for DV-EVM board by Rishi Bhattacharya, Apr 2005
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * Modified for DV-EVM board by Swaminathan S, Nov 2005
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

#include <config.h>
#include <asm/arch/hardware.h>

.globl	lowlevel_init
lowlevel_init:

	/*
	 * Call board-specific lowlevel init.
 	 * That MUST be present and THAT returns
	 * back to arch calling code with "mov pc, lr."
	 */
	b	dv_board_init
	nop

.ltorg

INTC_GLB_EN_ADDR:
    .word	INTC_GLB_EN  
INTC_EN_CLR0_ADDR:
	.word	INTC_EN_CLR0
INTC_HINT_EN_ADDR:
	.word	INTC_HINT_EN		

