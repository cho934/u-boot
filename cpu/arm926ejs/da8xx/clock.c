/*
 * Copyright (C) 2008 Sekhar Nori, Texas Instruments, Inc.  <nsekhar@ti.com>
 * 
 * DA8xx clock module
 *
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
#include <asm/arch/hardware.h>

unsigned int sysdiv[9] = {
	PLL_DIV1, PLL_DIV2, PLL_DIV3, PLL_DIV4, PLL_DIV5, PLL_DIV6, 
	PLL_DIV7, PLL_DIV8, PLL_DIV9 };

int clk_get(unsigned int id)
{
	int pll_out = CFG_OSCIN_FREQ;
	int pre_div;
	int pllm;
	int post_div;
	volatile unsigned int pll_base;

	if(id == AUXCLK) 
		goto out;

	if ((id >> 16) == 1)
	pll_base = DAVINCI_PLL_CNTRL1_BASE;
	else
	pll_base = DAVINCI_PLL_CNTRL0_BASE;

	id &= 0xFF;

	pre_div = (REG(pll_base + PLL_PREDIV) & 0xff) + 1;
	pllm = REG(pll_base + PLL_PLLM) + 1;
	post_div = (REG(pll_base + PLL_POSTDIV) & 0xff) + 1;


	/* Lets keep this simple. Combining operations can result in 
	 * unexpected approximations
	 */
	pll_out /= pre_div;
	pll_out *= pllm;

	if(id == PLLM) 
		goto out;
	
	pll_out /= post_div;
	
	if(id == PLLC) 
		goto out;

	pll_out /= (REG(pll_base + sysdiv[id - 1]) & 0xff) + 1;

out:
	return pll_out;	
}
