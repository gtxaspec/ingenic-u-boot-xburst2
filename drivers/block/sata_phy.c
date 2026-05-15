/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
 * Terry Lv <r65388@freescale.com>
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
 * Foundation, Inc.
 *
 */

#include <common.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/cache.h>
#include <linux/bitops.h>
#include <asm/arch-a1/clk.h>

#include "sata_phy.h"

unsigned int sata_phy_reset(unsigned int port_num, unsigned char rst_n)
{
	unsigned int reg = (port_num == 0)?P0PHYCR_ADDR:(P0PHYCR_ADDR+0x80);
	if(0 == rst_n) {
		unsigned int data = 0;

		data = *(volatile unsigned int *)reg;
		data &= ~PnPHYCR_PHY_RESET_N;
		data &= ~PnPHYCR_POWER_RESET_N;
		*(volatile unsigned int *)reg = data;
	} else {
		unsigned int data = 0;

		data = *(volatile unsigned int *)reg;
		udelay(2);
		data |= PnPHYCR_POWER_RESET_N;
		*(volatile unsigned int *)reg = data;
		udelay(2);
		data |= PnPHYCR_PHY_RESET_N;
		*(volatile unsigned int *)reg = data;
	}
	return 0;
}

void dump_phy_status(void)
{
    printf("dump sata phy status:\n");
    printf("0x130d0178 = 0x%08x\n", *(volatile unsigned int *)0xb30d0178);
    printf("0x130d017c = 0x%08x\n", *(volatile unsigned int *)0xb30d017c);
    printf("0x130d01f8 = 0x%08x\n", *(volatile unsigned int *)0xb30d01f8);
    printf("0x130d01fc = 0x%08x\n", *(volatile unsigned int *)0xb30d01fc);
    printf("0x130d00d0 = 0x%08x\n", *(volatile unsigned int *)0xb30d00d0);
    printf("0x130d00d4 = 0x%08x\n", *(volatile unsigned int *)0xb30d00d4);
}

#define SATA_GEN 2
int ahci_ingenic_phy_init()
{
    int err;
    unsigned int data, clkgr1, srbc;

/*CPM config
 *CLKGR1:bit11 open
 *SRBS0:bit8 reset
 * */
	//clkgr1 = *(volatile unsigned int *)CPM_CLKGR1_GATE;
    ////printf("Before config,clkgr1 is 0x%x\n", clkgr1);
    ////printf("Before config,SATA_GATE is 0x%x\n", (clkgr1 & CLKGR1_SATA));
	//if ((clkgr1 & CLKGR1_SATA) ) {
	//	clkgr1 &= ~CLKGR1_SATA;
	//	*(volatile unsigned int *)CPM_CLKGR1_GATE = clkgr1;
    //	//printf("After config 0xb0000038,clkgr1 is 0x%x\n", *(volatile unsigned int *)0xb0000038);
	//}
    //printf("After config,clkgr1 is 0x%x\n", *(volatile unsigned int *)0xb0000038);

	srbc = readl(CPM_SRBC_0);
    srbc |= SRBC_SATA_SR;
	writel(srbc,CPM_SRBC_0);
    srbc &= ~SRBC_SATA_SR;
	writel(srbc,CPM_SRBC_0);

    /* sata inno phy  clock register && Rx squelch&offset config value*/
	writel(0x1284,P0_RXSQUL);
	data = readl(P0_RXOFFSETA);
	writel(data&(~(0xff)),P0_RXOFFSETA);
    data = readl(P0_RXOFFSETB);
    data &= (~(0x1<<16));
    data |= (0x1<<17);
	writel(data,P0_RXOFFSETB);

    data = readl(P0_RXOFFSETC);
    data &= (~(0x3<<30));
    data |= (0x1<<29);
	writel(data,P0_RXOFFSETC);

    /*Eye Diagran correction for port0 */
    writel(PHY_TX_MAIN_LEG_BYPS_VALUE,P0PHY_TX_MAIN_LEG_BYPS);
    writel(PHY_TX_MAIN_LEG_VAL_VALUE,P0PHY_TX_MAIN_LEG_VAL);
    writel(PHY_RX_CTLE_EQN_BYPS_VALUE,P0PHY_RX_CTLE_EQN_BYPS);
    writel(PHY_RX_CTLE_EQN_VALUE,P0PHY_RX_CTLE_EQN);
    /* Set port0 Driver strength */
    writel(PHY_DRV_STRENGTH_VALUE,P0PHY_DRV_STRENGTH);

    /*set for SATA Controller PnPHYCR(for n=0; n < AHSATA_NUM_PORTS)*/
    //P0PHYCR
    data = readl(P0PHYCR_ADDR);
    //printf("Before config,P0PHYCR is 0x%04x\n",data);
    //printf("Before config 0xb30d0178, P0PHYCR is 0x%04x\n", *(volatile unsigned int *)0xb30d0178);
    //printf("Before config 0x130d0178, P0PHYCR is 0x%04x\n", *(volatile unsigned int *)0x130d0178);
#if (SATA_GEN == 2)
    data |= PnPHYCR_HOST_SEL|PnPHYCR_SPDSEL|PnPHYCR_MSB;//bit8:sata gen 1:0;sata gen 2:1,bit5:debug
#else
    data |= (PnPHYCR_HOST_SEL&(~PnPHYCR_SPDSEL))|PnPHYCR_MSB;//bit8:sata gen 1:0;sata gen 2:1,bit5:debug
#endif
    writel(data,P0PHYCR_ADDR);
    //printf("After config 0xb30d0178, P0PHYCR is 0x%04x\n", *(volatile unsigned int *)0xb30d0178);
    //printf("After config 0x130d0178, P0PHYCR is 0x%04x\n", *(volatile unsigned int *)0x130d0178);

    udelay(2);
    data |= PnPHYCR_POWER_RESET_N;
    writel(data,P0PHYCR_ADDR);

    udelay(2);
    data |= PnPHYCR_PHY_RESET_N;
    writel(data,P0PHYCR_ADDR);

    /*AHCI port register has same offset address,
    * same register config for port0&port1
    */

    writel(0x1284,P0_RXSQUL+0x10000);

	data = readl(P0_RXOFFSETA+0x10000);
    writel(data&(~(0xff)),P0_RXOFFSETA+0x10000);

    data = readl(P0_RXOFFSETB+0x10000);
    data &= (~(0x1<<16));
    data |= (0x1<<17);
    writel(data,P0_RXOFFSETB+0x10000);

    data = readl(P0_RXOFFSETC+0x10000);
    data &= (~(0x3<<30));
    data |= (0x1<<29);
    writel(data,P0_RXOFFSETC+0x10000);

    /*Eye Diagran correction for port1 */
    writel(PHY_TX_MAIN_LEG_BYPS_VALUE,P1PHY_TX_MAIN_LEG_BYPS);
    writel(PHY_TX_MAIN_LEG_VAL_VALUE,P1PHY_TX_MAIN_LEG_VAL);
    writel(PHY_RX_CTLE_EQN_BYPS_VALUE,P1PHY_RX_CTLE_EQN_BYPS);
    writel(PHY_RX_CTLE_EQN_VALUE,P1PHY_RX_CTLE_EQN);
    /* Set port1 Driver strength */
    writel(PHY_DRV_STRENGTH_VALUE,P1PHY_DRV_STRENGTH);

    //P1PHYCR = p0PHYCR + 0x80
    data = readl(P0PHYCR_ADDR+0x80);
#if (SATA_GEN == 2)
    data |= PnPHYCR_HOST_SEL|PnPHYCR_SPDSEL|PnPHYCR_MSB;//bit8:sata gen 1:0;sata gen 2:1,bit5:debug
#else
    data |= (PnPHYCR_HOST_SEL&(~PnPHYCR_SPDSEL))|PnPHYCR_MSB;//bit8:sata gen 1:0;sata gen 2:1,bit5:debug
#endif
    writel(data,P0PHYCR_ADDR+0x80);

    udelay(2);
    data |= PnPHYCR_POWER_RESET_N;
    writel(data,P0PHYCR_ADDR+0x80);

    udelay(2);
    data |= PnPHYCR_PHY_RESET_N;
    writel(data,P0PHYCR_ADDR+0x80);

    return 0;
}













