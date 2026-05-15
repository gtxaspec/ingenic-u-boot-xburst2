/*
 * DDR driver for Synopsys DWC DDR PHY.
 * Used by Jz4775, JZ4780...
 *
 * Copyright (C) 2013 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <ykli@ingenic.cn>
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
#include <common.h>
#include <linux/string.h>
#include <malloc.h>
#include <ddr/ddr_common.h>
#ifdef CONFIG_A1ALL
#include <generated/ddr_reg_values_a1all.h>
struct a1_ddr_params *pddr_params;
#else
#include <generated/ddr_reg_values.h>
#endif
#include <asm/cacheops.h>
#include <asm/dma-default.h>

#include <asm/io.h>
#include <asm/arch/clk.h>
#include "ddr_debug.h"

#define ddr_hang() do{						\
		printf("%s %d\n",__FUNCTION__,__LINE__);	\
		hang();						\
	}while(0)

/*#define DEBUG*/
/*#define DEBUG_READ_WRITE */
//#define CONFIG_DDR_DEBUG 1
//#define DDR_EFUSE_DEBUG

DECLARE_GLOBAL_DATA_PTR;

ddr3_param_t a1n_ddr3_param = {
    .version = "A1N@V20230825a",
    .params = {
        [odt_pd] = 1,
        [odt_pu] = 0,
        [drvcmd_pd] = 0xe,
        [drvcmd_pu] = 0xe,
        [drvcmdck_pd] = 0xe,
        [drvcmdck_pu] = 0xe,
        [drvalah_pd] = 0x14,
        [drvalah_pu] = 0x14,
        [drvblbh_pd] = 0x14,
        [drvblbh_pu] = 0x14,
        [dqA] = 0x10,
        [dqB] = 7,
        [vref] = 0x80,
        [kgd_odt] = 1,//001
        [kgd_ds] = 1,//01
        [dqsA] = 0x20,
        [dqsB] = 0x20
    }
};

ddr3_param_t a1x_ddr3_param= {
    .version = "A1X@V20230825a",
    .params = {
        [odt_pd] = 5,
        [odt_pu] = 11,
        [drvcmd_pd] = 0x1e,
        [drvcmd_pu] = 0x1e,
        [drvcmdck_pd] = 0x1e,
        [drvcmdck_pu] = 0x1e,
        [drvalah_pd] = 0xf,
        [drvalah_pu] = 0xf,
        [drvblbh_pd] = 0xf,
        [drvblbh_pu] = 0xf,
        [dqA] = 0xf,
        [dqB] = 0xd,
        [vref] = 0xa0,//160
        [kgd_odt] = 2,
        [kgd_ds] = 0,
        [dqsA] = 0x21,
        [dqsB] = 0x21
    }
};

ddr3_param_t a1a_ddr3_param = {
    .version = "A1A@V20230825a",
    .params = {
        [odt_pd] = 7,
        [odt_pu] = 12,
        [drvcmd_pd] = 0xe,
        [drvcmd_pu] = 0xe,
        [drvcmdck_pd] = 0xe,
        [drvcmdck_pu] = 0xe,
        [drvalah_pd] = 0x14,
        [drvalah_pu] = 0x14,
        [drvblbh_pd] = 0x14,
        [drvblbh_pu] = 0x14,
        [dqA] = 0x14,
        [dqB] = 0x14,
        [vref] = 0x96,//150
        [kgd_odt] = 1,
        [kgd_ds] = 1,
        [dqsA] = 0x23,
        [dqsB] = 0x23
    }
};

ddr3_param_t a1nt_ddr3_param = {
    .version = "A1NT@V20230825a",
    .params = {
        [odt_pd] = 3,
        [odt_pu] = 1,
        [drvcmd_pd] = 0xe,
        [drvcmd_pu] = 0xe,
        [drvcmdck_pd] = 0xe,
        [drvcmdck_pu] = 0xe,
        [drvalah_pd] = 0x14,
        [drvalah_pu] = 0x14,
        [drvblbh_pd] = 0x14,
        [drvblbh_pu] = 0x14,
        [dqA] = 0x10,
        [dqB] = 0xf,
        [vref] = 0x80,
        [kgd_odt] = 2,
        [kgd_ds] = 1,
        [dqsA] = 0x20,
        [dqsB] = 0x21
    }
};

ddr3_param_t a1l_ddr3_param = {
    .version = "A1L@V20230825a",
    .params = {
        [odt_pd] = 1,
        [odt_pu] = 1,
        [drvcmd_pd] = 0xe,
        [drvcmd_pu] = 0xe,
        [drvcmdck_pd] = 0xe,
        [drvcmdck_pu] = 0xe,
        [drvalah_pd] = 0x14,
        [drvalah_pu] = 0x14,
        [drvblbh_pd] = 0x14,
        [drvblbh_pu] = 0x14,
        [dqA] = 0xe,
        [dqB] = 0xe,
        [vref] = 0x80,
        [kgd_odt] = 1,
        [kgd_ds] = 1,
        [dqsA] = 0x1c,
        [dqsB] = 0x1c
    }
};

static unsigned short get_chip_type_from_efuse(void)
{
	unsigned int data = 0;
	unsigned short type = 0;
	data = *(volatile unsigned int *)(0xb3540250);
	type = data >> 16;
	return type;
}

#ifdef CONFIG_A1ALL


void  get_a1chip_ddr_config(void)
{
	unsigned short chip;
	chip = get_chip_type_from_efuse();

	switch (chip) {
		case 0x1111:
			pddr_params = &a1n_ddr_para;
			printf("chip type is A1N\n");
			break;

		case 0x5555:
			pddr_params = &a1nt_ddr_para;
			printf("chip type is A1NT\n");
			break;

		case 0x2222:
			pddr_params = &a1x_ddr_para;
			printf("chip type is A1X\n");
			break;

		case 0x3333:
			pddr_params = &a1l_ddr_para;
			printf("chip type is A1L\n");
			break;

		default:
			printf("Now 0x%x is not support!!!\n",chip);
			break;
	}
}
#endif

#ifdef  CONFIG_DDR_DEBUG
#define FUNC_ENTER() printf("%s enter.\n",__FUNCTION__);
#define FUNC_EXIT() printf("%s exit.\n",__FUNCTION__);

static void dump_ddrc_register(void)
{
	printf("DDRC_STATUS         0x%x\n", ddr_readl(DDRC_STATUS));
	printf("DDRC_CFG            0x%x\n", ddr_readl(DDRC_CFG));
	printf("DDRC_CTRL           0x%x\n", ddr_readl(DDRC_CTRL));
	printf("DDRC_LMR            0x%x\n", ddr_readl(DDRC_LMR));
	printf("DDRC_DLP            0x%x\n", ddr_readl(DDRC_DLP));
	printf("DDRC_TIMING1        0x%x\n", ddr_readl(DDRC_TIMING(1)));
	printf("DDRC_TIMING2        0x%x\n", ddr_readl(DDRC_TIMING(2)));
	printf("DDRC_TIMING3        0x%x\n", ddr_readl(DDRC_TIMING(3)));
	printf("DDRC_TIMING4        0x%x\n", ddr_readl(DDRC_TIMING(4)));
	printf("DDRC_TIMING5        0x%x\n", ddr_readl(DDRC_TIMING(5)));
	printf("DDRC_REFCNT         0x%x\n", ddr_readl(DDRC_REFCNT));
	printf("DDRC_AUTOSR_CNT     0x%x\n", ddr_readl(DDRC_AUTOSR_CNT));
	printf("DDRC_AUTOSR_EN      0x%x\n", ddr_readl(DDRC_AUTOSR_EN));
	printf("DDRC_MMAP0          0x%x\n", ddr_readl(DDRC_MMAP0));
	printf("DDRC_MMAP1          0x%x\n", ddr_readl(DDRC_MMAP1));
	printf("DDRC_REMAP1         0x%x\n", ddr_readl(DDRC_REMAP(1)));
	printf("DDRC_REMAP2         0x%x\n", ddr_readl(DDRC_REMAP(2)));
	printf("DDRC_REMAP3         0x%x\n", ddr_readl(DDRC_REMAP(3)));
	printf("DDRC_REMAP4         0x%x\n", ddr_readl(DDRC_REMAP(4)));
	printf("DDRC_REMAP5         0x%x\n", ddr_readl(DDRC_REMAP(5)));
	printf("DDRC_DWCFG          0x%x\n", ddr_readl(DDRC_DWCFG));
	printf("DDRC_DWSTATUS       0x%x\n", ddr_readl(DDRC_DWSTATUS));

	printf("DDRC_HREGPRO        0x%x\n", ddr_readl(DDRC_HREGPRO));
	printf("DDRC_PREGPRO        0x%x\n", ddr_readl(DDRC_PREGPRO));
	printf("DDRC_CGUC0          0x%x\n", ddr_readl(DDRC_CGUC0));
	printf("DDRC_CGUC1          0x%x\n", ddr_readl(DDRC_CGUC1));

}

static void dump_ddrp_register(void)
{
#if 1
	printf("DDRP_INNO_PHY_RST    0x%x\n", ddr_readl(DDRP_INNOPHY_INNO_PHY_RST	));
	printf("DDRP_MEM_CFG         0x%x\n", ddr_readl(DDRP_INNOPHY_MEM_CFG		));
	printf("DDRP_TRAINING_CTRL   0x%x\n", ddr_readl(DDRP_INNOPHY_TRAINING_CTRL	));
	printf("DDRP_CALIB_DELAY_AL  0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL1));
	printf("DDRP_CALIB_DELAY_AH  0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH1));
	printf("DDRP_CALIB_DELAY_BL  0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL1));
	printf("DDRP_CALIB_DELAY_BH  0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH1));
	printf("DDRP_CL              0x%x\n", ddr_readl(DDRP_INNOPHY_CL				));
	printf("DDRP_CWL             0x%x\n", ddr_readl(DDRP_INNOPHY_CWL			));
	printf("DDRP_WL_DONE         0x%x\n", ddr_readl(DDRP_INNOPHY_WL_DONE		));
	printf("DDRP_CALIB_DONE      0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DONE		));
	printf("DDRP_PLL_LOCK        0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_LOCK		));
	printf("DDRP_PLL_FBDIV       0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_FBDIV		));
	printf("DDRP_PLL_CTRL        0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_CTRL		));
	printf("DDRP_PLL_PDIV        0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_PDIV		));

	printf("DDRP_TRAINING_2c     0x%x\n", ddr_readl(DDRP_INNOPHY_TRAINING_2c	));
	printf("DDRP_TRAINING_3c     0x%x\n", ddr_readl(DDRP_INNOPHY_TRAINING_3c	));
	printf("DDRP_TRAINING_4c     0x%x\n", ddr_readl(DDRP_INNOPHY_TRAINING_4c	));
	printf("DDRP_TRAINING_5c     0x%x\n", ddr_readl(DDRP_INNOPHY_TRAINING_5c	));
#endif
#if 0
	unsigned int offset;
	printf("------------------ ddr phy common register ------------------\n");
	for (offset = 0; offset <= 0x136; offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	printf("------------------ ddr phy specail register ------------------\n");

	printf("-------- DRIVE STRENGTH --------\n");
	for (offset = 0x130; offset <= (0x130+3); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = 0x140; offset <= (0x140+3); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = 0x150; offset <= (0x150+3); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = 0x160; offset <= (0x160+3); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = 0x170; offset <= (0x170+3); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	printf("-------- PER BIT DE-SKEW --------\n");
	for (offset = 0x340; offset <= (0x340+0x32); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = 0x1c0; offset <= (0x1c0+0x2b); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = 0x220; offset <= (0x220+0x2b); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	printf("-------- RX CALIBRATION --------\n");
	for (offset = (0x70+0x2b); offset <= (0x70+0x2e); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = (0xa0+0x2b); offset <= (0xa0+0x2e); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = (0x70+0x00); offset <= (0x70+0x01); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = (0x70+0x10); offset <= (0x70+0x11); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = (0x70+0x02); offset <= (0x70+0x03); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = (0x70+0x12); offset <= (0x70+0x13); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = (0xa0+0x00); offset <= (0xa0+0x01); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = (0xa0+0x10); offset <= (0xa0+0x11); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = (0xa0+0x02); offset <= (0xa0+0x03); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
	for (offset = (0xa0+0x12); offset <= (0xa0+0x13); offset++) {
		printf("ddr phy offset %x, addr %x, value %x\n", offset, DDR_PHY_BASE+offset*4, readl(DDR_PHY_BASE+offset*4));
	}
#endif
}

static void dump_ddrp_driver_strength_register(void)
{
	printf("-------------------------------------------------------\n");
	printf("cmd strenth pull_down                = 0x%x\n", readl(DDR_PHY_BASE+4*0x130));
	printf("cmd strenth pull_up                  = 0x%x\n", readl(DDR_PHY_BASE+4*0x131));
	printf("clk strenth pull_down                = 0x%x\n", readl(DDR_PHY_BASE+4*0x132));
	printf("clk strenth pull_up                  = 0x%x\n", readl(DDR_PHY_BASE+4*0x133));
	printf("data A io ODT DQ[7:0]  pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x140));
	printf("data A io ODT DQ[7:0]  pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x141));
	printf("data A io ODT DQ[15:8] pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x150));
	printf("data A io ODT DQ[15:8] pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x151));
	printf("data B io ODT DQ[7:0]  pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x160));
	printf("data B io ODT DQ[7:0]  pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x161));
	printf("data B io ODT DQ[15:8] pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x170));
	printf("data B io ODT DQ[15:8] pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x171));
	printf("data A io strenth DQ[7:0]  pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x142));
	printf("data A io strenth DQ[7:0]  pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x143));
	printf("data A io strenth DQ[15:8] pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x152));
	printf("data A io strenth DQ[15:8] pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x153));
	printf("data B io strenth DQ[7:0]  pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x162));
	printf("data B io strenth DQ[7:0]  pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x163));
	printf("data B io strenth DQ[15:8] pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x172));
	printf("data B io strenth DQ[15:8] pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x173));
#if 0
	writel(0x1c, DDR_PHY_BASE+4*0x130);
	writel(0x1c, DDR_PHY_BASE+4*0x131);
	writel(0x1c, DDR_PHY_BASE+4*0x132);
	writel(0x1c, DDR_PHY_BASE+4*0x133);
#endif
#if 0
	//pull down
	writel(0x1, DDR_PHY_BASE+4*0x140);
	writel(0x1, DDR_PHY_BASE+4*0x150);
	writel(0x1, DDR_PHY_BASE+4*0x160);
	writel(0x1, DDR_PHY_BASE+4*0x170);
	//pull up
	writel(0x0, DDR_PHY_BASE+4*0x141);
	writel(0x0, DDR_PHY_BASE+4*0x151);
	writel(0x0, DDR_PHY_BASE+4*0x161);
	writel(0x0, DDR_PHY_BASE+4*0x171);
#endif
#if 0
	writel(0x1c, DDR_PHY_BASE+4*0x142);
	writel(0x1c, DDR_PHY_BASE+4*0x143);
	writel(0x1c, DDR_PHY_BASE+4*0x152);
	writel(0x1c, DDR_PHY_BASE+4*0x153);
	writel(0x1c, DDR_PHY_BASE+4*0x162);
	writel(0x1c, DDR_PHY_BASE+4*0x163);
	writel(0x1c, DDR_PHY_BASE+4*0x172);
	writel(0x1c, DDR_PHY_BASE+4*0x173);
#endif
#if 0
	printf("-------------------------------------------------------\n");
	printf("cmd strenth pull_down                = 0x%x\n", readl(DDR_PHY_BASE+4*0x130));
	printf("cmd strenth pull_up                  = 0x%x\n", readl(DDR_PHY_BASE+4*0x131));
	printf("clk strenth pull_down                = 0x%x\n", readl(DDR_PHY_BASE+4*0x132));
	printf("clk strenth pull_up                  = 0x%x\n", readl(DDR_PHY_BASE+4*0x133));
	printf("data A io ODT DQ[7:0]  pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x140));
	printf("data A io ODT DQ[7:0]  pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x141));
	printf("data A io ODT DQ[15:8] pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x150));
	printf("data A io ODT DQ[15:8] pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x151));
	printf("data B io ODT DQ[7:0]  pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x160));
	printf("data B io ODT DQ[7:0]  pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x161));
	printf("data B io ODT DQ[15:8] pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x170));
	printf("data B io ODT DQ[15:8] pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x171));
	printf("data A io strenth DQ[7:0]  pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x142));
	printf("data A io strenth DQ[7:0]  pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x143));
	printf("data A io strenth DQ[15:8] pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x152));
	printf("data A io strenth DQ[15:8] pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x153));
	printf("data B io strenth DQ[7:0]  pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x162));
	printf("data B io strenth DQ[7:0]  pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x163));
	printf("data B io strenth DQ[15:8] pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x172));
	printf("data B io strenth DQ[15:8] pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x173));
#endif

}

static void dump_ddrp_per_bit_de_skew(void)
{
	int channel = 0;
	unsigned int reg_offset[2] = {0x1c0, 0x220};
	unsigned int offset;
	printf("------------------------------rx---------------------------------\n");
	for (channel = 0; channel < 2; channel++) {
		offset = reg_offset[channel];
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x0)*4, readl(DDR_PHY_BASE+(offset+0x0)*4));//RX DM0
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x2)*4, readl(DDR_PHY_BASE+(offset+0x2)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x4)*4, readl(DDR_PHY_BASE+(offset+0x4)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x6)*4, readl(DDR_PHY_BASE+(offset+0x6)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x8)*4, readl(DDR_PHY_BASE+(offset+0x8)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0xa)*4, readl(DDR_PHY_BASE+(offset+0xa)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0xc)*4, readl(DDR_PHY_BASE+(offset+0xc)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0xe)*4, readl(DDR_PHY_BASE+(offset+0xe)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x10)*4, readl(DDR_PHY_BASE+(offset+0x10)*4));//RX DQ7
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x12)*4, readl(DDR_PHY_BASE+(offset+0x12)*4));//RX DQS0
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x15)*4, readl(DDR_PHY_BASE+(offset+0x15)*4));//RX DM1
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x17)*4, readl(DDR_PHY_BASE+(offset+0x17)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x19)*4, readl(DDR_PHY_BASE+(offset+0x19)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x1b)*4, readl(DDR_PHY_BASE+(offset+0x1b)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x1d)*4, readl(DDR_PHY_BASE+(offset+0x1d)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x1f)*4, readl(DDR_PHY_BASE+(offset+0x1f)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x21)*4, readl(DDR_PHY_BASE+(offset+0x21)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x23)*4, readl(DDR_PHY_BASE+(offset+0x23)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x25)*4, readl(DDR_PHY_BASE+(offset+0x25)*4));//RX DQ15
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x27)*4, readl(DDR_PHY_BASE+(offset+0x27)*4));//RX DQS1
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x2a)*4, readl(DDR_PHY_BASE+(offset+0x2a)*4));//RX DQSB0
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x2b)*4, readl(DDR_PHY_BASE+(offset+0x2b)*4));//RX DQSB1
	}
	printf("------------------------------tx---------------------------------\n");
	for (channel = 0; channel < 2; channel++) {
		offset = reg_offset[channel];
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x1)*4, readl(DDR_PHY_BASE+(offset+0x1)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x3)*4, readl(DDR_PHY_BASE+(offset+0x3)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x5)*4, readl(DDR_PHY_BASE+(offset+0x5)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x7)*4, readl(DDR_PHY_BASE+(offset+0x7)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x9)*4, readl(DDR_PHY_BASE+(offset+0x9)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0xb)*4, readl(DDR_PHY_BASE+(offset+0xb)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0xd)*4, readl(DDR_PHY_BASE+(offset+0xd)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0xf)*4, readl(DDR_PHY_BASE+(offset+0xf)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x11)*4, readl(DDR_PHY_BASE+(offset+0x1a)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x13)*4, readl(DDR_PHY_BASE+(offset+0x13)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x14)*4, readl(DDR_PHY_BASE+(offset+0x14)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x16)*4, readl(DDR_PHY_BASE+(offset+0x16)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x18)*4, readl(DDR_PHY_BASE+(offset+0x18)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x1c)*4, readl(DDR_PHY_BASE+(offset+0x1c)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x1e)*4, readl(DDR_PHY_BASE+(offset+0x1e)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x20)*4, readl(DDR_PHY_BASE+(offset+0x20)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x22)*4, readl(DDR_PHY_BASE+(offset+0x22)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x24)*4, readl(DDR_PHY_BASE+(offset+0x24)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x26)*4, readl(DDR_PHY_BASE+(offset+0x26)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x28)*4, readl(DDR_PHY_BASE+(offset+0x28)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x29)*4, readl(DDR_PHY_BASE+(offset+0x29)*4));

	}


}
#else
#define FUNC_ENTER()
#define FUNC_EXIT()
#define dump_ddrc_register()
#define dump_ddrp_register()
#define dump_ddrp_driver_strength_register()

#ifdef DDR_PHY_PARAMS_TEST_A1
static void dump_ddrp_per_bit_de_skew(void)
{
	int channel = 0;
	unsigned int reg_offset[2] = {0x1c0, 0x220};
	unsigned int offset;
	printf("------------------------------rx---------------------------------\n");
	for (channel = 0; channel < 2; channel++) {
		offset = reg_offset[channel];
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x0, DDR_PHY_BASE+(offset+0x0)*4, readl(DDR_PHY_BASE+(offset+0x0)*4));//RX DM0
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x2, DDR_PHY_BASE+(offset+0x2)*4, readl(DDR_PHY_BASE+(offset+0x2)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x4, DDR_PHY_BASE+(offset+0x4)*4, readl(DDR_PHY_BASE+(offset+0x4)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x6, DDR_PHY_BASE+(offset+0x6)*4, readl(DDR_PHY_BASE+(offset+0x6)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x8, DDR_PHY_BASE+(offset+0x8)*4, readl(DDR_PHY_BASE+(offset+0x8)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0xa, DDR_PHY_BASE+(offset+0xa)*4, readl(DDR_PHY_BASE+(offset+0xa)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0xc, DDR_PHY_BASE+(offset+0xc)*4, readl(DDR_PHY_BASE+(offset+0xc)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0xe, DDR_PHY_BASE+(offset+0xe)*4, readl(DDR_PHY_BASE+(offset+0xe)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x10, DDR_PHY_BASE+(offset+0x10)*4, readl(DDR_PHY_BASE+(offset+0x10)*4));//RX DQ7
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x12, DDR_PHY_BASE+(offset+0x12)*4, readl(DDR_PHY_BASE+(offset+0x12)*4));//RX DQS0
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x15, DDR_PHY_BASE+(offset+0x15)*4, readl(DDR_PHY_BASE+(offset+0x15)*4));//RX DM1
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x17, DDR_PHY_BASE+(offset+0x17)*4, readl(DDR_PHY_BASE+(offset+0x17)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x19, DDR_PHY_BASE+(offset+0x19)*4, readl(DDR_PHY_BASE+(offset+0x19)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x1b, DDR_PHY_BASE+(offset+0x1b)*4, readl(DDR_PHY_BASE+(offset+0x1b)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x1d, DDR_PHY_BASE+(offset+0x1d)*4, readl(DDR_PHY_BASE+(offset+0x1d)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x1f, DDR_PHY_BASE+(offset+0x1f)*4, readl(DDR_PHY_BASE+(offset+0x1f)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x21, DDR_PHY_BASE+(offset+0x21)*4, readl(DDR_PHY_BASE+(offset+0x21)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x23, DDR_PHY_BASE+(offset+0x23)*4, readl(DDR_PHY_BASE+(offset+0x23)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x25, DDR_PHY_BASE+(offset+0x25)*4, readl(DDR_PHY_BASE+(offset+0x25)*4));//RX DQ15
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x27, DDR_PHY_BASE+(offset+0x27)*4, readl(DDR_PHY_BASE+(offset+0x27)*4));//RX DQS1
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x2a, DDR_PHY_BASE+(offset+0x2a)*4, readl(DDR_PHY_BASE+(offset+0x2a)*4));//RX DQSB0
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, 0x2b, DDR_PHY_BASE+(offset+0x2b)*4, readl(DDR_PHY_BASE+(offset+0x2b)*4));//RX DQSB1
	}
	printf("------------------------------tx---------------------------------\n");
	for (channel = 0; channel < 2; channel++) {
		offset = reg_offset[channel];
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x1)*4, readl(DDR_PHY_BASE+(offset+0x1)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x3)*4, readl(DDR_PHY_BASE+(offset+0x3)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x5)*4, readl(DDR_PHY_BASE+(offset+0x5)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x7)*4, readl(DDR_PHY_BASE+(offset+0x7)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x9)*4, readl(DDR_PHY_BASE+(offset+0x9)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0xb)*4, readl(DDR_PHY_BASE+(offset+0xb)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0xd)*4, readl(DDR_PHY_BASE+(offset+0xd)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0xf)*4, readl(DDR_PHY_BASE+(offset+0xf)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x11)*4, readl(DDR_PHY_BASE+(offset+0x1a)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x13)*4, readl(DDR_PHY_BASE+(offset+0x13)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x14)*4, readl(DDR_PHY_BASE+(offset+0x14)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x16)*4, readl(DDR_PHY_BASE+(offset+0x16)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x18)*4, readl(DDR_PHY_BASE+(offset+0x18)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x1c)*4, readl(DDR_PHY_BASE+(offset+0x1c)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x1e)*4, readl(DDR_PHY_BASE+(offset+0x1e)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x20)*4, readl(DDR_PHY_BASE+(offset+0x20)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x22)*4, readl(DDR_PHY_BASE+(offset+0x22)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x24)*4, readl(DDR_PHY_BASE+(offset+0x24)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x26)*4, readl(DDR_PHY_BASE+(offset+0x26)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x28)*4, readl(DDR_PHY_BASE+(offset+0x28)*4));
		printf("phy de skew chn %d, reg offset %x, reg addr %x, reg value %x\n", channel, offset, DDR_PHY_BASE+(offset+0x29)*4, readl(DDR_PHY_BASE+(offset+0x29)*4));

	}
}
#else
#define dump_ddrp_per_bit_de_skew()
#endif
#endif

/*
        设置DQ
        dqnum等于-1是设置整组。

        abchannel ： 0 a通道  1 b通道
        dqnum DQ编号
        bitoffset 比对的位偏移
*/
static void ddr_test_phy_dq_bit_set(int abchannel,int dqnum,int val)
{
        int dqx;
	unsigned int reg_offset[2] = {0x1c0, 0x220}; //AB通道
	int DQxRxOFFSET[]={0x2,0x4,0x6,0x8,0xa,0xc,0xe,0x10,0x17,0x19,0x1b,0x1d,0x1f,0x21,0x23,0x25};
        int chl=reg_offset[abchannel];

        if(dqnum == -1)
        {
                 for(dqx=0;dqx <= 15;dqx++)
                 {
                         writel(val,  DDR_PHY_BASE+(chl+DQxRxOFFSET[dqx])*4);
                 }
        }
        else
        {
                writel(val,  DDR_PHY_BASE+(chl+DQxRxOFFSET[dqnum])*4);
        }
}

static void mem_remap(void)
{
	int i;
#ifdef CONFIG_A1ALL
			unsigned int remap_array[5];
			memcpy(remap_array, pddr_params->array, sizeof(remap_array));
#else
		unsigned int remap_array[] = REMMAP_ARRAY;
#endif
		for(i = 0;i < ARRAY_SIZE(remap_array);i++)
		{
			printf("remap_array[%d] is 0x%x\n", i, remap_array[i]);
			ddr_writel(remap_array[i],DDRC_REMAP(i+1));
		}
}

//#define ddr_writel(value, reg)	writel((value), DDRC_BASE + reg)

static void ddrp_reg_set_range(u32 offset, u32 startbit, u32 bitscnt, u32 value)
{
	u32 reg = 0;
	u32 mask = 0;
	mask = ((0xffffffff>>startbit)<<(startbit))&((0xffffffff<<(32-startbit-bitscnt))>>(32-startbit-bitscnt));
	reg = readl(DDRC_BASE+DDR_PHY_OFFSET+(offset*4));
	reg = (reg&(~mask))|((value<<startbit)&mask);
	//printf("value = %x, reg = %x, mask = %x", value, reg, mask);
	writel(reg, DDRC_BASE+DDR_PHY_OFFSET+(offset*4));
}

static u32 ddrp_reg_get(u32 offset)
{
	return  readl(DDRC_BASE+DDR_PHY_OFFSET+(offset*4));
}

#ifdef DDR_PHY_PARAMS_TEST_A1
static int ddr_mem_test(void)
{
	int i = 0;
	int j = 0;
	unsigned int addr;
	unsigned int value_get;
	unsigned int data;
	unsigned int test_size =  64;
	unsigned int test_data =  0xffffffff;
        int ret = 0;

        printf("mem test12 start! input\n");
        scanf("%u\n",&test_data);
	printf("mem test12 test_data:%x\n",test_data);

	for(i = 0; i < 1; i++) {


		addr = 0x81000000;
		for (j = 0; j < test_size; j+=4) {
			data = (j/4)%2?test_data:0;
			*(volatile unsigned int *)(addr+j) = data;
		}

		//flush cache
		flush_dcache_range(addr, addr+test_size);
		flush_l2cache_range(addr, addr+test_size);

		//read cached
		addr = 0x81000000;
		for (j = 0; j < test_size; j+=4) {
			value_get = *(volatile unsigned int *)(addr+j);
		}
#if 0
		//write uncached
		addr = 0xa1000000;
		for (j = 0; j < test_size; j+=4) {
			data = (j/4)%2?test_data:0;
			*(volatile unsigned int *)(addr+j) = data;
		}
#endif
		//invalid cache
		addr = 0x81000000;
		invalid_dcache_range(addr, addr+test_size);
		invalid_l2cache_range(addr, addr+test_size);
		//read cached
		addr = 0x81000000;
		printf("mem data:\n");
		for (j = 0; j < test_size; j+=4) {
			value_get = *(volatile unsigned int *)(addr+j);

                        //printf("%x:%x\n ", value_get,((j/4)%2?test_data:0));
			if (value_get != ((j/4)%2?test_data:0))
                                ret++;
#if 0
			if (0 == j/4%4)
				printf("%x: ", addr+j);
			printf("  %x", value_get);
			if (3 == j/4%4)
				printf("\n");
#endif
		}
	}
	//printf("mem test10 end!\n");
	return ret;
}


unsigned int ddr_mem_data_patterns[][16] = {
	{0x00000000,0xffffffff,0x00000000,0xffffffff,0x00000000,0xffffffff,0x00000000,0xffffffff,0x00000000,0xffffffff,0x00000000,0xffffffff,0x00000000,0xffffffff,0x00000000,0xffffffff},
	{0x00000000,0xffff0000,0x00000000,0xffff0000,0x00000000,0xffff0000,0x00000000,0xffff0000,0x00000000,0xffff0000,0x00000000,0xffff0000,0x00000000,0xffff0000,0x00000000,0xffff0000},
	{0x00000000,0x0000ffff,0x00000000,0x0000ffff,0x00000000,0x0000ffff,0x00000000,0x0000ffff,0x00000000,0x0000ffff,0x00000000,0x0000ffff,0x00000000,0x0000ffff,0x00000000,0x0000ffff},
	{0x00000000,0x000000ff,0x00000000,0x000000ff,0x00000000,0x000000ff,0x00000000,0x000000ff,0x00000000,0x000000ff,0x00000000,0x000000ff,0x00000000,0x000000ff,0x00000000,0x000000ff},
	{0x00000000,0x0000ff00,0x00000000,0x0000ff00,0x00000000,0x0000ff00,0x00000000,0x0000ff00,0x00000000,0x0000ff00,0x00000000,0x0000ff00,0x00000000,0x0000ff00,0x00000000,0x0000ff00},
        {0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000,0xffff0000},
        {0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff,0x0000ffff},
        {0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xff0fffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xfffff0ff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff},
        {0x5a5a5a5a,0xa5a5a5a5,0x5a5a5a5a,0xa5a5a5a5,0x5a5a5a5a,0xa5a5a5a5,0x5a5a5a5a,0xa5a5a5a5,0x5a5a5a5a,0xa5a5a5a5,0x5a5a5a5a,0xa5a5a5a5,0x5a5a5a5a,0xa5a5a5a5,0x5a5a5a5a,0xa5a5a5a5},
        {0xffff0000,0x0000ffff,0xffff0000,0x0000ffff,0xffff0000,0x0000ffff,0xffff0000,0x0000ffff,0xffff0000,0x0000ffff,0xffff0000,0x0000ffff,0xffff0000,0x0000ffff,0xffff0000,0x0000ffff},
	{0xffffffff,0xffffffff,0x00000000,0x00000000,0xffffffff,0xffffffff,0x00000000,0x00000000,0xffffffff,0xffffffff,0x00000000,0x00000000,0xffffffff,0xffffffff,0x00000000,0x00000000},
        {0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828,0x28282828},
	{0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000,0x00060000}
};

/*
        按字节或者位测试pattern
         bit: -1 按字节, or 0-31 bit
*/
static int ddr_mem_pattern_test(int bit)
{
	int i = 0;
	int j = 0;
	int pattern_num = 0;
	int data_num = 0;
	int ret = 0;
	unsigned int addr;
	unsigned int value_get;
	unsigned int data;

	pattern_num = sizeof(ddr_mem_data_patterns)/sizeof(ddr_mem_data_patterns[i]);
       // printf("pattern_num %d\n",pattern_num);
	for(i = 0; i < pattern_num; i++) {
		addr = 0x81000000;
		data_num = sizeof(ddr_mem_data_patterns[i])/4;

                //printf("data_num %d\n",data_num);
		for(j = 0; j < data_num; j++) {
			data = ddr_mem_data_patterns[i][j];
			*(volatile unsigned int *)(addr+j*4) = data;
		}
		//flush cache
		flush_dcache_range(addr, addr+data_num*4);
		flush_l2cache_range(addr, addr+data_num*4);

		//read cached
		addr = 0x81000000;
		for(j = 0; j < data_num; j++) {
			value_get = *(volatile unsigned int *)(addr+j*4);
		}
#if 0
		//write uncached
		addr = 0xa1000000;
		for(j = 0; j < data_num; j++) {
			data = ddr_mem_data_patterns[i][j];
			*(volatile unsigned int *)(addr+j*4) = data;
		}
#endif
		//invalid cache
		addr = 0x81000000;
		invalid_dcache_range(addr, addr+data_num*4);
		invalid_l2cache_range(addr, addr+data_num*4);
		//read cached
		addr = 0x81000000;
		//printf("mem data:\n");
		for(j = 0; j < data_num; j++) {
			data = ddr_mem_data_patterns[i][j];
			value_get = *(volatile unsigned int *)(addr+j*4);
                        if (-1 == bit) {
			        if (data != value_get)
				        ret++;
                        } else {
			        if ((data&(1<<bit)) != (value_get&(1<<bit)))
				        ret++;
                        }

                        #if 0
			if (0 == j%4)
				printf("%x: ", addr+j);
			printf("  %x", value_get);
			if (3 == j%4)
				printf("\n");
                        #endif
		}
	}
	return ret;
}


/*
        测试一遍pattern,并打印结果，偏于查看DDR数据对错状态。
*/
static int ddr_mem_pattern_test_once(void)
{
	int i = 0;
	int j = 0;
	int pattern_num = 0;
	int data_num = 0;
	int ret = 0;
	unsigned int addr;
	unsigned int value_get;
	unsigned int data;

	pattern_num = sizeof(ddr_mem_data_patterns)/sizeof(ddr_mem_data_patterns[i]);
       // printf("pattern_num %d\n",pattern_num);
	for(i = 0; i < pattern_num; i++) {
		addr = 0x81000000;
		data_num = sizeof(ddr_mem_data_patterns[i])/4;

                //printf("data_num %d\n",data_num);
		for(j = 0; j < data_num; j++) {
			data = ddr_mem_data_patterns[i][j];
			*(volatile unsigned int *)(addr+j*4) = data;
		}
		//flush cache
		flush_dcache_range(addr, addr+data_num*4);
		flush_l2cache_range(addr, addr+data_num*4);

		//read cached
		addr = 0x81000000;
		for(j = 0; j < data_num; j++) {
			value_get = *(volatile unsigned int *)(addr+j*4);
		}
#if 0
		//write uncached
		addr = 0xa1000000;
		for(j = 0; j < data_num; j++) {
			data = ddr_mem_data_patterns[i][j];
			*(volatile unsigned int *)(addr+j*4) = data;
		}
#endif
		//invalid cache
		addr = 0x81000000;
		invalid_dcache_range(addr, addr+data_num*4);
		invalid_l2cache_range(addr, addr+data_num*4);
		//read cached
		addr = 0x81000000;
		//printf("mem data:\n");
		for(j = 0; j < data_num; j++) {
			data = ddr_mem_data_patterns[i][j];
			value_get = *(volatile unsigned int *)(addr+j*4);
			if (data != value_get)
				ret++;

                        #if 1
			if (0 == j%4)
				printf("%x: ", addr+j);
			printf("  %x", value_get);
			if (3 == j%4)
				printf("\n");

                        #endif
		}

	}
	return ret;
}

/*
        测试一个DQ   ，0到31范围，数据是否正确。
        abchannel ： 0 a通道  1 b通道
        dqnum DQ编号
        bitoffset 比对的位偏移
        txrx: 0 rx, 1 tx.
*/
static void ddr_test_phy_dq_bit(int abchannel,int dqnum,int bitoffset,int txrx)
{
        int regval;
	unsigned int reg_offset[2] = {0x1c0, 0x220}; //AB通道
	int DQxRxOFFSET[]={0x2,0x4,0x6,0x8,0xa,0xc,0xe,0x10,0x17,0x19,0x1b,0x1d,0x1f,0x21,0x23,0x25}; //DQxxx RX寄存器偏移
	int DQxTxOFFSET[]={0x1,0x3,0x5,0x7,0x9,0xb,0xd,0xf,0x18,0x1a,0x1c,0x1e,0x20,0x22,0x24,0x26}; //DQxxx TX寄存器偏移
        int chl=reg_offset[abchannel];
        int firstRight=0,endRight=0;
        bool sw = 1;

        if(abchannel == 1) //B通道+16
                bitoffset+=16;

        //值范围0-0x3f
        for(regval=0;regval <= 0x3f;regval++)
        {
                if(txrx == 0)
                        writel(regval,  DDR_PHY_BASE+(chl+DQxRxOFFSET[dqnum])*4);
                else
                        writel(regval,  DDR_PHY_BASE+(chl+DQxTxOFFSET[dqnum])*4);

                if(ddr_mem_pattern_test(bitoffset))
                        printf("-"); //错误
                else
               {
                       printf("*"); //正确

                       if(sw == 1)
                       {
                           sw = 0;
                           firstRight = regval;
                       }
                       endRight = regval;
               }
        }
        //写回默认值，不影响其他DQ的测试。
        if(txrx == 0)
                writel(0x7,  DDR_PHY_BASE+(chl+DQxRxOFFSET[dqnum])*4);
        else
                writel(regval,  DDR_PHY_BASE+(chl+DQxTxOFFSET[dqnum])*4);

        printf("  right first:%d,end:%d,mid:%d\n",firstRight,endRight,(endRight+firstRight)/2);
}


/*
        按通道测试一组DQ

        abchannel ： 0 a通道  1 b通道
        txrx: 0 rx , 1 tx
*/
static void ddr_test_phy_dq_channel(int abchannel,int txrx)
{
        int dqx;

        printf("\n");

        printf(" dqx:                        0 - 0x3f \n");
        for(dqx=0; dqx<=15; dqx++)
        {
                if(dqx < 10)
                        printf(" ");
                printf("dq%d ",dqx);
                ddr_test_phy_dq_bit(abchannel,dqx,dqx,txrx);
        }
}


static void ddr_test_phy_parms(void)
{

	int odt_pu,odt_pd;
	int drvcmd;
	int drval_pu,drval_pd;
	int drvah_pu,drvah_pd;
	int drvbl_pu,drvbl_pd;
	int drvbh_pu,drvbh_pd;
        int ret,sum=0,w,x,y,z;

        int odt_pdl,odt_pul,odt_pdh,odt_puh;


	printf("##############ddr_test_phy_parms########################\n");

        //for(odt_pd=0;odt_pd<=0x1f;odt_pd++)
        //for(odt_pu=0;odt_pu<=0x1f;odt_pu++)
        //for(drvcmd=0;drvcmd<=0x1f;drvcmd++)
        //odt_pd=7;
        //odt_pu=1;

        for(w=0;w<=0xf;w++)
        for(x=0;x<=0xf;x++)

        for(y=0;y<=0xf;y++)
        for(z=0;z<=0xf;z++)

        for(drvcmd=0xe;drvcmd<=0xe;drvcmd++)
                        for(drval_pd=0x14;drval_pd<=0x14;drval_pd++)
                        for(drval_pu=0x14;drval_pu<=0x14;drval_pu++)
                        for(drvah_pd=0x14;drvah_pd<=0x14;drvah_pd++)
                        for(drvah_pu=0x14;drvah_pu<=0x14;drvah_pu++)
                                for(drvbl_pd=0x14;drvbl_pd<=0x14;drvbl_pd++)
                                for(drvbl_pu=0x14;drvbl_pu<=0x14;drvbl_pu++)
                                for(drvbh_pd=0x14;drvbh_pd<=0x14;drvbh_pd++)
                                for(drvbh_pu=0x14;drvbh_pu<=0x14;drvbh_pu++)

                                {

#if 0
                                        //odt
                                        //pull down
                                        writel(odt_pd, DDR_PHY_BASE+4*0x140);//a
                                        writel(odt_pd, DDR_PHY_BASE+4*0x150);
                                        //pull up
                                        writel(odt_pu, DDR_PHY_BASE+4*0x141);//a
                                        writel(odt_pu, DDR_PHY_BASE+4*0x151);

                                        //pull down
                                        writel(odt_pd, DDR_PHY_BASE+4*0x160);//b
                                        writel(odt_pd, DDR_PHY_BASE+4*0x170);
                                        //pull up
                                        writel(odt_pu, DDR_PHY_BASE+4*0x161);//b
                                        writel(odt_pu, DDR_PHY_BASE+4*0x171);

#endif
                                        //driver
                                        writel(drvcmd, DDR_PHY_BASE+4*0x130);
                                        writel(drvcmd, DDR_PHY_BASE+4*0x131);
                                        writel(drvcmd, DDR_PHY_BASE+4*0x132);
                                        writel(drvcmd, DDR_PHY_BASE+4*0x133);



                                        //pull down
                                        writel(w, DDR_PHY_BASE+4*0x142);   //al_pd
                                        writel(w, DDR_PHY_BASE+4*0x152);   //ah pd

                                        //pull up
                                        writel(x, DDR_PHY_BASE+4*0x143);  //al_pu
                                        writel(x, DDR_PHY_BASE+4*0x153);



                                        //pull down
                                        writel(y, DDR_PHY_BASE+4*0x162);   //bl_pd
                                        writel(y, DDR_PHY_BASE+4*0x172);

                                        //pull up
                                        writel(z, DDR_PHY_BASE+4*0x163);   //bl_pu
                                        writel(z, DDR_PHY_BASE+4*0x173);

                                        ret=ddr_mem_pattern_test(-1);
                                        sum++;
                                        if(!ret)
                                        {
                                                printf("Right:r %d,s %d ",ret,sum);

                                                 #if 0
                                                 printf("dump drv,odt:"
                                                                "odt_pd = %x, odt_pu = %x"
                                                                "drvcmd = %x"
                                                                "drval_pd = %x, drval_pu = %x"
                                                                "drvah_pd = %x, drvah_pu = %x"
                                                                "drvbl_pd = %x, drvbl_pu = %x"
                                                                "drvbh_pd = %x, drvbh_pu = %x\n",
                                                                odt_pd, odt_pu,
                                                                drvcmd,
                                                                drval_pd, drval_pu,
                                                                drvah_pd, drvah_pu,
                                                                drvbl_pd, drvbl_pu,
                                                                drvbh_pd, drvbh_pu
                                                          );
                                                #endif
                                                 //printf("opd=%x opu=%x cmd=%x\n",odt_pd, odt_pu,drvcmd);
                                                printf("drv apd:%x,apu:%x;bpd:%x,bpu:%x\n",w,x,y,z);
                                        }
                                        else{
                                                printf("wrong:r %d,s %d ",ret,sum);
                                                #if 0
                                                printf("dump drv,odt:"
                                                                "odt_pd = %x, odt_pu = %x"
                                                                "drvcmd = %x"
                                                                "drval_pd = %x, drval_pu = %x"
                                                                "drvah_pd = %x, drvah_pu = %x"
                                                                "drvbl_pd = %x, drvbl_pu = %x"
                                                                "drvbh_pd = %x, drvbh_pu = %x\n",
                                                                odt_pd, odt_pu,
                                                                drvcmd,
                                                                drval_pd, drval_pu,
                                                                drvah_pd, drvah_pu,
                                                                drvbl_pd, drvbl_pu,
                                                                drvbh_pd, drvbh_pu
                                                          );
                                                #endif
                                                 //printf("opd=%x opu=%x cmd=%x\n",odt_pd, odt_pu,drvcmd);
                                                printf("drv apd:%x,apu:%x;bpd:%x,bpu:%x\n",w,x,y,z);
                                        }
        }
	printf("##############ddr_test_phy_parms  end########################\n");

}


int set_rx_vref(int val)
{
    int A_L=0x80,A_H=0x80,B_L=0x80,B_H=0x80;

    //printf("ddrp_set_rx_vref &A_L,&A_H,&B_L,&B_H:\n");

    //A_L
    ddrp_reg_set_range(0x140+0x07, 0, 8, val);
    //A_H
    ddrp_reg_set_range(0x140+0x17, 0, 8, val);
    //B_L
    ddrp_reg_set_range(0x160+0x07, 0, 8, val);
    //B_H
    ddrp_reg_set_range(0x160+0x17, 0, 8, val);

    //printf("ddrp vref AL: %x\n", ddrp_readl_byidx(0x140+0x07));
    //printf("ddrp vref AH: %x\n", ddrp_readl_byidx(0x140+0x17));
    //printf("ddrp vref BL: %x\n", ddrp_readl_byidx(0x160+0x07));
    //printf("ddrp vref BH: %x\n", ddrp_readl_byidx(0x160+0x07));
	return 0;
}


/*
        测试一个DQ   ，0到31范围，数据是否正确。
        abchannel ： 0 a通道  1 b通道
        dqnum DQ编号
        bitoffset 比对的位偏移
*/
static int ddr_mem_pattern_test_abchannel_bit(int abchannel,int dqnum,int bitoffset)
{
        int regval;
        int num=-1;

        if(abchannel == 1) //B通道+16
                bitoffset+=16;

        if(ddr_mem_pattern_test(bitoffset))
        {
                printf("-"); //错误
                return -1;
        }
        else
       {
               num = regval;
               printf("*"); //正确
               return 0;
       }

}


static void ddr_test_phy_vref_dq_channel(int abchannel,int vrefmin,int vrefmax)
{
        int dqx;

        printf("\n");
        for(dqx=0; dqx<=15; dqx++)
        {
                if(dqx < 10)
                        printf(" ");
                printf("dq%d ",dqx);
                ddr_test_phy_vref_dq_bit(abchannel,dqx,dqx);
        }
}

void ddr_test_vref(void)
{
        int times,i,j,rt;
        static int min=0,max=0,en=0;
        en=0;

        printf("\n                                                        vref \n",j);
        for(j=0;j<16;j++)
        {
                printf("A dq%d ",j);
                for(i=50;i<=205;i++)
                {
                        set_rx_vref(i);
                        //printf(" vref:%d ",i);
                        rt=ddr_mem_pattern_test_abchannel_bit(0, j, j);
                        if(!rt)
                        {
                                if(!en)
                                        min = i;
                                en=1;
                                max = i;
                        }
                }
                en=0;

                printf("  vref min:%d,mid:%d,max:%d\n",min,(min+max)/2,max);
        }
        printf("\n\n");

        en=0;
        for(j=0;j<16;j++)
        {
                printf("B dq%d ",j);
                for(i=50;i<=205;i++)
                {
                        set_rx_vref(i);
                        //printf(" vref:%d ",i);
                        rt=ddr_mem_pattern_test_abchannel_bit(1, j, j);
                        if(!rt)
                        {
                                if(!en)
                                        min = i;
                                en=1;
                                max = i;
                        }
                }

                en=0;
                printf("  vref min:%d,mid:%d,max:%d\n",min,(min+max)/2,max);
        }

}

/*
        测试设置DQS不同值时，AB通道，所有DQx 0-0x3f时的对错
*/
void ddr_test_setDQS_ABchl_dqx(void)
{
        int onoff=-1,times,i,dqs=0xf,mode=0,cmdRegAddr,chl=0;

        printf("input test rx ?(0 rx,1 tx)\n");
        scanf("%d\n",&mode);
        if(!mode)
        {
                printf("input rx dqs ?(0-0x3f or 999 auto test)\n");
                scanf("%d\n",&dqs);
                if(dqs != 0)
                {
                        if(dqs <= 0x3f)
                        {
                                printf("dqs value:%x\n",readl(DDR_PHY_BASE+((0x1c0 + 0x12)*4)));
                                writel(dqs, DDR_PHY_BASE+((0x1c0 + 0x12)*4));
                                writel(dqs, DDR_PHY_BASE+((0x1c0 + 0x27)*4));
                                writel(dqs, DDR_PHY_BASE+((0x1c0 + 0x2a)*4));
                                writel(dqs, DDR_PHY_BASE+((0x1c0 + 0x2b)*4));

                                writel(dqs, DDR_PHY_BASE+((0x220 + 0x12)*4));
                                writel(dqs, DDR_PHY_BASE+((0x220 + 0x27)*4));
                                writel(dqs, DDR_PHY_BASE+((0x220 + 0x2a)*4));
                                writel(dqs, DDR_PHY_BASE+((0x220 + 0x2b)*4));

                                printf("dqs value:%x\n",readl(DDR_PHY_BASE+((0x1c0 + 0x12)*4)));
                        }
                        else
                        {
                                for(i=0;i<0x3f;i++)
                                {
                                        writel(i, DDR_PHY_BASE+((0x1c0 + 0x12)*4));
                                        writel(i, DDR_PHY_BASE+((0x1c0 + 0x27)*4));
                                        writel(i, DDR_PHY_BASE+((0x1c0 + 0x2a)*4));
                                        writel(i, DDR_PHY_BASE+((0x1c0 + 0x2b)*4));

                                        writel(i, DDR_PHY_BASE+((0x220 + 0x12)*4));
                                        writel(i, DDR_PHY_BASE+((0x220 + 0x27)*4));
                                        writel(i, DDR_PHY_BASE+((0x220 + 0x2a)*4));
                                        writel(i, DDR_PHY_BASE+((0x220 + 0x2b)*4));

                                        printf("\n\ndqs rx value:%x\n",readl(DDR_PHY_BASE+((0x1c0 + 0x12)*4)));
                                        printf("A rx:\n");
                                        ddr_test_phy_dq_channel(0,0);
                                        printf("\n");
                                        printf("B rx:\n");
                                        ddr_test_phy_dq_channel(1,0);

                                        printf("\n\n");
                                }
                                        printf("reg read default:\n");
                                        printf("odt_pd:%x\n",readl(DDR_PHY_BASE+4*0x140));
                                        printf("odt_pu:%x\n",readl(DDR_PHY_BASE+4*0x141));
                                        printf("drvcmd:%x\n",readl(DDR_PHY_BASE+4*0x130));
                                        printf("drval_pd:%x\n",readl(DDR_PHY_BASE+4*0x142));
                                        printf("drvbl_pd:%x\n",readl(DDR_PHY_BASE+4*0x162));
                                return;
                        }
                }

                printf("\n");
                printf("A:\n");
                ddr_test_phy_dq_channel(0,0);
                printf("\n");
                printf("B:\n");
                ddr_test_phy_dq_channel(1,0);

                printf("reg read default:\n");
                printf("odt_pd:%x\n",readl(DDR_PHY_BASE+4*0x140));
                printf("odt_pu:%x\n",readl(DDR_PHY_BASE+4*0x141));
                printf("drvcmd:%x\n",readl(DDR_PHY_BASE+4*0x130));
                printf("drval_pd:%x\n",readl(DDR_PHY_BASE+4*0x142));
                printf("drvbl_pd:%x\n",readl(DDR_PHY_BASE+4*0x162));
        }
        else
        {
                printf("input tx dqs ?(0-0x3f or 999 auto test)\n");
                scanf("%d\n",&dqs);
                if(dqs != 0)
                {

                        printf("reg read default:\n");
                        printf("DQS TX :%x\n",readl(DDR_PHY_BASE+((0x1c0 + 0x13)*4)));
                        printf("DQS TX:%x\n",readl(DDR_PHY_BASE+((0x220 + 0x28)*4)));

                        printf("wr level :%x\n",readl(DDR_PHY_BASE+((0x1c0 + 0x13)*4)));


                        for(cmdRegAddr =0 ; cmdRegAddr <=0x32 ; cmdRegAddr++ )
                        {
                                if(cmdRegAddr > 0x1f && cmdRegAddr < 0x30)
                                        continue;
                                writel(i, DDR_PHY_BASE+((0x340 + cmdRegAddr)*4));

                                printf("cmd%d :%x\n",cmdRegAddr,readl(DDR_PHY_BASE+((0x340 + cmdRegAddr)*4)));
                        }

                        if(dqs <= 0x3f)
                        {

                        }
                        else
                        {
                                for(i=0;i<0x3f;i++)
                                {
                                        writel(i, DDR_PHY_BASE+((0x1c0 + 0x13)*4));
                                        writel(i, DDR_PHY_BASE+((0x1c0 + 0x14)*4));
                                        writel(i, DDR_PHY_BASE+((0x1c0 + 0x28)*4));
                                        writel(i, DDR_PHY_BASE+((0x1c0 + 0x29)*4));

                                        writel(i, DDR_PHY_BASE+((0x220 + 0x13)*4));
                                        writel(i, DDR_PHY_BASE+((0x220 + 0x14)*4));
                                        writel(i, DDR_PHY_BASE+((0x220 + 0x28)*4));
                                        writel(i, DDR_PHY_BASE+((0x220 + 0x29)*4));

                                        for(cmdRegAddr =0 ; cmdRegAddr <=0x32 ; cmdRegAddr++ )
                                        {
                                                if(cmdRegAddr > 0x1f && cmdRegAddr < 0x30)
                                                        continue;
                                                writel(i, DDR_PHY_BASE+((0x340 + cmdRegAddr)*4));

                                                printf("wr read cmd%d :%x\n",cmdRegAddr,readl(DDR_PHY_BASE+((0x340 + cmdRegAddr)*4)));
                                        }

                                        printf("\n\ndqs tx value:%x\n",readl(DDR_PHY_BASE+((0x220 + 0x29)*4)));
                                        printf("A tx:\n");
                                        ddr_test_phy_dq_channel(0,1);
                                        printf("\n");
                                        printf("B tx:\n");
                                        ddr_test_phy_dq_channel(1,1);

                                        printf("\n\n");
                                }
                                        printf("reg read default:\n");
                                        printf("odt_pd:%x\n",readl(DDR_PHY_BASE+4*0x140));
                                        printf("odt_pu:%x\n",readl(DDR_PHY_BASE+4*0x141));
                                        printf("drvcmd:%x\n",readl(DDR_PHY_BASE+4*0x130));
                                        printf("drval_pd:%x\n",readl(DDR_PHY_BASE+4*0x142));
                                        printf("drvbl_pd:%x\n",readl(DDR_PHY_BASE+4*0x162));
                                return;
                        }
                }

                printf("\n");
                printf("A:\n");
                ddr_test_phy_dq_channel(0,1);
                printf("\n");
                printf("B:\n");
                ddr_test_phy_dq_channel(1,1);

                printf("reg read default:\n");
                printf("odt_pd:%x\n",readl(DDR_PHY_BASE+4*0x140));
                printf("odt_pu:%x\n",readl(DDR_PHY_BASE+4*0x141));
                printf("drvcmd:%x\n",readl(DDR_PHY_BASE+4*0x130));
                printf("drval_pd:%x\n",readl(DDR_PHY_BASE+4*0x142));
                printf("drvbl_pd:%x\n",readl(DDR_PHY_BASE+4*0x162));


        }
}


/*
        测试dq  vref 眼图。
        abchannel ： 0 a通道  1 b通道
        dqx DQ编号
        setdqs：设置DQS值
        txrx：暂时未用
*/
void ddr_test_vref_dqx_eye(int dqx,int setdqs,int abchannel,int txrx)
{
        int times,i,j,rt,regval;
        static int min=0,max=0,en=0;
	int DQxRxOFFSET[]={0x2,0x4,0x6,0x8,0xa,0xc,0xe,0x10,0x17,0x19,0x1b,0x1d,0x1f,0x21,0x23,0x25}; //DQxxx RX寄存器偏移
	unsigned int reg_offset[2] = {0x1c0, 0x220}; //AB通道
        int chl=reg_offset[abchannel];

        writel(setdqs, DDR_PHY_BASE+((0x1c0 + 0x12)*4));
        writel(setdqs, DDR_PHY_BASE+((0x1c0 + 0x27)*4));
        writel(setdqs, DDR_PHY_BASE+((0x1c0 + 0x2a)*4));
        writel(setdqs, DDR_PHY_BASE+((0x1c0 + 0x2b)*4));

        writel(setdqs, DDR_PHY_BASE+((0x220 + 0x12)*4));
        writel(setdqs, DDR_PHY_BASE+((0x220 + 0x27)*4));
        writel(setdqs, DDR_PHY_BASE+((0x220 + 0x2a)*4));
        writel(setdqs, DDR_PHY_BASE+((0x220 + 0x2b)*4));

        printf("\n                  \n ");
        printf("read dqs value:%x \n",readl(DDR_PHY_BASE+((0x1c0 + 0x12)*4)));
        if(abchannel == 0)
                printf("A rx dq%d                         0-0x3f \n",dqx);
        else
                printf("B rx dq%d                         0-0x3f \n",dqx);

        for(i=50;i<=205;i+=12)
        {
                set_rx_vref(i);

                if(i<100)
                printf(" vref:%d  ",i);
                else
                printf(" vref:%d ",i);

                for(regval=0;regval <= 0x3f;regval++)
                {
                        writel(regval,  DDR_PHY_BASE+(chl+DQxRxOFFSET[dqx])*4);

                        rt=ddr_mem_pattern_test_abchannel_bit(abchannel, dqx, dqx);
                        if(!rt)
                        {
                        }
                }
                printf("\n");
        }
        printf("\n\n");
}

typedef unsigned long ul;
typedef unsigned long long ull;
typedef unsigned long volatile ulv;
typedef unsigned char volatile u8v;
typedef unsigned short volatile u16v;

#define UL_ONEBITS 0xffffffff
#define UL_LEN 32
#define CHECKERBOARD1 0x55555555
#define CHECKERBOARD2 0xaaaaaaaa
#define UL_BYTE(x) ((x | x << 8 | x << 16 | x << 24))


/* Global vars - so tests have access to this information */
int use_phys = 0;
off_t physaddrbase = 0;

ulv *BUFA=(volatile unsigned int *)(0x81000000);;
ulv *BUFB=(volatile unsigned int *)(0x82000000);;

/* Function definitions. */

int compare_regions(ulv *bufa, ulv *bufb, size_t count) {
    int r = 0;
    size_t i;
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    off_t physaddr;

    for (i = 0; i < count; i++, p1++, p2++) {

        printf("p1:%x,p2:%x\n",*p1,*p2);
        if (*p1 != *p2) {
            if (use_phys) {
                physaddr = physaddrbase + (i * sizeof(ul));
                printf("FAILURE: 0x%x != 0x%x at physical address "
                        "0x%x.\n",
                        (ul) *p1, (ul) *p2, physaddr);
            } else {
                printf("FAILURE: 0x%x != 0x%x at offset %d.\n",
                        (ul) *p1, (ul) *p2, (ul) (i * sizeof(ul)));
            }
            r = -1;
        }
    }
    return r;
}

/* memtester */
int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    printf("     test_solidbits_comparison 1 \n      ");
    for (j = 0; j < 64; j++) {
        q = (j % 2) == 0 ? UL_ONEBITS : 0;
        //printf("setting %d", j);
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        //printf("testing %d", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\n  OK     \n");
    return 0;
}

void ddr_test(void)
{
                int onoff=-1,times,i,dqs=0,mode=0,cmdRegAddr,chl=0;
                printf("ddr phy parms test ?\n");
                scanf("%d\n",&onoff);

                if(onoff == 1)
                        ddr_test_phy_parms();
                if(onoff == 2)
                        ddr_mem_pattern_test_once();
                if(onoff == 3)//测试设置DQS不同值时，AB通道，所有DQx 0-0x3f时的对错。
                        ddr_test_setDQS_ABchl_dqx();

                if(onoff == 4)
                {
                        ddr_test_phy_dq_bit_set(1, -1, 5);
                        dump_ddrp_per_bit_de_skew();
                }
                if(onoff == 5)
                {
                        printf("test times:\n");
                        scanf("%d",&times);
                        for(i=1; i <= times;i++)
                        {
                                if(ddr_mem_pattern_test_once())
                                        break;
                        }
                        if(i > times)
                                printf("************ no error\n");
                }
                if(onoff == 6)
                {
                        while(1)
                        {
                                if(test_solidbits_comparison(BUFA,BUFB,16*1024))
                                        break;
                        }
                }
                if(onoff == 7)//扫描vref
                {
                        ddr_test_vref();
                }
                if(onoff == 8)
                {
#if 0
                        //扫描 DQ0，AB通道的rx，与VREF的眼图。
                        printf("default dqs value:%x\n",readl(DDR_PHY_BASE+((0x1c0 + 0x12)*4)));
                        printf("input A or B chl(0 a,1 b.),rx dqs ?(0-0x3f or 999 auto test)\n");
                        scanf("%d,%d\n",&chl,&dqs);
                        ddr_test_vref_dqx_eye(0,dqs,chl,0);
#else
                        //扫描 DQ0,DQ8，AB通道的rx，与VREF的眼图。
                        dqs = readl(DDR_PHY_BASE+((0x1c0 + 0x12)*4));
                        printf("default dqs value:%x\n",dqs );
                        printf("input rx dqs ?\n");
                        scanf("%d,%d\n",&dqs);
                        ddr_test_vref_dqx_eye(0,dqs,0,0);
                        ddr_test_vref_dqx_eye(0,dqs,1,0);
                        ddr_test_vref_dqx_eye(8,dqs,0,0);
                        ddr_test_vref_dqx_eye(8,dqs,1,0);
#endif
                }
}

#endif //DDR_PHY_PARAMS_TEST_A1

static void ddrp_set_drv_odt(unsigned short chip)
{
	int odt_pu,odt_pd;
	int drvcmd;
	int drval_pu,drval_pd;
	int drvah_pu,drvah_pd;
	int drvbl_pu,drvbl_pd;
	int drvbh_pu,drvbh_pd;

#ifdef DDR_PHY_PARAMS_TEST_A1
        int odt_ad,odt_au,odt_bd,odt_bu;
        int ald,alu,ahd,ahu,bld,blu,bhd,bhu;

	printf("reg read default:\n");

	printf("odt_pd:%x\n",readl(DDR_PHY_BASE+4*0x140));
	printf("odt_pu:%x\n",readl(DDR_PHY_BASE+4*0x141));
	printf("drvcmd:%x\n",readl(DDR_PHY_BASE+4*0x130));
	printf("drval_pd:%x\n",readl(DDR_PHY_BASE+4*0x142));
	printf("drvbl_pd:%x\n",readl(DDR_PHY_BASE+4*0x162));
#endif

	if (chip == 0x1111){ //A1N
                printf("A1N@V20230825a\n");
		odt_pd = 1;
                odt_pu = 0;
                drvcmd = 0xe;

		drval_pd = 0x14; drval_pu = 0x14;
		drvah_pd = 0x14; drvah_pu = 0x14;
		drvbl_pd = 0x14; drvbl_pu = 0x14;
		drvbh_pd = 0x14; drvbh_pu = 0x14;

                //set AB chl dqs
                writel(0x20, DDR_PHY_BASE+((0x1c0 + 0x12)*4));
                writel(0x20, DDR_PHY_BASE+((0x1c0 + 0x27)*4));
                writel(0x20, DDR_PHY_BASE+((0x1c0 + 0x2a)*4));
                writel(0x20, DDR_PHY_BASE+((0x1c0 + 0x2b)*4));

                ddr_test_phy_dq_bit_set(0, -1, 16); //set a channel dq value

	} else if (chip == 0x5555) { //A1NT
                printf("A1NT@V20230825a\n");
                odt_pd = 3;
                odt_pu = 1;
                drvcmd = 0xe;

                drval_pd = 0x14; drval_pu = 0x14;
                drvah_pd = 0x14; drvah_pu = 0x14;
                drvbl_pd = 0x14; drvbl_pu = 0x14;
                drvbh_pd = 0x14; drvbh_pu = 0x14;

                //set AB chl dqs
                writel(0x20, DDR_PHY_BASE+((0x1c0 + 0x12)*4));
                writel(0x20, DDR_PHY_BASE+((0x1c0 + 0x27)*4));
                writel(0x20, DDR_PHY_BASE+((0x1c0 + 0x2a)*4));
                writel(0x20, DDR_PHY_BASE+((0x1c0 + 0x2b)*4));

                writel(0x21, DDR_PHY_BASE+((0x220 + 0x12)*4));
                writel(0x21, DDR_PHY_BASE+((0x220 + 0x27)*4));
                writel(0x21, DDR_PHY_BASE+((0x220 + 0x2a)*4));
                writel(0x21, DDR_PHY_BASE+((0x220 + 0x2b)*4));

                ddr_test_phy_dq_bit_set(0, -1, 16); //set a channel dq value
                ddr_test_phy_dq_bit_set(1, -1, 15); //set b channel dq value

	} else if (chip == 0x2222) { //A1X

#if 0
                printf("A1X@V20230825c\n");
                odt_pd = 6;
                odt_pu = 7;
                drvcmd = 0xe;

                drval_pd = 0x14; drval_pu = 0x14;
                drvah_pd = 0x14; drvah_pu = 0x14;
                drvbl_pd = 0x14; drvbl_pu = 0x14;
                drvbh_pd = 0x14; drvbh_pu = 0x14;

                //set AB chl dqs 0x21
                writel(0x23, DDR_PHY_BASE+((0x1c0 + 0x12)*4));
                writel(0x23, DDR_PHY_BASE+((0x1c0 + 0x27)*4));
                writel(0x23, DDR_PHY_BASE+((0x1c0 + 0x2a)*4));
                writel(0x23, DDR_PHY_BASE+((0x1c0 + 0x2b)*4));

                writel(0x23, DDR_PHY_BASE+((0x220 + 0x12)*4));
                writel(0x23, DDR_PHY_BASE+((0x220 + 0x27)*4));
                writel(0x23, DDR_PHY_BASE+((0x220 + 0x2a)*4));
                writel(0x23, DDR_PHY_BASE+((0x220 + 0x2b)*4));

                ddr_test_phy_dq_bit_set(0, -1, 17); //set a channel dq value
                ddr_test_phy_dq_bit_set(1, -1, 15); //set b channel dq value

                //setvref
                //A_L
                ddrp_reg_set_range(0x140+0x07, 0, 8, 138);
                //A_H
                ddrp_reg_set_range(0x140+0x17, 0, 8, 138);
                //B_L
                ddrp_reg_set_range(0x160+0x07, 0, 8, 138);
                //B_H
                ddrp_reg_set_range(0x160+0x17, 0, 8, 138);
#else
                printf("A1X@V20230825b\n");//VERSION 2.
                odt_pd = 5;
                odt_pu = 11;
                drvcmd = 0x1e;

                drval_pd = 0xf; drval_pu = 0xf;
                drvah_pd = 0xf; drvah_pu = 0xf;
                drvbl_pd = 0xf; drvbl_pu = 0xf;
                drvbh_pd = 0xf; drvbh_pu = 0xf;

                //set AB chl dqs 0x21
                writel(0x21, DDR_PHY_BASE+((0x1c0 + 0x12)*4));
                writel(0x21, DDR_PHY_BASE+((0x1c0 + 0x27)*4));
                writel(0x21, DDR_PHY_BASE+((0x1c0 + 0x2a)*4));
                writel(0x21, DDR_PHY_BASE+((0x1c0 + 0x2b)*4));

                writel(0x21, DDR_PHY_BASE+((0x220 + 0x12)*4));
                writel(0x21, DDR_PHY_BASE+((0x220 + 0x27)*4));
                writel(0x21, DDR_PHY_BASE+((0x220 + 0x2a)*4));
                writel(0x21, DDR_PHY_BASE+((0x220 + 0x2b)*4));

                ddr_test_phy_dq_bit_set(0, -1, 15); //set a channel dq value 14.
                ddr_test_phy_dq_bit_set(1, -1, 13); //set b channel dq value 14.

                //setvref
                //A_L
                ddrp_reg_set_range(0x140+0x07, 0, 8, 160);
                //A_H
                ddrp_reg_set_range(0x140+0x17, 0, 8, 160);
                //B_L
                ddrp_reg_set_range(0x160+0x07, 0, 8, 160);
                //B_H
                ddrp_reg_set_range(0x160+0x17, 0, 8, 160);
#endif

	} else if (chip == 0x3333) {  //A1L
		printf("A1L@V20230825a\n");
                odt_pd = 1;
                odt_pu = 1;
                drvcmd = 0xe;

                drval_pd = 0x14; drval_pu = 0x14;
                drvah_pd = 0x14; drvah_pu = 0x14;
                drvbl_pd = 0x14; drvbl_pu = 0x14;
                drvbh_pd = 0x14; drvbh_pu = 0x14;

                //set AB chl dqs
                writel(0x1c, DDR_PHY_BASE+((0x1c0 + 0x12)*4));
                writel(0x1c, DDR_PHY_BASE+((0x1c0 + 0x27)*4));
                writel(0x1c, DDR_PHY_BASE+((0x1c0 + 0x2a)*4));
                writel(0x1c, DDR_PHY_BASE+((0x1c0 + 0x2b)*4));

                writel(0x1c, DDR_PHY_BASE+((0x220 + 0x12)*4));
                writel(0x1c, DDR_PHY_BASE+((0x220 + 0x27)*4));
                writel(0x1c, DDR_PHY_BASE+((0x220 + 0x2a)*4));
                writel(0x1c, DDR_PHY_BASE+((0x220 + 0x2b)*4));

                ddr_test_phy_dq_bit_set(0, -1, 0xe); //set a channel dq value
                ddr_test_phy_dq_bit_set(1, -1, 0xe); //set b channel dq value

	}else if (chip == 0x4444) {  //A1A
		printf("A1A@V20230825a\n");
                odt_pd = 7;
                odt_pu = 12;
                drvcmd = 0xe;

                drval_pd = 0x14; drval_pu = 0x14;
                drvah_pd = 0x14; drvah_pu = 0x14;
                drvbl_pd = 0x14; drvbl_pu = 0x14;
                drvbh_pd = 0x14; drvbh_pu = 0x14;

                //set AB chl dqs
                writel(0x23, DDR_PHY_BASE+((0x1c0 + 0x12)*4));
                writel(0x23, DDR_PHY_BASE+((0x1c0 + 0x27)*4));
                writel(0x23, DDR_PHY_BASE+((0x1c0 + 0x2a)*4));
                writel(0x23, DDR_PHY_BASE+((0x1c0 + 0x2b)*4));

                writel(0x23, DDR_PHY_BASE+((0x220 + 0x12)*4));
                writel(0x23, DDR_PHY_BASE+((0x220 + 0x27)*4));
                writel(0x23, DDR_PHY_BASE+((0x220 + 0x2a)*4));
                writel(0x23, DDR_PHY_BASE+((0x220 + 0x2b)*4));

                ddr_test_phy_dq_bit_set(0, -1, 20); //set a channel dq value
                ddr_test_phy_dq_bit_set(1, -1, 20); //set b channel dq value

                //setvref
                //A_L
                ddrp_reg_set_range(0x140+0x07, 0, 8, 150);
                //A_H
                ddrp_reg_set_range(0x140+0x17, 0, 8, 150);
                //B_L
                ddrp_reg_set_range(0x160+0x07, 0, 8, 150);
                //B_H
                ddrp_reg_set_range(0x160+0x17, 0, 8, 150);

	}  else {
		/*Resered for another A1 chip*/
		printf("A1 unknow ddr\n");
                odt_pd = 1;
                odt_pu = 0;
                drvcmd = 0xe;

		drval_pd = 0x14; drval_pu = 0x14;
		drvah_pd = 0x14; drvah_pu = 0x14;
		drvbl_pd = 0x14; drvbl_pu = 0x14;
		drvbh_pd = 0x14; drvbh_pu = 0x14;
	}

#if 0
	printf("input drv,odt(odt_pd,odt_pu,drvcmd):\n");
	if (3 == scanf("%d,%d,%d\n",
				&odt_pd, &odt_pu,
				&drvcmd)) {
		;
	} else {
	}
	printf("input drv,odt(drval_pd,drval_pu,drvah_pd,drvah_pu):\n");
	if (4 == scanf("%d,%d,%d,%d\n",
				&drval_pd, &drval_pu,
				&drvah_pd, &drvah_pu)) {
		;
	} else {
	}
	printf("input drv,odt(drvbl_pd,drvbl_pu,drvbh_pd,drvbh_pu):\n");
	if (4 == scanf("%d,%d,%d,%d\n",
				&drvbl_pd, &drvbl_pu,
				&drvbh_pd, &drvbh_pu)) {
		;
	} else {
	}

	printf("dump drv,odt:\n"
			"odt_pd = %x, odt_pu = %x\n"
			"drvcmd = %x\n"
			"drval_pd = %x, drval_pu = %x\n"
			"drvah_pd = %x, drvah_pu = %x\n"
			"drvbl_pd = %x, drvbl_pu = %x\n"
			"drvbh_pd = %x, drvbh_pu = %x\n",
			odt_pd, odt_pu,
			drvcmd,
			drval_pd, drval_pu,
			drvah_pd, drvah_pu,
			drvbl_pd, drvbl_pu,
			drvbh_pd, drvbh_pu
		  );
#endif

#ifdef DDR_PHY_PARAMS_TEST_A1

        odt_ad=odt_bd=odt_pd;
        odt_au=odt_bu=odt_pu;



	printf("input drv,odt(&odt_ad,&odt_au,&odt_bd,&odt_bu,drvcmd):\n");
	if (5 == scanf("%d,%d,%d,%d,%d\n",
				&odt_ad,&odt_au,&odt_bd,&odt_bu,
				&drvcmd)) {
		;
	} else {
	}
	printf("input drv,odt(drval_pd,drval_pu,drvah_pd,drvah_pu):\n");
	if (4 == scanf("%d,%d,%d,%d\n",
				&drval_pd, &drval_pu,
				&drvah_pd, &drvah_pu)) {
		;
	} else {
	}
	printf("input drv,odt(drvbl_pd,drvbl_pu,drvbh_pd,drvbh_pu):\n");
	if (4 == scanf("%d,%d,%d,%d\n",
				&drvbl_pd, &drvbl_pu,
				&drvbh_pd, &drvbh_pu)) {
		;
	} else {
	}

	printf("dump drv,odt:\n"
			"%x,%x,%x,%x\n"
			"drvcmd = %x\n"
			"drval_pd = %x, drval_pu = %x\n"
			"drvah_pd = %x, drvah_pu = %x\n"
			"drvbl_pd = %x, drvbl_pu = %x\n"
			"drvbh_pd = %x, drvbh_pu = %x\n",
                        odt_ad,odt_au,odt_bd,odt_bu,
			drvcmd,
			drval_pd, drval_pu,
			drvah_pd, drvah_pu,
			drvbl_pd, drvbl_pu,
			drvbh_pd, drvbh_pu
		  );

        //odt
        //pull down
        writel(odt_ad, DDR_PHY_BASE+4*0x140);//a
        writel(odt_ad, DDR_PHY_BASE+4*0x150);

        //pull up
        writel(odt_au, DDR_PHY_BASE+4*0x141);//a
        writel(odt_au, DDR_PHY_BASE+4*0x151);
        //pull down
        writel(odt_bd, DDR_PHY_BASE+4*0x160);//b
        writel(odt_bd, DDR_PHY_BASE+4*0x170);
        //pull up
        writel(odt_bu, DDR_PHY_BASE+4*0x161);//b
        writel(odt_bu, DDR_PHY_BASE+4*0x171);

	//driver
	writel(drvcmd, DDR_PHY_BASE+4*0x130);
	writel(drvcmd, DDR_PHY_BASE+4*0x131);
	writel(drvcmd, DDR_PHY_BASE+4*0x132);
	writel(drvcmd, DDR_PHY_BASE+4*0x133);

	writel(drval_pd, DDR_PHY_BASE+4*0x142);
	writel(drval_pu, DDR_PHY_BASE+4*0x143);
	writel(drvah_pd, DDR_PHY_BASE+4*0x152);
	writel(drvah_pu, DDR_PHY_BASE+4*0x153);

	writel(drvbl_pd, DDR_PHY_BASE+4*0x162);
	writel(drvbl_pu, DDR_PHY_BASE+4*0x163);
	writel(drvbh_pd, DDR_PHY_BASE+4*0x172);
	writel(drvbh_pu, DDR_PHY_BASE+4*0x173);

	printf("reg read:\n");

	printf("odt_apd:%x\n",readl(DDR_PHY_BASE+4*0x140));
	printf("odt_apu:%x\n",readl(DDR_PHY_BASE+4*0x141));
        printf("odt_bpd:%x\n",readl(DDR_PHY_BASE+4*0x160));
        printf("odt_bpu:%x\n",readl(DDR_PHY_BASE+4*0x161));
	printf("drvcmd:%x\n",readl(DDR_PHY_BASE+4*0x130));
	printf("drval_pd:%x\n",readl(DDR_PHY_BASE+4*0x142));
	printf("drvbl_pd:%x\n",readl(DDR_PHY_BASE+4*0x162));

#if 0

        //odt
        //pull down
        writel(odt_ad, DDR_PHY_BASE+4*0x140);//a
        writel(odt_ad, DDR_PHY_BASE+4*0x150);
        //pull up
        writel(odt_au, DDR_PHY_BASE+4*0x141);//a
        writel(odt_au, DDR_PHY_BASE+4*0x151);

        //pull down
        writel(odt_bd, DDR_PHY_BASE+4*0x160);//b
        writel(odt_bd, DDR_PHY_BASE+4*0x170);
        //pull up
        writel(odt_bu, DDR_PHY_BASE+4*0x161);//b
        writel(odt_bu, DDR_PHY_BASE+4*0x171);
#endif
#else


	//odt
	//pull down
	writel(odt_pd, DDR_PHY_BASE+4*0x140);
	writel(odt_pd, DDR_PHY_BASE+4*0x150);
	writel(odt_pd, DDR_PHY_BASE+4*0x160);
	writel(odt_pd, DDR_PHY_BASE+4*0x170);
	//pull up
	writel(odt_pu, DDR_PHY_BASE+4*0x141);
	writel(odt_pu, DDR_PHY_BASE+4*0x151);
	writel(odt_pu, DDR_PHY_BASE+4*0x161);
	writel(odt_pu, DDR_PHY_BASE+4*0x171);

	//driver
	writel(drvcmd, DDR_PHY_BASE+4*0x130);
	writel(drvcmd, DDR_PHY_BASE+4*0x131);
	writel(drvcmd, DDR_PHY_BASE+4*0x132);
	writel(drvcmd, DDR_PHY_BASE+4*0x133);

	writel(drval_pd, DDR_PHY_BASE+4*0x142);
	writel(drval_pu, DDR_PHY_BASE+4*0x143);
	writel(drvah_pd, DDR_PHY_BASE+4*0x152);
	writel(drvah_pu, DDR_PHY_BASE+4*0x153);

	writel(drvbl_pd, DDR_PHY_BASE+4*0x162);
	writel(drvbl_pu, DDR_PHY_BASE+4*0x163);
	writel(drvbh_pd, DDR_PHY_BASE+4*0x172);
	writel(drvbh_pu, DDR_PHY_BASE+4*0x173);
#endif

}
static void ddr_param_write(ddr3_param_t *ddr3_param)
{
    if(ddr3_param)
    {
        //odt
        //pull down
        writel(ddr3_param->params[odt_pd], DDR_PHY_BASE+4*0x140);
        writel(ddr3_param->params[odt_pd], DDR_PHY_BASE+4*0x150);
        writel(ddr3_param->params[odt_pd], DDR_PHY_BASE+4*0x160);
        writel(ddr3_param->params[odt_pd], DDR_PHY_BASE+4*0x170);

        //pull up
        writel(ddr3_param->params[odt_pu], DDR_PHY_BASE+4*0x141);
        writel(ddr3_param->params[odt_pu], DDR_PHY_BASE+4*0x151);
        writel(ddr3_param->params[odt_pu], DDR_PHY_BASE+4*0x161);
        writel(ddr3_param->params[odt_pu], DDR_PHY_BASE+4*0x171);

        //driver cmd
        writel(ddr3_param->params[drvcmd_pd], DDR_PHY_BASE+4*0x130);
        writel(ddr3_param->params[drvcmd_pu], DDR_PHY_BASE+4*0x131);

        //driver cmd ck
        writel(ddr3_param->params[drvcmdck_pd], DDR_PHY_BASE+4*0x132);
        writel(ddr3_param->params[drvcmdck_pu], DDR_PHY_BASE+4*0x133);

        //driver dq
        writel(ddr3_param->params[drvalah_pd], DDR_PHY_BASE+4*0x142);
        writel(ddr3_param->params[drvalah_pu], DDR_PHY_BASE+4*0x143);
        writel(ddr3_param->params[drvalah_pd], DDR_PHY_BASE+4*0x152);
        writel(ddr3_param->params[drvalah_pu], DDR_PHY_BASE+4*0x153);

        writel(ddr3_param->params[drvblbh_pd], DDR_PHY_BASE+4*0x162);
        writel(ddr3_param->params[drvblbh_pu], DDR_PHY_BASE+4*0x163);
        writel(ddr3_param->params[drvblbh_pd], DDR_PHY_BASE+4*0x172);
        writel(ddr3_param->params[drvblbh_pu], DDR_PHY_BASE+4*0x173);

        //set AB chl dqs
        writel(ddr3_param->params[dqsA], DDR_PHY_BASE+((0x1c0 + 0x12)*4));
        writel(ddr3_param->params[dqsA], DDR_PHY_BASE+((0x1c0 + 0x27)*4));
        writel(ddr3_param->params[dqsA], DDR_PHY_BASE+((0x1c0 + 0x2a)*4));
        writel(ddr3_param->params[dqsA], DDR_PHY_BASE+((0x1c0 + 0x2b)*4));

        writel(ddr3_param->params[dqsB], DDR_PHY_BASE+((0x220 + 0x12)*4));
        writel(ddr3_param->params[dqsB], DDR_PHY_BASE+((0x220 + 0x27)*4));
        writel(ddr3_param->params[dqsB], DDR_PHY_BASE+((0x220 + 0x2a)*4));
        writel(ddr3_param->params[dqsB], DDR_PHY_BASE+((0x220 + 0x2b)*4));

        ddr_test_phy_dq_bit_set(0, -1, ddr3_param->params[dqA]); //set a channel dq value
        ddr_test_phy_dq_bit_set(1, -1, ddr3_param->params[dqB]); //set b channel dq value

        //setvref
        //A_L
        ddrp_reg_set_range(0x140+0x07, 0, 8, ddr3_param->params[vref]);
        //A_H
        ddrp_reg_set_range(0x140+0x17, 0, 8, ddr3_param->params[vref]);
        //B_L
        ddrp_reg_set_range(0x160+0x07, 0, 8, ddr3_param->params[vref]);
        //B_H
        ddrp_reg_set_range(0x160+0x17, 0, 8, ddr3_param->params[vref]);

#ifdef DEBUG_DDR_EFUSE_PARAM
        printf("reg dump:\n");
        printf("-------------------------------------------------------\n");

        printf("data A io ODT DQ[7:0]  pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x140));
        printf("data A io ODT DQ[7:0]  pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x141));
        printf("data A io ODT DQ[15:8] pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x150));
        printf("data A io ODT DQ[15:8] pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x151));
        printf("data B io ODT DQ[7:0]  pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x160));
        printf("data B io ODT DQ[7:0]  pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x161));
        printf("data B io ODT DQ[15:8] pull_down               = 0x%x\n", readl(DDR_PHY_BASE+4*0x170));
        printf("data B io ODT DQ[15:8] pull_up                 = 0x%x\n", readl(DDR_PHY_BASE+4*0x171));

        printf("-------------------------------------------------------\n");
        printf("cmd strenth pull_down                = 0x%x\n", readl(DDR_PHY_BASE+4*0x130));
        printf("cmd strenth pull_up                  = 0x%x\n", readl(DDR_PHY_BASE+4*0x131));
        printf("clk strenth pull_down                = 0x%x\n", readl(DDR_PHY_BASE+4*0x132));
        printf("clk strenth pull_up                  = 0x%x\n", readl(DDR_PHY_BASE+4*0x133));
        printf("-------------------------------------------------------\n");

        printf("data A io strenth DQ[7:0]  pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x142));
        printf("data A io strenth DQ[7:0]  pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x143));
        printf("data A io strenth DQ[15:8] pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x152));
        printf("data A io strenth DQ[15:8] pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x153));
        printf("data B io strenth DQ[7:0]  pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x162));
        printf("data B io strenth DQ[7:0]  pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x163));
        printf("data B io strenth DQ[15:8] pull_down    = 0x%x\n", readl(DDR_PHY_BASE+4*0x172));
        printf("data B io strenth DQ[15:8] pull_up      = 0x%x\n", readl(DDR_PHY_BASE+4*0x173));
        printf("-------------------------------------------------------\n");

        printf("A L VREF = 0x%x\n", readl(DDR_PHY_BASE+4*(0x140+0x07)));
        printf("dqA = 0x%x\n", readl(DDR_PHY_BASE+(0x1c0+0x2)*4));
        printf("dqB = 0x%x\n", readl(DDR_PHY_BASE+(0x220+0x2)*4));
        printf("-------------------------------------------------------\n");

#endif
    }
}


static void ddrc_post_init(void)
{
#ifdef  CONFIG_A1ALL
	ddr_writel(pddr_params->DDRC_REFCNT_VALUE, DDRC_REFCNT);
	if(pddr_params->config_ddr_type_ddr3 == 1)
		mem_remap();
	ddr_writel(pddr_params->DDRC_CTRL_VALUE, DDRC_CTRL);

	ddr_writel(pddr_params->DDRC_CGUC0_VALUE, DDRC_CGUC0);
	ddr_writel(pddr_params->DDRC_CGUC1_VALUE, DDRC_CGUC1);
#else
	ddr_writel(DDRC_REFCNT_VALUE, DDRC_REFCNT);
#ifdef CONFIG_DDR_TYPE_DDR3
	mem_remap();
#endif
	ddr_writel(DDRC_CTRL_VALUE, DDRC_CTRL);

	ddr_writel(DDRC_CGUC0_VALUE, DDRC_CGUC0);
	ddr_writel(DDRC_CGUC1_VALUE, DDRC_CGUC1);
#endif
}

static void ddrc_prev_init(void)
{
	FUNC_ENTER();

	/* DDRC CFG init*/
	/* /\* DDRC CFG init*\/ */
	/* ddr_writel(DDRC_CFG_VALUE, DDRC_CFG); */
#ifdef CONFIG_A1ALL
	/* DDRC timing init*/
	ddr_writel(pddr_params->DDRC_TIMING1_VALUE, DDRC_TIMING(1));
	ddr_writel(pddr_params->DDRC_TIMING2_VALUE, DDRC_TIMING(2));
	ddr_writel(pddr_params->DDRC_TIMING3_VALUE, DDRC_TIMING(3));
	ddr_writel(pddr_params->DDRC_TIMING4_VALUE, DDRC_TIMING(4));
	ddr_writel(pddr_params->DDRC_TIMING5_VALUE, DDRC_TIMING(5));

	/* DDRC memory map configure*/
	ddr_writel(pddr_params->DDRC_MMAP0_VALUE, DDRC_MMAP0);
	ddr_writel(pddr_params->DDRC_MMAP1_VALUE, DDRC_MMAP1);

	/* ddr_writel(DDRC_CTRL_CKE, DDRC_CTRL); */
	ddr_writel((pddr_params->DDRC_CTRL_VALUE) & ~(7 << 12), DDRC_CTRL);
#else
    /* DDRC timing init*/
	ddr_writel(DDRC_TIMING1_VALUE, DDRC_TIMING(1));
	ddr_writel(DDRC_TIMING2_VALUE, DDRC_TIMING(2));
	ddr_writel(DDRC_TIMING3_VALUE, DDRC_TIMING(3));
	ddr_writel(DDRC_TIMING4_VALUE, DDRC_TIMING(4));
	ddr_writel(DDRC_TIMING5_VALUE, DDRC_TIMING(5));

	/* DDRC memory map configure*/
	ddr_writel(DDRC_MMAP0_VALUE, DDRC_MMAP0);
	ddr_writel(DDRC_MMAP1_VALUE, DDRC_MMAP1);

	/* ddr_writel(DDRC_CTRL_CKE, DDRC_CTRL); */
	ddr_writel(DDRC_CTRL_VALUE & ~(7 << 12), DDRC_CTRL);
#endif
	FUNC_EXIT();
}

static enum ddr_type get_ddr_type(void)
{
	int type;
	ddrc_cfg_t ddrc_cfg;
#ifdef CONFIG_A1ALL
	ddrc_cfg.d32 = pddr_params->DDRC_CFG_VALUE;
#else
	ddrc_cfg.d32 = DDRC_CFG_VALUE;
#endif
	switch(ddrc_cfg.b.TYPE){
	case 3:
		type = LPDDR;
		break;
	case 4:
		type = DDR2;
		break;
	case 5:
		type = LPDDR2;
		break;
	case 6:
		type = DDR3;
		break;
	default:
		type = UNKOWN;
		debug("unsupport ddr type!\n");
		ddr_hang();
	}
	return type;
}

static void ddrc_reset_phy(void)
{
	FUNC_ENTER();
	ddr_writel(0xf << 20, DDRC_CTRL);
	mdelay(1);
	ddr_writel(0x8 << 20, DDRC_CTRL);  //dfi_reset_n low for innophy
	mdelay(1);
	FUNC_EXIT();
}

static struct jzsoc_ddr_hook *ddr_hook = NULL;
void register_ddr_hook(struct jzsoc_ddr_hook * hook)
{
	ddr_hook = hook;
}

static void ddrp_pll_init(unsigned int rate)
{
	unsigned int val;
	unsigned int timeout = 1000000;

	/* fbdiv={reg21[0],reg20[7:0]};
	 * pre_div = reg22[4:0];
	 * post_div = 2^reg22[7:5];
	 * fpll_4xclk = fpll_refclk * fbdiv / ( post_div * pre_div);
	 *			  = fpll_refclk * 16 / ( 1 * 4 ) = fpll_refclk * 4;
	 * fpll_1xclk = fpll_2xclk / 2 = fpll_refclk;
	 * Use: IN:fpll_refclk, OUT:fpll_4xclk */

	/* reg20; bit[7:0]fbdiv M[7:0] */
#if 0
	val = ddr_readl(DDRP_INNOPHY_PLL_FBDIV);
	val &= ~(0xff);
	val |= 0x6a;
	ddr_writel(val, DDRP_INNOPHY_PLL_FBDIV);

	/* reg21; bit[7:2]reserved; bit[1]PLL reset; bit[0]fbdiv M8 */
	//val = ddr_readl(DDRP_INNOPHY_PLL_CTRL);
	//val &= ~(0xff);
	//val |= 0x1a;
	//ddr_writel(val, DDRP_INNOPHY_PLL_CTRL);

	/* reg22; bit[7:5]post_div; bit[4:0]pre_div */
	val = ddr_readl(DDRP_INNOPHY_PLL_PDIV);
	val &= ~(0xff);
	val |= 0x42;
	ddr_writel(val, DDRP_INNOPHY_PLL_PDIV);

	/* reg21; bit[7:2]reserved; bit[1]PLL reset; bit[0]fbdiv M8 */
	val = ddr_readl(DDRP_INNOPHY_PLL_CTRL);
	val &= ~(0xff);
	val |= 0x4;
	ddr_writel(val, DDRP_INNOPHY_PLL_CTRL);

	val = ddr_readl(DDRP_INNOPHY_PLL_PD);
	val &= ~(0xff);
	val |= 0x0;
	ddr_writel(val, DDRP_INNOPHY_PLL_PD);
#endif

	val = ddr_readl(DDRP_INNOPHY_MEM_CFG);
	val &= ~(0xff);
	val |= 0xa;
	ddr_writel(val, DDRP_INNOPHY_MEM_CFG);

	/* reg1f; bit[7:4]reserved; bit[3:0]DQ width
	 * DQ width bit[3:0]:0x1:8bit,0x3:16bit,0x7:24bit,0xf:32bit */
	val = ddr_readl(DDRP_INNOPHY_DQ_WIDTH);
	val &= ~(0xf);
#ifdef CONFIG_A1ALL
	if (pddr_params->config_ddr_dw32 == 1)
		val |= 0xf; /*0x0f:32bit*/
	else
		val |= DDRP_DQ_WIDTH_DQ_H | DDRP_DQ_WIDTH_DQ_L; /* 0x3:16bit */

#else
#if CONFIG_DDR_DW32
	val |= 0xf; /* 0x0f:32bit */
#else
	val |= DDRP_DQ_WIDTH_DQ_H | DDRP_DQ_WIDTH_DQ_L; /* 0x3:16bit */
#endif
#endif
	ddr_writel(val, DDRP_INNOPHY_DQ_WIDTH);

#if 0
	/* reg1; bit[7:5]reserved; bit[4]burst type; bit[3:2]reserved; bit[1:0]DDR mode */
	val = ddr_readl(DDRP_INNOPHY_MEM_CFG);
	val &= ~(0xff);
#ifdef CONFIG_DDR_TYPE_DDR3
	val |= 0x10; /* burst 8 */
#elif defined(CONFIG_DDR_TYPE_DDR2)
	val |= 0x11; /* burst 8 ddr2 */
#endif
	ddr_writel(val, DDRP_INNOPHY_MEM_CFG);
#endif

	/* bit[3]reset digital core; bit[2]reset analog logic; bit[0]Reset Initial status
	 * other reserved */
	val = ddr_readl(DDRP_INNOPHY_INNO_PHY_RST);
	val &= ~(0xff);
	val |= 0x0d;
	ddr_writel(val, DDRP_INNOPHY_INNO_PHY_RST);

	/* CWL */
	val = ddr_readl(DDRP_INNOPHY_CWL);
	val &= ~(0xf);
#ifdef CONFIG_A1ALL
	val |= pddr_params->DDRP_CWL_VALUE;
#else
	val |= DDRP_CWL_VALUE;
#endif
	ddr_writel(val, DDRP_INNOPHY_CWL);

	/* CL */
	val = ddr_readl(DDRP_INNOPHY_CL);
	val &= ~(0xf);
#ifdef CONFIG_A1ALL
	val |= pddr_params->DDRP_CL_VALUE;
#else
	val |= DDRP_CL_VALUE;
#endif
	ddr_writel(val, DDRP_INNOPHY_CL);

	/* AL */
	val = ddr_readl(DDRP_INNOPHY_AL);
	val &= ~(0xf);
	val = 0x0;
	ddr_writel(val, DDRP_INNOPHY_AL);

#if 0//def DDR_PHY_PARAMS_TEST_A1
#define TESTINVDLY_ADDR (DDR_PHY_BASE+4*0x8)
        int invdelaysel;
	printf("\n ddr test invdelaysel:%x input:\n",readl(TESTINVDLY_ADDR));

	if (1 == scanf("%d\n",&invdelaysel)) {
                printf("invdelaysel %d.\n",invdelaysel);
                invdelaysel= invdelaysel | 0X10;
                writel(invdelaysel, TESTINVDLY_ADDR);
                printf("\n2 invdelaysel:%x\n\n",readl(TESTINVDLY_ADDR));

	}else{};

#endif
       val = ddr_readl(DDRP_INNOPHY_PLL_PD);
       val &= ~(0xff);
       val |= 0x0;
       ddr_writel(val, DDRP_INNOPHY_PLL_PD);

       /*Edwin.wen@20230206 A1 inno phy pll set with different rate */
        if(rate > 162500000 && (rate <= 325000000))
        {
                val = ddr_readl(DDRP_INNOPHY_PLL_CTRL);
                val = val | (0x1 << 7);
                ddr_writel(val, DDRP_INNOPHY_PLL_CTRL);

                val = ddr_readl(DDRP_INNOPHY_PLL_PD);
                val = val |( 0x2 << 5);
                ddr_writel(val, DDRP_INNOPHY_PLL_PD);

        }else if((rate > 325000000) && (rate <= 650000000 ))
        {
                val = ddr_readl(DDRP_INNOPHY_PLL_CTRL);
                val = val | (0x1 << 7);
                ddr_writel(val, DDRP_INNOPHY_PLL_CTRL);

                val = ddr_readl(DDRP_INNOPHY_PLL_PD);
                val = val |( 0x1 << 5);
                ddr_writel(val, DDRP_INNOPHY_PLL_PD);
        }else
        {}//650Mhz-900Mhz default 0.

	while(!(ddr_readl(DDRP_INNOPHY_PLL_LOCK) & 1 << 2) && --timeout);
	if(!timeout) {
		printf("DDRP_INNOPHY_PLL_LOCK time out!!!\n");
		hang();
	}
	//mdelay(20);
    //dump_ddrp_driver_strength_register();
}

static void ddrp_software_writeleveling_tx(void)
{
    int k,i,j,a,b,middle;
    int ret = -1;
    unsigned int addr = 0xa2000000,val;
    unsigned int reg_val;
    unsigned int ddbuf[8] = {0};
    unsigned int pass_invdelay_total[64] = {0};
    unsigned int pass_cnt = 0;
    unsigned int offset_addr_tx[4] = {0xb3011484,0xb30114b0,0xb3011684,0xb30116b0}; //A_DQ0 , A_DQ8, B_DQ0, B_DQ8 rx invdelay control register

    //ddr_writel((0x8|ddr_readl(DDRP_INNOPHY_TRAINING_CTRL)), DDRP_INNOPHY_TRAINING_CTRL);

    writel(0x8, 0xb30114a4); //A_DQS0
    writel(0x8, 0xb30114d0); //A_DQS1
    writel(0x8, 0xb30116a4); //B_DQS0
    writel(0x8, 0xb30116d0); //B_DQS1

    writel(0x8, 0xb30114a8); //A_DQSB0
    writel(0x8, 0xb30114d4); //A_DQSB1
    writel(0x8, 0xb30116a8); //B_DQSB0
    writel(0x8, 0xb30116d4); //B_DQSB1

    for (j=0; j<=0x3; j++) {
        for(k=0; k<=0x7; k++) { // DQ0~DQ7
            pass_cnt = 0;
            for(i=0; i<=0x4; i++) { //invdelay from 0 to 3f

                reg_val = readl(offset_addr_tx[j]+k*4) & ~0x3f;
                writel((i | reg_val) , (offset_addr_tx[j]+k*4)); // set invdelay, i is invdelay value , offset_addr_tx[j]+k*2 is the addr

                //mdelay(5);
			    for(a = 0; a < 0xff; a++) {

                    val = 0x12345678 + (j + k + i + a)*4;

                    *(volatile unsigned int *)(addr + a * 4) = val;

                    if(((*(volatile unsigned int *)(addr + a * 4)) & 0xff000000) != (val & 0xff000000) && ((*(volatile unsigned int *)(addr + a * 4)) & 0x00ff0000) != (val & 0x00ff0000) && ((*(volatile unsigned int *)(addr + a * 4)) & 0x0000ff00) != (val & 0x0000ff00) && ((*(volatile unsigned int *)(addr + a * 4)) & 0x000000ff) != (val & 0x000000ff)) {
                        printf(" AL fail VALUE 0x%x  sVALUE 0x%x error \n", val ,(*(volatile unsigned int *)(addr + i * 4)));
                        break;
                    }
                }

                if(a == 0xff) {
                    pass_invdelay_total[pass_cnt] = i;
                    pass_cnt++;
                }
            }

            middle = pass_cnt / 2;
            writel((pass_invdelay_total[middle] | reg_val), (offset_addr_tx[j]+k*4));    //this means step 3
            printf("\nTX  chan%d DQ%d small_value = %d big value %d  size = 0x%x middle = %x\n",j, k, pass_invdelay_total[0], pass_invdelay_total[pass_cnt-1], (pass_invdelay_total[pass_cnt-1] - pass_invdelay_total[0]), pass_invdelay_total[middle]);
        }
    }
}
static void ddrp_software_writeleveling_rx(void)
{
    int k,i,j,a,b,c,d,e,middle;
    int ret = -1;
    unsigned int addr = 0xa1000000;
    unsigned int val[0xff],value,tmp;
    unsigned int reg_val;
    unsigned int ddbuf[8] = {0};
    unsigned int pass_invdelay_total[64] = {0};
    unsigned int pass_dqs_total[64] = {0};
    unsigned int pass_dqs_value[64] = {0};
    unsigned int dq[8],middle_dq=0,max_dqs = 0;
    unsigned int pass_cnt = 0, pass_cnt_dqs = 0;
    unsigned int offset_addr_rx[4] = {0xb3011504,0xb3011530,0xb3011704,0xb3011730}; //A_DQ0 , A_DQ8, B_DQ0, B_DQ8 rx invdelay control register
    unsigned int offset_addr_dqs[4] = {0xb3011524,0xb3011550,0xb3011724,0xb3011750}; //A_DQS0 , A_DQS1, B_DQS0, B_DQS1 rx invdelay control register


    ddr_writel((0x8|ddr_readl(DDRP_INNOPHY_TRAINING_CTRL)), DDRP_INNOPHY_TRAINING_CTRL);
#if 1

    /* this is set default value */
    value = 0x1f;
	ddr_writel(value, DDR_PHY_OFFSET + (0x120+0x20)*4);///DM0
	ddr_writel(value, DDR_PHY_OFFSET + (0x120+0x2b)*4);///DM1

    ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+0x20)*4);///DM0
	ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+0x2b)*4);///DM1
    //RX DQ
    for (i = 0x21; i <= 0x28;i++) {
	    ddr_writel(value, DDR_PHY_OFFSET + (0x120+i)*4);//DQ0-DQ15
	    ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+i)*4);//DQ15-DQ31
    }

    //DQS
	ddr_writel(value, DDR_PHY_OFFSET + (0x120+0x29)*4);///DQS0
	ddr_writel(value, DDR_PHY_OFFSET + (0x120+0x34)*4);///DQS1

	ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+0x29)*4);///DQS0
	ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+0x34)*4);///DQS1
    for (i = 0x2c; i <= 0x33;i++) {
	    ddr_writel(value, DDR_PHY_OFFSET + (0x120+i)*4);//DQ0-DQ15
	    ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+i)*4);//DQ15-DQ31
    }

#endif
#if 1
    for(j=0; j<=0x3; j++) {
        for(k=0; k<=0x7; k++) { // DQ0~DQ7
            pass_cnt = 0;

            for(i=0x20; i<=0x3f; i++) { //invdelay from 0 to 63
                reg_val = 0;
                reg_val = readl(offset_addr_rx[j]+k*4) & ~0x3f;
                writel((i | reg_val), (offset_addr_rx[j]+k*4)); // set invdelay, i is invdelay value , offset_addr_rx[j]+k*4 is the addr

                //mdelay(5);
                for(a = 0; a < 0xff; a ++) {
                    val[a] = 0x12345678 + (j + k + i + a)*4;
                    *(volatile unsigned int *)(addr + a * 4) = val[a];
                    *(volatile unsigned int *)(addr + (a + 0xff) * 4) = val[a];
                    *(volatile unsigned int *)(addr + (a + 0xff*2) * 4) = val[a];
                    *(volatile unsigned int *)(addr + (a + 0xff*3) * 4) = val[a];
                    *(volatile unsigned int *)(addr + (a + 0xff*4) * 4) = val[a];
                    *(volatile unsigned int *)(addr + (a + 0xff*5) * 4) = val[a];
                    *(volatile unsigned int *)(addr + (a + 0xff*6) * 4) = val[a];
                }

                //flush_dcache_range(addr, addr+(0xff*4));
		        //flush_l2cache_range(addr, addr+(0xff*4));

                for(a = 0; a < 0xff; a ++) {
                    if(((*(volatile unsigned int *)(addr + a * 4)) != val[a]) && ((*(volatile unsigned int *)(addr + (a + 0xff*3) * 4)) != val[a])) {
                        //printf("want = 0x%x value1 = 0x%x  value2 = 0x%x value3 = 0x%x value4 = 0x%x \n", val1, tmp, tmp1, tmp2, tmp3);
                        break;
                    }
                }

                if(a == 0xff) {
                    pass_invdelay_total[pass_cnt] = i;
                    //printf("\nRX  chan%d DQ%d pass_value = %d \n",j, k, i);
                    pass_cnt++;
                } else {
                    printf("##### this is the big :2222 pass_cnt = %d\n", pass_cnt);
                    break;
                }

            }
            for(i=0x1f; i>=0x0; i--) { //invdelay from 0 to 63
                reg_val = 0;
                reg_val = readl(offset_addr_rx[j]+k*4) & ~0x3f;
                writel((i | reg_val), (offset_addr_rx[j]+k*4)); // set invdelay, i is invdelay value , offset_addr_rx[j]+k*4 is the addr

                //mdelay(5);
                for(a = 0; a < 0xff; a ++) {
                    val[a] = 0x12345678 + (j + k + i + a)*4;
                    *(volatile unsigned int *)(addr + a * 4) = val[a];
                }
                //flush_dcache_range(addr, addr+(0xff*4));
		        //flush_l2cache_range(addr, addr+(0xff*4));

                for(a = 0; a < 0xff; a ++) {
                    if((*(volatile unsigned int *)(addr + a * 4)) != val[a]) {
                        //printf("want = 0x%x value1 = 0x%x  value2 = 0x%x value3 = 0x%x value4 = 0x%x \n", val1, tmp, tmp1, tmp2, tmp3);
                        break;
                    }
                }

                if(a == 0xff) {
                    pass_invdelay_total[pass_cnt] = i;
                    //printf("\nRX  chan%d DQ%d pass_value = %d \n",j, k, i);
                    pass_cnt++;
                } else {
                    printf("##### this is the small:2222 pass_cnt = %d\n", pass_cnt);
                    break;
                }
            }

            for(a=0; a<pass_cnt-1; a++) {
                for(b=0; b<pass_cnt-a-1; b++) {
                    if(pass_invdelay_total[b] > pass_invdelay_total[b+1]) {
                        tmp= pass_invdelay_total[b];
                        pass_invdelay_total[b] = pass_invdelay_total[b+1];
                        pass_invdelay_total[b+1] = tmp;
                    }
                }
            }
            for(a=0; a<pass_cnt;a++) {
                printf("##### pass_invdelay_total[%d] value %d\n", a, pass_invdelay_total[a]);
            }
            printf("##### this is the small:3333\n");
            middle = pass_cnt / 2;
            writel(pass_invdelay_total[middle], (offset_addr_rx[j]+k*4));    //this means step 3
            printf("\nRX  chan%d DQ%d small_value = %d big value %d  size = 0x%x middle = %x\n",j, k, pass_invdelay_total[0], pass_invdelay_total[pass_cnt-1], (pass_invdelay_total[pass_cnt-1] - pass_invdelay_total[0]), pass_invdelay_total[middle]);
        }
    }

#endif
}

static void ddrp_write_leveling_calibration(void)
{
#ifdef CONFIG_A1ALL
	unsigned int timeout;
	if(pddr_params->config_ddr_type_ddr3 == 1)
		timeout = 1000000;
	ddrp_writel_byidx((pddr_params->DDR_MR1_VALUE)&0xff, DDRP_REG_OFFSET_WR_LEVEL1);
	ddrp_writel_byidx((1<<6)|(((pddr_params->DDR_MR1_VALUE)>>8)&0x3f), DDRP_REG_OFFSET_WR_LEVEL2);
#else
#ifdef CONFIG_DDR_TYPE_DDR3
	unsigned int timeout = 1000000;
	ddrp_writel_byidx(DDR_MR1_VALUE&0xff, DDRP_REG_OFFSET_WR_LEVEL1);
	ddrp_writel_byidx((1<<6)|((DDR_MR1_VALUE>>8)&0x3f), DDRP_REG_OFFSET_WR_LEVEL2);
#endif
	ddrp_reg_set_range(DDRP_REG_OFFSET_RAINING_CTRL, 2, 1, 1);
	while (((ddrp_readl_byidx(DDRP_REG_OFFSET_WL_DONE) & 0xf) != 0xf) && timeout--);
	if(!timeout) {
		printf("timeout:INNOPHY_WL_DONE %x\n", ddrp_readl_byidx(DDRP_REG_OFFSET_WL_DONE));
		hang();
	}
	ddrp_reg_set_range(DDRP_REG_OFFSET_RAINING_CTRL, 2, 1, 0);
	printf("ddrp write leveling calibration:\n");
	printf("A chn low,  reg offset %x value %x\n", DDRP_REG_OFFSET_WRLEVEL_RESULT_LOW_A, ddrp_readl_byidx(DDRP_REG_OFFSET_WRLEVEL_RESULT_LOW_A));
	printf("A chn high, reg offset %x value %x\n", DDRP_REG_OFFSET_WRLEVEL_RESULT_HIGH_A, ddrp_readl_byidx(DDRP_REG_OFFSET_WRLEVEL_RESULT_HIGH_A));
	printf("B chn low,  reg offset %x value %x\n", DDRP_REG_OFFSET_WRLEVEL_RESULT_LOW_B, ddrp_readl_byidx(DDRP_REG_OFFSET_WRLEVEL_RESULT_LOW_B));
	printf("B chn high, reg offset %x value %x\n", DDRP_REG_OFFSET_WRLEVEL_RESULT_HIGH_B, ddrp_readl_byidx(DDRP_REG_OFFSET_WRLEVEL_RESULT_HIGH_B));
#endif
}

static void ddrp_hardware_calibration(void)
{
	unsigned int val;
	unsigned int timeout = 1000000;

#ifdef CONFIG_A1A
	ddrp_write_leveling_calibration();
#endif
	ddr_writel(0, DDRP_INNOPHY_TRAINING_CTRL);
	ddr_readl(DDRP_INNOPHY_TRAINING_CTRL);
	debug("ddr training into!\n");
	ddr_writel(1, DDRP_INNOPHY_TRAINING_CTRL);
#ifdef CONFIG_A1ALL
	if (pddr_params->config_ddr_dw32 == 1) {
			do
			{
				val = ddr_readl(DDRP_INNOPHY_CALIB_DONE);
			} while (((val & 0xf) != 0xf) && --timeout);/*32bit ddr mode*/
	} else {
			do
			{
				val = ddr_readl(DDRP_INNOPHY_CALIB_DONE);
			} while (((val & 0xf) != 0x3) && --timeout);/*16bit ddr mode*/
			printf("timeout is %d, val = 0x%x\n", timeout, val);
	}
#else
#if CONFIG_DDR_DW32
	do
	{
		val = ddr_readl(DDRP_INNOPHY_CALIB_DONE);
	} while (((val & 0xf) != 0xf) && --timeout);/*32bit ddr mode*/
#else
	do
	{
		val = ddr_readl(DDRP_INNOPHY_CALIB_DONE);
	} while (((val & 0xf) != 0x3) && --timeout);/*16bit ddr mode*/
	printf("timeout is %d, val = 0x%x\n", timeout, val);
#endif
#endif

	if(!timeout) {
		printf("timeout:INNOPHY_CALIB_DONE %x\n", ddr_readl(DDRP_INNOPHY_CALIB_DONE));
		hang();
	}

	ddr_writel(ddr_readl(DDRP_INNOPHY_TRAINING_CTRL)&(~0x1), DDRP_INNOPHY_TRAINING_CTRL);
	{
		int reg1, reg2;
		printf("ddrp rx hard calibration:\n");
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL_RESULT1);
		printf("ddrp rx hard calibration: reg1 is 0x%x\n",reg1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL_RESULT2);
		printf("ddrp rx hard calibration:reg2 is 0x%x\n", reg2);
		printf("CALIB_AL: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH_RESULT1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH_RESULT2);
		printf("CALIB_AH: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL_RESULT1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL_RESULT2);
		printf("CALIB_BL: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH_RESULT1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH_RESULT2);
		printf("CALIB_BH: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
	}
}

static void ddr_phy_cfg_driver_odt(void)
{
	/* ddr phy driver strength and  ODT config */

#ifdef CONFIG_T40A
	/* cmd */
	ddrp_reg_set_range(0xb0, 0, 5, 0x15);
	ddrp_reg_set_range(0xb1, 0, 5, 0x15);
	/* ck */
	ddrp_reg_set_range(0xb2, 0, 5, 0x18); //pull down
	ddrp_reg_set_range(0xb3, 0, 5, 0x18);//pull up

	/* DQ ODT config */
	u32 dq_odt = 0x6;
	/* Channel A */
	ddrp_reg_set_range(0xc0, 0, 5, dq_odt);
	ddrp_reg_set_range(0xc1, 0, 5, dq_odt);
	ddrp_reg_set_range(0xd0, 0, 5, dq_odt);
	ddrp_reg_set_range(0xd1, 0, 5, dq_odt);
	/* Channel B */
	ddrp_reg_set_range(0xe0, 0, 5, dq_odt);
	ddrp_reg_set_range(0xe1, 0, 5, dq_odt);
	ddrp_reg_set_range(0xf0, 0, 5, dq_odt);
	ddrp_reg_set_range(0xf1, 0, 5, dq_odt);

	/* driver strength config */
    	u32 dq_ds = 0x15;
	ddrp_reg_set_range(0xc2, 0, 5, dq_ds);//pull down
	ddrp_reg_set_range(0xc3, 0, 5, dq_ds);//pull up
	ddrp_reg_set_range(0xd2, 0, 5, dq_ds);
	ddrp_reg_set_range(0xd3, 0, 5, dq_ds);

	ddrp_reg_set_range(0xe2, 0, 5, dq_ds);
	ddrp_reg_set_range(0xe3, 0, 5, dq_ds);
	ddrp_reg_set_range(0xf2, 0, 5, dq_ds);
	ddrp_reg_set_range(0xf3, 0, 5, dq_ds);

#elif defined (CONFIG_T40XP)

	/* cmd */
	ddrp_reg_set_range(0xb0, 0, 5, 0xf);
	ddrp_reg_set_range(0xb1, 0, 5, 0xf);
	/* ck */
	ddrp_reg_set_range(0xb2, 0, 5, 0x11);//pull down
	ddrp_reg_set_range(0xb3, 0, 5, 0x11);//pull up

	/* DQ ODT config */
	u32 dq_odt = 0x3;
	/* Channel A */
	ddrp_reg_set_range(0xc0, 0, 5, dq_odt);
	ddrp_reg_set_range(0xc1, 0, 5, dq_odt);
	ddrp_reg_set_range(0xd0, 0, 5, dq_odt);
	ddrp_reg_set_range(0xd1, 0, 5, dq_odt);
	/* Channel B */
	ddrp_reg_set_range(0xe0, 0, 5, dq_odt);
	ddrp_reg_set_range(0xe1, 0, 5, dq_odt);
	ddrp_reg_set_range(0xf0, 0, 5, dq_odt);
	ddrp_reg_set_range(0xf1, 0, 5, dq_odt);

	/* driver strength config */
	u32 dq_ds = 0x11;
	ddrp_reg_set_range(0xc2, 0, 5, dq_ds);//pull down
	ddrp_reg_set_range(0xc3, 0, 5, dq_ds);//pull up
	ddrp_reg_set_range(0xd2, 0, 5, dq_ds);
	ddrp_reg_set_range(0xd3, 0, 5, dq_ds);

	ddrp_reg_set_range(0xe2, 0, 5, dq_ds);
	ddrp_reg_set_range(0xe3, 0, 5, dq_ds);
	ddrp_reg_set_range(0xf2, 0, 5, dq_ds);
	ddrp_reg_set_range(0xf3, 0, 5, dq_ds);
#elif defined (CONFIG_T40N)
	//driver cmd and ck
	//cmd
	ddrp_reg_set_range(0xb0, 0, 4, 0x5); //0x4
	ddrp_reg_set_range(0xb1, 0, 4, 0x5);
	//ck
	ddrp_reg_set_range(0xb2, 0, 4, 0x5); //pull down,0x5
	ddrp_reg_set_range(0xb3, 0, 4, 0x5);//pull up
	//dq ODT
	//ch A
	ddrp_reg_set_range(0xc0, 0, 5, 0x3);
	ddrp_reg_set_range(0xc1, 0, 5, 0x3);
	ddrp_reg_set_range(0xd0, 0, 5, 0x3);
	ddrp_reg_set_range(0xd1, 0, 5, 0x3);
	//chb B
	ddrp_reg_set_range(0xe0, 0, 4, 0x3);
	ddrp_reg_set_range(0xe1, 0, 4, 0x3);
	ddrp_reg_set_range(0xf0, 0, 4, 0x3);
	ddrp_reg_set_range(0xf1, 0, 4, 0x3);
	// driver strength
	ddrp_reg_set_range(0xc2, 0, 4, 0x5);//pull down
	ddrp_reg_set_range(0xc3, 0, 4, 0x5);//pull up
	ddrp_reg_set_range(0xd2, 0, 4, 0x5);
	ddrp_reg_set_range(0xd3, 0, 4, 0x5);

	ddrp_reg_set_range(0xe2, 0, 4, 0x5);
	ddrp_reg_set_range(0xe3, 0, 4, 0x5);
	ddrp_reg_set_range(0xf2, 0, 4, 0x5);
	ddrp_reg_set_range(0xf3, 0, 4, 0x5);
#endif
}

static void ddr_phy_init(unsigned int rate)
{
	u32 idx;
	u32  val;
#if 0
	/* bit[3]reset digital core; bit[2]reset analog logic; bit[0]Reset Initial status
	 * other reserved */
	val = ddr_readl(DDRP_INNOPHY_INNO_PHY_RST);
	val &= ~(0xff);
	mdelay(2);
	val |= 0x0d;
	ddr_writel(val, DDRP_INNOPHY_INNO_PHY_RST);
	ddr_phy_cfg_driver_odt();

	writel(0xc, DDR_PHY_BASE + (0x15)*4);//default 0x4 cmd
	writel(0x1, DDR_PHY_BASE + (0x16)*4);//default 0x0 ck

	writel(0xc, DDR_PHY_BASE + (0x54)*4);//default 0x4  byte0 dq dll
	writel(0xc, DDR_PHY_BASE + (0x64)*4);//default 0x4  byte1 dq dll
	writel(0xc, DDR_PHY_BASE + (0x84)*4);//default 0x4  byte2 dq dll
	writel(0xc, DDR_PHY_BASE + (0x94)*4);//default 0x4  byte3 dq dll

	writel(0x1, DDR_PHY_BASE + (0x55)*4);//default 0x0  byte0 dqs dll
	writel(0x1, DDR_PHY_BASE + (0x65)*4);//default 0x0  byte1 dqs dll
	writel(0x1, DDR_PHY_BASE + (0x85)*4);//default 0x0  byte2 dqs dll
	writel(0x1, DDR_PHY_BASE + (0x95)*4);//default 0x0  byte3 dqs dll

#ifdef CONFIG_DDR_TYPE_DDR3
	unsigned int i = 0;
	u32 value = 8;
	/* write leveling dq delay time config */
	//cmd
	for (i = 0; i <= 0x1e;i++) {
		ddr_writel(value, DDR_PHY_OFFSET + (0x100+i)*4);///cmd
	}
	//tx DQ
	for (i = 0; i <= 0x8;i++) {
		ddr_writel(value, DDR_PHY_OFFSET + (0x120+i)*4);//DQ0-DQ15
		ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+i)*4);//DQ15-DQ31
	}
	//tx DQ
	for (i = 0xb; i <= 0x13;i++) {
		ddr_writel(value, DDR_PHY_OFFSET + (0x120+i)*4);//DQ0-DQ15
		ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+i)*4);//DQ15-DQ31
	}
	/* rx dqs */
	ddr_writel(5, DDR_PHY_OFFSET + (0x120+0x29)*4);//DQS0-A
	ddr_writel(5, DDR_PHY_OFFSET + (0x1a0+0x29)*4);//DQS0-B
	ddr_writel(5, DDR_PHY_OFFSET + (0x120+0x34)*4);//DQS1-A
	ddr_writel(5, DDR_PHY_OFFSET + (0x1a0+0x34)*4);//DQS1-B
	//enable bypass write leveling
	//open manual per bit de-skew
	//printf("PHY REG-02 :  0x%x \n",readl(0xb3011008));
	writel((readl(0xb3011008))|(0x8), 0xb3011008);
	//printf("PHY REG-02 :  0x%x \n",readl(0xb3011008));

	dump_ddr_phy_cfg_per_bit_de_skew_register();

#ifdef CONFIG_DDR_DEBUG
	//writel(0x73, DDR_PHY_BASE+(0x88*4));
	//writel(0x73, DDR_PHY_BASE+(0x98*4));
	printf("--------inno a dll reg:0x58 = 0x%x\n", readl(DDR_PHY_BASE+(0x58*4)));
	printf("--------inno a dll reg:0x68 = 0x%x\n", readl(DDR_PHY_BASE+(0x68*4)));
	printf("--------inno b dll reg:0x88 = 0x%x\n", readl(DDR_PHY_BASE+(0x88*4)));
	printf("--------inno b dll reg:0x98 = 0x%x\n", readl(DDR_PHY_BASE+(0x98*4)));
#endif
#elif defined(CONFIG_DDR_TYPE_DDR2)
	unsigned int i = 0;
	u32 value = 5;
	/* write leveling dq delay time config */
	//cmd
	for (i = 0; i <= 0x1e;i++) {
		ddr_writel(value, DDR_PHY_OFFSET + (0x100+i)*4);///cmd
	}
	//tx DQ
	for (i = 0; i <= 0x8;i++) {
		ddr_writel(value, DDR_PHY_OFFSET + (0x120+i)*4);//DQ0-DQ15
		ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+i)*4);//DQ15-DQ31
	}
	//tx DQ
	for (i = 0xb; i <= 0x13;i++) {
		ddr_writel(value, DDR_PHY_OFFSET + (0x120+i)*4);//DQ0-DQ15
		ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+i)*4);//DQ15-DQ31
	}
	ddr_writel(2, DDR_PHY_OFFSET + (0x120+0x9)*4);//DQS0-A
	ddr_writel(2, DDR_PHY_OFFSET + (0x1a0+0x9)*4);//DQS0-B
	ddr_writel(2, DDR_PHY_OFFSET + (0x120+0xa)*4);//DQS0B-A
	ddr_writel(2, DDR_PHY_OFFSET + (0x1a0+0xa)*4);//DQS0B-B
	ddr_writel(2, DDR_PHY_OFFSET + (0x120+0x14)*4);//DQS1-A
	ddr_writel(2, DDR_PHY_OFFSET + (0x1a0+0x14)*4);//DQS1-B
	ddr_writel(2, DDR_PHY_OFFSET + (0x120+0x15)*4);//DQS1B-A
	ddr_writel(2, DDR_PHY_OFFSET + (0x1a0+0x15)*4);//DQS1B-B

	writel((readl(0xb3011008))|(0x8), 0xb3011008);
	//printf("PHY REG-02 :  0x%x \n",readl(0xb3011008));
#endif
#endif
	ddrp_pll_init(rate);
}
/*
 * Name     : ddrp_calibration_manual()
 * Function : control the RX DQS window delay to the DQS
 *
 * a_low_8bit_delay	= al8_2x * clk_2x + al8_1x * clk_1x;
 * a_high_8bit_delay	= ah8_2x * clk_2x + ah8_1x * clk_1x;
 *
 * */
static void ddrp_software_calibration(void)
{
	int x, y;
	int w, z;
	int c, o, d;
	unsigned int addr = 0xa1000000, val;
	unsigned int i, n, m = 0;
	unsigned int sel = 0;
	int ret = -1;
	int j, k, q;

	unsigned int d0 = 0, d1 = 0, d2 = 0, d3 = 0, d4 = 0, d5 = 0, d6 = 0, d7 = 0;
	unsigned int ddbuf[8] = {0};
	unsigned short calv[0x3ff] = {0};
	ddrp_reg_set_range(0x2, 1, 1, 1);
	printf("BEFORE A CALIB\n");
	printf("CALIB DELAY AL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL1));
	printf("CALIB DELAY AL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL2));
	printf("CALIB DELAY AH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH1));
	printf("CALIB DELAY AH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH2));
	printf("CALIB DELAY BL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL1));
	printf("CALIB DELAY BL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL2));
	printf("CALIB DELAY BH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH1));
	printf("CALIB DELAY BH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH2));
#if 1
	for(c = 0; c <=0x3ff; c++) {
		ddr_writel((c>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_AL1);
		ddr_writel((c&0x1f)<<3 | ((c>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_AL2);
		ddr_writel((c>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_AH1);
		ddr_writel((c&0x1f)<<3 | ((c>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_AH2);
		unsigned int value = 0x12345678;
		for(i = 0; i < 4 * 1024; i += 8) {
			*(volatile unsigned int *)(addr + (i + 0) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 1) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 2) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 3) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 4) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 5) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 6) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 7) * 4) = value;
		}
		for(i = 0; i < 4 * 1024; i += 8) {
			ddbuf[0] = *(volatile unsigned int *)(addr + (i + 0) * 4);
			ddbuf[1] = *(volatile unsigned int *)(addr + (i + 1) * 4);
			ddbuf[2] = *(volatile unsigned int *)(addr + (i + 2) * 4);
			ddbuf[3] = *(volatile unsigned int *)(addr + (i + 3) * 4);
			ddbuf[4] = *(volatile unsigned int *)(addr + (i + 4) * 4);
			ddbuf[5] = *(volatile unsigned int *)(addr + (i + 5) * 4);
			ddbuf[6] = *(volatile unsigned int *)(addr + (i + 6) * 4);
			ddbuf[7] = *(volatile unsigned int *)(addr + (i + 7) * 4);
			for(q = 0; q < 8; q++) {
				if ((ddbuf[q]&0xffff0000) != (value&0xffff0000)) {
					;//printf("#####################################   high error want 0x%x get 0x%x\n", value, ddbuf[q]);
				}
				if ((ddbuf[q]&0xffff) != (value&0xffff)) {
					//printf("SET AL,AH %x q[%d] fail want 0x%x  get 0x%x \n", c, q, value, ddbuf[q]);
					ret = -1;
					break;
				} else {
					//printf("SET AL,AH %x q[%d] pass want 0x%x  get 0x%x \n", c, q, value, ddbuf[q]);
					//printf("SET %d  AL[%d] pass want 0x%x  get 0x%x \n", c, q, value, ddbuf[q]);
					ret = 0;
				}
			}
			if (ret) {
				break;
			}
		}

		if(i == 4 * 1024) {
			calv[m] = c;
			m++;
			printf("calib a once idx = %d,  value = %x\n", m, c);
		}
	}

	if(!m) {
		printf("####################### AL a calib bypass fail\n");
		ddr_writel(0x1c, DDRP_INNOPHY_CALIB_DELAY_AL1);
		ddr_writel(0x1c, DDRP_INNOPHY_CALIB_DELAY_AH1);
		//		return;
	} else {
		/* choose the middle parameter */
		sel = m * 1 / 2;
		ddr_writel((calv[sel]>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_AL1);
		ddr_writel((calv[sel]&0x1f)<<3 | ((calv[sel]>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_AL2);
		ddr_writel((calv[sel]>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_AH1);
		ddr_writel((calv[sel]&0x1f)<<3 | ((calv[sel]>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_AH2);
	}
	printf("calib a done range = %d, value = %x\n", m, calv[sel]);
	printf("AFTER A CALIB\n");
	printf("CALIB DELAY AL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL1));
	printf("CALIB DELAY AL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL2));
	printf("CALIB DELAY AH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH1));
	printf("CALIB DELAY AH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH2));
	printf("CALIB DELAY BL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL1));
	printf("CALIB DELAY BL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL2));
	printf("CALIB DELAY BH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH1));
	printf("CALIB DELAY BH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH2));
#endif
#ifdef CONFIG_A1ALL
	if (pddr_params->config_ddr_dw32 == 1) {
	#if 1
	m = 0;
	for(c = 0; c < 0x3ff; c ++) {
		ddr_writel((c>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_BL1);
		ddr_writel((c&0x1f)<<3 | ((c>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_BL2);
		ddr_writel((c>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_BH1);
		ddr_writel((c&0x1f)<<3 | ((c>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_BH2);
		unsigned int value = 0xf0f0f0f0;
		for(i = 0; i < 4 * 1024; i += 8) {
			*(volatile unsigned int *)(addr + (i + 0) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 1) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 2) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 3) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 4) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 5) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 6) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 7) * 4) = value;
			//flush_cache((unsigned int *)(addr + i * 4), 32);
		}

		for(i = 0; i < 4 * 1024; i += 8) {
			ddbuf[0] = *(volatile unsigned int *)(addr + (i + 0) * 4);
			ddbuf[1] = *(volatile unsigned int *)(addr + (i + 1) * 4);
			ddbuf[2] = *(volatile unsigned int *)(addr + (i + 2) * 4);
			ddbuf[3] = *(volatile unsigned int *)(addr + (i + 3) * 4);
			ddbuf[4] = *(volatile unsigned int *)(addr + (i + 4) * 4);
			ddbuf[5] = *(volatile unsigned int *)(addr + (i + 5) * 4);
			ddbuf[6] = *(volatile unsigned int *)(addr + (i + 6) * 4);
			ddbuf[7] = *(volatile unsigned int *)(addr + (i + 7) * 4);

			for(q = 0; q < 8; q++) {

				if ((ddbuf[q]&0xffff0000) != (value&0xffff0000)) {
					//printf("SET BL,BH 0x%x q[%d] fail want 0x%x  get 0x%x \n", c, q, value, ddbuf[q]);
					ret = -1;
					break;
				} else {
					//printf("SET BL,BH 0x%x q[%d] pass want 0x%x  get 0x%x \n", c, q, value, ddbuf[q]);
					ret = 0;
				}
			}
			if (ret) {
				break;
			}
		}

		if(i == 4 * 1024) {
			calv[m] = c;
			m++;
			printf("calib b once idx = %d,  value = %x\n", m, c);
		}
	}

	if(!m) {
		printf("####################### AL calib bypass fail\n");
		ddr_writel(0x1c, DDRP_INNOPHY_CALIB_DELAY_BL1);
		ddr_writel(0x1c, DDRP_INNOPHY_CALIB_DELAY_BH1);

		//return;
	} else {
		/* choose the middle parameter */
		sel = m/2;
		ddr_writel((calv[sel]>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_BL1);
		ddr_writel((calv[sel]&0x1f)<<3 | ((calv[sel]>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_BL2);
		ddr_writel((calv[sel]>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_BH1);
		ddr_writel((calv[sel]&0x1f)<<3 | ((calv[sel]>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_BH2);
	}
	printf("calib b done range = %d, value = %x\n", m, calv[sel]);
	printf("AFTER B CALIB\n");

	printf("CALIB DELAY AL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL1));
	printf("CALIB DELAY AL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL2));
	printf("CALIB DELAY AH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH1));
	printf("CALIB DELAY AH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH2));
	printf("CALIB DELAY BL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL1));
	printf("CALIB DELAY BL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL2));
	printf("CALIB DELAY BH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH1));
	printf("CALIB DELAY BH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH2));

	{
		int reg1, reg2;
		printf("ddrp rx soft calibration:\n");
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL2);
		printf("CALIB_AL: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH2);
		printf("CALIB_AH: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL2);
		printf("CALIB_BL: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH2);
		printf("CALIB_BH: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
	}
#endif
	}
#else
#if CONFIG_DDR_DW32
#if 1
	m = 0;
	for(c = 0; c < 0x3ff; c ++) {
		ddr_writel((c>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_BL1);
		ddr_writel((c&0x1f)<<3 | ((c>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_BL2);
		ddr_writel((c>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_BH1);
		ddr_writel((c&0x1f)<<3 | ((c>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_BH2);
		unsigned int value = 0xf0f0f0f0;
		for(i = 0; i < 4 * 1024; i += 8) {
			*(volatile unsigned int *)(addr + (i + 0) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 1) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 2) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 3) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 4) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 5) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 6) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 7) * 4) = value;
			//flush_cache((unsigned int *)(addr + i * 4), 32);
		}

		for(i = 0; i < 4 * 1024; i += 8) {
			ddbuf[0] = *(volatile unsigned int *)(addr + (i + 0) * 4);
			ddbuf[1] = *(volatile unsigned int *)(addr + (i + 1) * 4);
			ddbuf[2] = *(volatile unsigned int *)(addr + (i + 2) * 4);
			ddbuf[3] = *(volatile unsigned int *)(addr + (i + 3) * 4);
			ddbuf[4] = *(volatile unsigned int *)(addr + (i + 4) * 4);
			ddbuf[5] = *(volatile unsigned int *)(addr + (i + 5) * 4);
			ddbuf[6] = *(volatile unsigned int *)(addr + (i + 6) * 4);
			ddbuf[7] = *(volatile unsigned int *)(addr + (i + 7) * 4);

			for(q = 0; q < 8; q++) {

				if ((ddbuf[q]&0xffff0000) != (value&0xffff0000)) {
					//printf("SET BL,BH 0x%x q[%d] fail want 0x%x  get 0x%x \n", c, q, value, ddbuf[q]);
					ret = -1;
					break;
				} else {
					//printf("SET BL,BH 0x%x q[%d] pass want 0x%x  get 0x%x \n", c, q, value, ddbuf[q]);
					ret = 0;
				}
			}
			if (ret) {
				break;
			}
		}

		if(i == 4 * 1024) {
			calv[m] = c;
			m++;
			printf("calib b once idx = %d,  value = %x\n", m, c);
		}
	}

	if(!m) {
		printf("####################### AL calib bypass fail\n");
		ddr_writel(0x1c, DDRP_INNOPHY_CALIB_DELAY_BL1);
		ddr_writel(0x1c, DDRP_INNOPHY_CALIB_DELAY_BH1);

		//return;
	} else {
		/* choose the middle parameter */
		sel = m/2;
		ddr_writel((calv[sel]>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_BL1);
		ddr_writel((calv[sel]&0x1f)<<3 | ((calv[sel]>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_BL2);
		ddr_writel((calv[sel]>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_BH1);
		ddr_writel((calv[sel]&0x1f)<<3 | ((calv[sel]>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_BH2);
	}
	printf("calib b done range = %d, value = %x\n", m, calv[sel]);
	printf("AFTER B CALIB\n");

	printf("CALIB DELAY AL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL1));
	printf("CALIB DELAY AL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL2));
	printf("CALIB DELAY AH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH1));
	printf("CALIB DELAY AH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH2));
	printf("CALIB DELAY BL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL1));
	printf("CALIB DELAY BL 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL2));
	printf("CALIB DELAY BH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH1));
	printf("CALIB DELAY BH 0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH2));

	{
		int reg1, reg2;
		printf("ddrp rx soft calibration:\n");
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL2);
		printf("CALIB_AL: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH2);
		printf("CALIB_AH: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL2);
		printf("CALIB_BL: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH2);
		printf("CALIB_BH: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
	}
#endif
#endif/*end if CONFIG_DDR_DW32 */
#endif/*end if CONFIG_A1ALL*/
}

static void ddrc_dfi_init(enum ddr_type type, int bypass,unsigned short chip,ddr3_param_t *ddr3_param)
{
    int thirdword = 0;
    int reg_value = 0;

    FUNC_ENTER();

#ifdef CONFIG_A1ALL
                if(pddr_params->config_ddr_buswidth32 == 1) {
                        ddr_writel((DDRC_DWCFG_DFI_INIT_START | 1), DDRC_DWCFG); // dfi_init_start high 0x09
                        ddr_writel(1, DDRC_DWCFG); // set buswidth 32bit
                } else {
                        ddr_writel((DDRC_DWCFG_DFI_INIT_START), DDRC_DWCFG); // dfi_init_start high ddr buswidth 16bit
                        ddr_writel(0, DDRC_DWCFG); // set buswidth 16bit
                }
                while(!(ddr_readl(DDRC_DWSTATUS) & DDRC_DWSTATUS_DFI_INIT_COMP)); //polling dfi_init_complete

                udelay(50);
                ddr_writel(0, DDRC_CTRL); //set dfi_reset_n high
                ddr_writel(pddr_params->DDRC_CFG_VALUE, DDRC_CFG);
                udelay(500);
                ddr_writel(DDRC_CTRL_CKE, DDRC_CTRL); // set CKE to high
                udelay(10);

                switch(type) {
                        case DDR2:
                                break;
                        case LPDDR2:
                        break;
                case DDR3:
#define DDRC_LMR_MR(n)															\
                        (pddr_params->DDRC_DLMR_VALUE) | DDRC_LMR_START | DDRC_LMR_CMD_LMR |            \
                        ((pddr_params->DDR_MR##n##_VALUE & 0xffff) << DDRC_LMR_DDR_ADDR_BIT) |  \
                                (((pddr_params->DDR_MR##n##_VALUE >> 16) & 0x7) << DDRC_LMR_BA_BIT)
                        ddr_writel(0, DDRC_LMR);udelay(5);
                        ddr_writel(DDRC_LMR_MR(2), DDRC_LMR); //MR2
                        printf("mr2: %x\n", DDRC_LMR_MR(2));
                        udelay(5);
                        ddr_writel(0, DDRC_LMR);udelay(5);
                        ddr_writel(DDRC_LMR_MR(3), DDRC_LMR); //MR3
                        printf("mr3: %x\n", DDRC_LMR_MR(3));
                        udelay(5);
#ifdef DEBUG_DDR_EFUSE_PARAM
                        printf("ddr3_param->params[kgd_odt] %d\n",ddr3_param->params[kgd_odt]);
                        printf("ddr3_param->params[kgd_ds] %d\n",ddr3_param->params[kgd_ds]);
                        printf("pddr_params->DDR_MR1_VALUE %d\n",pddr_params->DDR_MR1_VALUE);
#endif
                        //取出MR1 A9 A6 A2,A5 A1
                        reg_value = (pddr_params->DDR_MR1_VALUE & 0xFFFFFD99) | (((ddr3_param->params[kgd_odt] >> 2) & 0x1)<< 9)\
                        | (((ddr3_param->params[kgd_odt] >> 1) & 0x1)<< 6) | ((ddr3_param->params[kgd_odt] & 0x1)<< 2)\
                        | (((ddr3_param->params[kgd_ds] >> 1) & 0x1)<< 5) | ((ddr3_param->params[kgd_ds] & 0x1)<< 1);

                        reg_value = pddr_params->DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR | \
                        ((reg_value & 0xffff) << DDRC_LMR_DDR_ADDR_BIT) |       \
                        (((reg_value >> 16) & 0x7) << DDRC_LMR_BA_BIT);

                        ddr_writel(0, DDRC_LMR);udelay(5);
                        ddr_writel(reg_value, DDRC_LMR); //MR1
                        printf("mr1: %x\n", reg_value);
                        udelay(5);

                        ddr_writel(0, DDRC_LMR);udelay(5);
                        ddr_writel(DDRC_LMR_MR(0), DDRC_LMR); //MR0
                        printf("mr0: %x\n", DDRC_LMR_MR(0));
                        udelay(5);

                        switch (chip)
                        {
                                case 0x1111://A1N
                                case 0x5555://A1NT
                                case 0x3333://A1L
                                case 0x4444://A1A
                                case 0x2222://A1X
                                        ddr_writel((pddr_params->DDRC_DLMR_VALUE) | DDRC_LMR_START | DDRC_LMR_CMD_ZQCL_CS0, DDRC_LMR); //ZQCL
                                        printf("ZQCL ENABLE\n");
                                        break;
                                default:
                                        ddr_hang();
                                        break;
                        }

                        udelay(5);

#undef DDRC_LMR_MR

                        break;
                default:
                        ddr_hang();
                }

#else/* not CONFIG_A1ALL */


#if CONFIG_DDR_BUSWIDTH_32
                ddr_writel((DDRC_DWCFG_DFI_INIT_START | 1), DDRC_DWCFG); // dfi_init_start high 0x09
                ddr_writel(1, DDRC_DWCFG); // set buswidth 32bit
#else
                ddr_writel((DDRC_DWCFG_DFI_INIT_START), DDRC_DWCFG); // dfi_init_start high ddr buswidth 16bit
                ddr_writel(0, DDRC_DWCFG); // set buswidth 16bit
#endif
                while(!(ddr_readl(DDRC_DWSTATUS) & DDRC_DWSTATUS_DFI_INIT_COMP)); //polling dfi_init_complete

                udelay(50);
                ddr_writel(0, DDRC_CTRL); //set dfi_reset_n high

                ddr_writel(DDRC_CFG_VALUE, DDRC_CFG);
                udelay(500);
                ddr_writel(DDRC_CTRL_CKE, DDRC_CTRL); // set CKE to high
                udelay(10);

                switch(type) {
                        case DDR2:
#define DDRC_LMR_MR(n)												\
                                1 << 1 | DDRC_LMR_START | DDRC_LMR_CMD_LMR |            \
                                ((DDR_MR##n##_VALUE & 0x1fff) << DDRC_LMR_DDR_ADDR_BIT) |       \
                                (((DDR_MR##n##_VALUE >> 13) & 0x3) << DDRC_LMR_BA_BIT)
                                while (ddr_readl(DDRC_LMR) & (1 << 0));
                                ddr_writel(0x400003, DDRC_LMR);
                                udelay(100);
                                ddr_writel(DDRC_LMR_MR(2), DDRC_LMR); //MR2
                                udelay(5);
                                ddr_writel(DDRC_LMR_MR(3), DDRC_LMR); //MR3
                                udelay(5);
                                ddr_writel(DDRC_LMR_MR(1), DDRC_LMR); //MR1
                                udelay(5);
                                ddr_writel(DDRC_LMR_MR(0), DDRC_LMR); //MR0
                                udelay(5 * 1000);
                                ddr_writel(0x400003, DDRC_LMR);
                                udelay(100);
                                ddr_writel(0x43, DDRC_LMR);
                                udelay(5);
                                ddr_writel(0x43, DDRC_LMR);
                                udelay(5 * 1000);
#undef DDRC_LMR_MR
                                break;
                        case LPDDR2:
#define DDRC_LMR_MR(n)													\
                        1 << 1| DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR |   \
                                ((DDR_MR##n##_VALUE & 0xff) << 24) |                                            \
                                (((DDR_MR##n##_VALUE >> 8) & 0xff) << (16))
                        ddr_writel(DDRC_LMR_MR(63), DDRC_LMR); //set MRS reset
                        ddr_writel(DDRC_LMR_MR(10), DDRC_LMR); //set IO calibration
                        ddr_writel(DDRC_LMR_MR(1), DDRC_LMR); //set MR1
                        ddr_writel(DDRC_LMR_MR(2), DDRC_LMR); //set MR2
                        ddr_writel(DDRC_LMR_MR(3), DDRC_LMR); //set MR3
#undef DDRC_LMR_MR
                        break;
                case DDR3:

#define DDRC_LMR_MR(n)												\
                        DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR |           \
                        ((DDR_MR##n##_VALUE & 0xffff) << DDRC_LMR_DDR_ADDR_BIT) |       \
                                (((DDR_MR##n##_VALUE >> 16) & 0x7) << DDRC_LMR_BA_BIT)
                        ddr_writel(0, DDRC_LMR);udelay(5);
                        ddr_writel(DDRC_LMR_MR(2), DDRC_LMR); //MR2
                        printf("mr2: %x\n", DDRC_LMR_MR(2));
                        udelay(5);
                        ddr_writel(0, DDRC_LMR);udelay(5);
                        ddr_writel(DDRC_LMR_MR(3), DDRC_LMR); //MR3
                        printf("mr3: %x\n", DDRC_LMR_MR(3));
                        udelay(5);
#ifdef DEBUG_DDR_EFUSE_PARAM
                        printf("ddr3_param->params[kgd_odt] %d\n",ddr3_param->params[kgd_odt]);
                        printf("ddr3_param->params[kgd_ds] %d\n",ddr3_param->params[kgd_ds]);
                        printf("DDR_MR1_VALUE %d\n",DDR_MR1_VALUE);
#endif
                        //取出MR1 A9 A6 A2,A5 A1
                        reg_value = (DDR_MR1_VALUE & 0xFFFFFD99) | (((ddr3_param->params[kgd_odt] >> 2) & 0x1)<< 9)\
                        | (((ddr3_param->params[kgd_odt] >> 1) & 0x1)<< 6) | ((ddr3_param->params[kgd_odt] & 0x1)<< 2)\
                        | (((ddr3_param->params[kgd_ds] >> 1) & 0x1)<< 5) | ((ddr3_param->params[kgd_ds] & 0x1)<< 1);

                        reg_value = DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR |           \
                        ((reg_value & 0xffff) << DDRC_LMR_DDR_ADDR_BIT) |       \
                        (((reg_value >> 16) & 0x7) << DDRC_LMR_BA_BIT);

                        ddr_writel(0, DDRC_LMR);udelay(5);
                        ddr_writel(reg_value, DDRC_LMR); //MR1
                        printf("mr1: %x\n", reg_value);
                        udelay(5);

                        ddr_writel(0, DDRC_LMR);udelay(5);
                        ddr_writel(DDRC_LMR_MR(0), DDRC_LMR); //MR0
                        printf("mr0: %x\n", DDRC_LMR_MR(0));
                        udelay(5);

                        switch (chip)
                        {
                                case 0x1111://A1N
                                case 0x5555://A1NT
                                case 0x3333://A1L
                                case 0x4444://A1A
                                case 0x2222://A1X
                                        ddr_writel(DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_ZQCL_CS0, DDRC_LMR); //ZQCL
                    printf("ZQCL ENABLE\n");
                                        break;
                                default:
                                        ddr_hang();
                                        break;
                        }

                        udelay(5);

#undef DDRC_LMR_MR

                        break;
                default:
                        ddr_hang();
                }

#endif /* END CONFIG_A1ALL */
    FUNC_EXIT();
}


struct ddr_calib_value {
	unsigned int rate;
	unsigned int refcnt;
	unsigned char bypass_al;
	unsigned char bypass_ah;
};

#define REG32(addr) *(volatile unsigned int *)(addr)
#define CPM_DDRCDR (0xb000002c)

static void ddr_calibration(struct ddr_calib_value *dcv, int div)
{
	unsigned int val;

	// Set change_en
	val = REG32(CPM_DDRCDR);
	val |= ((1 << 29) | (1 << 25));
	REG32(CPM_DDRCDR) = val;
	while((REG32(CPM_DDRCDR) & (1 << 24)))
		;
	/* // Set clock divider */
	val = REG32(CPM_DDRCDR);
	val &= ~(0xf);
	val |= div;
	REG32(CPM_DDRCDR) = val;

	// Polling PHY_FREQ_DONE
	while(((ddr_readl(DDRC_DWSTATUS) & (1 << 3 | 1 << 1)) & 0xf) != 0xa);
	ddrp_hardware_calibration();
	/* ddrp_software_calibration(); */

	dcv->bypass_al = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL1);
	/* printf("auto :CALIB_AL: dcv->bypss_al %x\n", dcv->bypass_al); */
	dcv->bypass_ah = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH1);
	/* printf("auto:CAHIB_AH: dcv->bypss_ah %x\n", dcv->bypass_ah); */

	// Set Controller Freq Exit
	val = ddr_readl(DDRC_DWCFG);
	val |= (1 << 2);
	ddr_writel(val, DDRC_DWCFG);

	// Clear Controller Freq Exit
	val = ddr_readl(DDRC_DWCFG);
	val &= ~(1 << 2);
	ddr_writel(val, DDRC_DWCFG);

	val = REG32(CPM_DDRCDR);
	val &= ~((1 << 29) | (1 << 25));
	REG32(CPM_DDRCDR) = val;
}

static void get_dynamic_calib_value(unsigned int rate)
{
	struct ddr_calib_value *dcv;
	unsigned int drate = 0;
	int div, n, cur_div;
#define CPU_TCSM_BASE (0xb2400000)
	dcv = (struct ddr_calib_value *)(CPU_TCSM_BASE + 2048);
	cur_div = REG32(CPM_DDRCDR) & 0xf;
	div = cur_div + 1;
	do {
		drate = rate / (div + 1);
		if(drate < 100000000) {
#ifdef CONFIG_A1ALL
			dcv[cur_div].rate = rate;
			dcv[cur_div].refcnt = pddr_params->get_refcnt_value(cur_div);
			ddr_calibration(&dcv[cur_div], cur_div);
			break;
		}
		dcv[div].rate = drate;
		dcv[div].refcnt = pddr_params->get_refcnt_value(div);
		ddr_calibration(&dcv[div], div);
		div ++;

#else
			dcv[cur_div].rate = rate;
			dcv[cur_div].refcnt = get_refcnt_value(cur_div);
			ddr_calibration(&dcv[cur_div], cur_div);
			break;
		}
		dcv[div].rate = drate;
		dcv[div].refcnt = get_refcnt_value(div);
		ddr_calibration(&dcv[div], div);
		div ++;
#endif
	} while(1);

	/* for(div = 6, n = 0; div > 0; div--, n++) { */
	/* 	dcv[div - 1].rate = rate / div; */
	/* 	if(dcv[div - 1].rate < 100000000) */
	/* 		break; */
	/* 	dcv[div - 1].refcnt = get_refcnt_value(div); */
	/* 	get_calib_value(&dcv[div - 1], div); */
	/* } */
}

static void ddrp_set_per_bit_de_skew(void)
{
	writel(0x14, DDR_PHY_BASE+((0x1c0+0x12)*4));//RX DQS0
	writel(0x14, DDR_PHY_BASE+((0x1c0+0x27)*4));//RX DQS1
	writel(0x14, DDR_PHY_BASE+((0x1c0+0x2a)*4));//RX DQSB0
	writel(0x14, DDR_PHY_BASE+((0x1c0+0x2b)*4));//RX DQSB1

	writel(0x14, DDR_PHY_BASE+((0x220+0x12)*4));//RX DQS0
	writel(0x14, DDR_PHY_BASE+((0x220+0x27)*4));//RX DQS1
	writel(0x14, DDR_PHY_BASE+((0x220+0x2a)*4));//RX DQSB0
	writel(0x14, DDR_PHY_BASE+((0x220+0x2b)*4));//RX DQSB1
}

static void ddrp_try_per_bit_de_skew(void)
{
	int value = 0;
	int channel = 0;
	unsigned int reg_offset[4] = {0x1c0, 0x220};
	unsigned int offset;;
    dump_ddrp_per_bit_de_skew();
	for (value = 0; value < 0x3f; value++) {
		for (channel = 0; channel < 2; channel++) {
			offset = reg_offset[channel];
#if 0
			writel(value, DDR_PHY_BASE+((offset+0x0)*4));//RX DM0
			writel(value, DDR_PHY_BASE+((offset+0x2)*4));
			writel(value, DDR_PHY_BASE+((offset+0x4)*4));
			writel(value, DDR_PHY_BASE+((offset+0x6)*4));
			writel(value, DDR_PHY_BASE+((offset+0x8)*4));
			writel(value, DDR_PHY_BASE+((offset+0xa)*4));
			writel(value, DDR_PHY_BASE+((offset+0x8)*4));
			writel(value, DDR_PHY_BASE+((offset+0xe)*4));
			writel(value, DDR_PHY_BASE+((offset+0x10)*4));//RX DQ7

#endif
			writel(value, DDR_PHY_BASE+((offset+0x12)*4));//RX DQS0
#if 0
			writel(value, DDR_PHY_BASE+((offset+0x15)*4));//RX DM1
			writel(value, DDR_PHY_BASE+((offset+0x17)*4));
			writel(value, DDR_PHY_BASE+((offset+0x19)*4));
			writel(value, DDR_PHY_BASE+((offset+0x1b)*4));
			writel(value, DDR_PHY_BASE+((offset+0x1d)*4));
			writel(value, DDR_PHY_BASE+((offset+0x1f)*4));
			writel(value, DDR_PHY_BASE+((offset+0x21)*4));
			writel(value, DDR_PHY_BASE+((offset+0x23)*4));
			writel(value, DDR_PHY_BASE+((offset+0x25)*4));//RX DQ15
#endif
			writel(value, DDR_PHY_BASE+((offset+0x27)*4));//RX DQS1
			writel(value, DDR_PHY_BASE+((offset+0x2a)*4));//RX DQSB0
			writel(value, DDR_PHY_BASE+((offset+0x2b)*4));//RX DQSB1
		}
		{
			int j = 0;
			unsigned int addr;
			unsigned int value_get;
			unsigned int data;
			unsigned int test_size =  64;
			unsigned int test_data =  0xffffffff;
			printf("try rx skew %x\n", value);
			int isok = 1;
			for (j = 0; j < test_size; j+=4) {
				data = (j/4)%2?test_data:0;
				*(volatile unsigned int *)(addr+j) = data;
			}
			addr = 0xa1000000;
			for (j = 0; j < test_size; j+=4) {
				data = (j/4)%2?test_data:0;
				value_get = *(volatile unsigned int *)(addr+j);
				if (value_get != data) {
					printf("memtest rx skew %x  addr 0x%x  want 0x%x get 0x%x error 0x%x\n",
							value, addr+j, data, value_get, data^value_get);
					isok = 0;
				}
			}
			if (isok)
				printf("try rx skew %x success.\n", value);
			else
				printf("try rx skew %x failed.\n", value);
		}
	}

    dump_ddrp_per_bit_de_skew();
}


int ddrp_set_rfifo()
{
	ddrp_reg_set_range(0x1, 5, 1, 1);
	ddrp_reg_set_range(0xe, 0, 3, 3);
	return 0;
}

int ddrp_set_rx_vref()
{
    int A_L=0x80,A_H=0x80,B_L=0x80,B_H=0x80;

#ifdef DDR_PHY_PARAMS_TEST_A1
    printf("ddrp_set_rx_vref &A_L,&A_H,&B_L,&B_H:\n");
    scanf("%d,%d,%d,%d\n",&A_L,&A_H,&B_L,&B_H);


    printf("ddrp vref AL: %x\n", ddrp_readl_byidx(0x140+0x07));
    printf("ddrp vref AH: %x\n", ddrp_readl_byidx(0x140+0x17));
    printf("ddrp vref BL: %x\n", ddrp_readl_byidx(0x160+0x07));
    printf("ddrp vref BH: %x\n", ddrp_readl_byidx(0x160+0x07));

    //A_L
    ddrp_reg_set_range(0x140+0x07, 0, 8, A_L);
    //A_H
    ddrp_reg_set_range(0x140+0x17, 0, 8, A_H);
    //B_L
    ddrp_reg_set_range(0x160+0x07, 0, 8, B_L);
    //B_H
    ddrp_reg_set_range(0x160+0x17, 0, 8, B_H);
#else
    //A_L
    ddrp_reg_set_range(0x140+0x07, 0, 8, 0x80);
    //A_H
    ddrp_reg_set_range(0x140+0x17, 0, 8, 0x80);
    //B_L
    ddrp_reg_set_range(0x160+0x07, 0, 8, 0x80);
    //B_H
    ddrp_reg_set_range(0x160+0x17, 0, 8, 0x80);
#endif


    printf("ddrp vref AL: %x\n", ddrp_readl_byidx(0x140+0x07));
    printf("ddrp vref AH: %x\n", ddrp_readl_byidx(0x140+0x17));
    printf("ddrp vref BL: %x\n", ddrp_readl_byidx(0x160+0x07));
    printf("ddrp vref BH: %x\n", ddrp_readl_byidx(0x160+0x07));
	return 0;
}

/* DDR sdram init */
void sdram_init(void)
{
	enum ddr_type type;
	unsigned int rate;
	int bypass = 0;
    unsigned short chipid = 0;
    ddr3_param_t *ddr3_param_ptr = NULL;
    bool is16dw = false;

	type = get_ddr_type();
	debug("ddr type is =%d\n",type);

	chipid = get_chip_type_from_efuse();
        printf("chipid 0x%x\n",chipid);

    if( 0 == chipid){
#ifdef CONFIG_A1N
            chipid = 0x1111;
#endif
#ifdef CONFIG_A1X
            chipid = 0x2222;
#endif
#ifdef CONFIG_A1L
            chipid = 0x3333;
#endif
#ifdef CONFIG_A1A
            chipid = 0x4444;
#endif
#ifdef CONFIG_A1NT
            chipid = 0x5555;
#endif
    printf("SET 0x%x\n",chipid);

#ifdef CONFIG_A1ALL
    printf("no chip id , Compile option error,Can't use ALL compile!\n\n");
    while(1);
#endif
    }

    switch (chipid)
    {
        case 0x1111://A1N
            ddr3_param_ptr = &a1n_ddr3_param;
            is16dw = true;
            break;
        case 0x2222://A1X
            ddr3_param_ptr = &a1x_ddr3_param;
            break;
        case 0x3333://A1L
            ddr3_param_ptr = &a1l_ddr3_param;
            is16dw = true;
            break;
        case 0x4444://A1A
            ddr3_param_ptr = &a1a_ddr3_param;
            break;
        case 0x5555://A1NT
            ddr3_param_ptr = &a1nt_ddr3_param;
            break;
        default:
            printf("chipid error,no ddr param!\n\n");
            return -1;
    }

    printf("%s\n",ddr3_param_ptr->version);
    ddr_param_efuse_init(ddr3_param_ptr, is16dw);

#ifndef CONFIG_FPGA
	clk_set_rate(DDR, gd->arch.gi->ddrfreq);
	/*if(ddr_hook && ddr_hook->prev_ddr_init)*/
		/*ddr_hook->prev_ddr_init(type);*/
	rate = clk_get_rate(DDR);
#else
	rate = gd->arch.gi->ddrfreq;
#endif

#ifndef CONFIG_FAST_BOOT
    printf("DDR clk rate: %d\n\n", rate);
#endif

	ddrc_reset_phy();
	//mdelay(20);
	ddr_phy_init(rate);

    ddr_param_write(ddr3_param_ptr);

#ifdef DDR_PHY_PARAMS_TEST_A1
	//ddrp_set_drv_odt(chipid);
#endif

    ddrc_dfi_init(type, bypass, chipid ,ddr3_param_ptr);
	ddrc_prev_init();

#ifdef CONFIG_DDR_HARDWARE_TRAINING
	ddrp_hardware_calibration();
#endif
#ifdef	CONFIG_DDR_SOFT_TRAINING
	ddrp_software_calibration();
#endif

#ifdef CONFIG_DDR_DEBUG
    dump_ddrp_driver_strength_register();

	printf("DDRP_INNOPHY_INNO_PHY 0X1d0    0x%x\n", ddr_readl(DDR_PHY_OFFSET + 0x1d0));
	printf("DDRP_INNOPHY_INNO_PHY 0X1d4    0x%x\n", ddr_readl(DDR_PHY_OFFSET + 0x1d4));
	printf("DDRP_INNOPHY_INNO_PHY 0X290    0x%x\n", ddr_readl(DDR_PHY_OFFSET + 0x290));
	printf("DDRP_INNOPHY_INNO_PHY 0X294    0x%x\n", ddr_readl(DDR_PHY_OFFSET + 0x294));
	printf("DDRP_INNOPHY_INNO_PHY 0X1c0    0x%x\n", ddr_readl(DDR_PHY_OFFSET + 0x1c0));
	printf("DDRP_INNOPHY_INNO_PHY 0X280    0x%x\n", ddr_readl(DDR_PHY_OFFSET + 0x280));
#endif

	ddrc_post_init();

	/*get_dynamic_calib_value(rate);*/
#ifdef CONFIG_A1ALL
	if(pddr_params->DDRC_AUTOSR_EN_VALUE) {
		ddr_writel(pddr_params->DDRC_AUTOSR_CNT_VALUE, DDRC_AUTOSR_CNT);
		ddr_writel(1, DDRC_AUTOSR_EN);
	} else {
		ddr_writel(pddr_params->DDRC_AUTOSR_CNT_VALUE, DDRC_AUTOSR_CNT);
		ddr_writel(0, DDRC_AUTOSR_EN);
	}

	ddr_writel(pddr_params->DDRC_HREGPRO_VALUE, DDRC_HREGPRO);
	ddr_writel(pddr_params->DDRC_PREGPRO_VALUE, DDRC_PREGPRO);
#else
	if(DDRC_AUTOSR_EN_VALUE) {
		ddr_writel(DDRC_AUTOSR_CNT_VALUE, DDRC_AUTOSR_CNT);
		ddr_writel(1, DDRC_AUTOSR_EN);
	} else {
		ddr_writel(DDRC_AUTOSR_CNT_VALUE, DDRC_AUTOSR_CNT);
		ddr_writel(0, DDRC_AUTOSR_EN);
	}

	ddr_writel(DDRC_HREGPRO_VALUE, DDRC_HREGPRO);
	ddr_writel(DDRC_PREGPRO_VALUE, DDRC_PREGPRO);
#endif
	//ddrp_set_rx_vref();
	ddrp_set_rfifo();

#ifdef CONFIG_DDR_DEBUG
	dump_ddrc_register();
	dump_ddrp_register();
	dump_ddrp_driver_strength_register();
#endif

#ifdef DDR_PHY_PARAMS_TEST_A1
        ddr_test();
#endif

#if 0
	printf("reselect ddr type\n");
#ifdef CONFIG_DDR_TYPE_DDR3
	//printf("PHY REG-01 :  0x%x \n", ddrp_reg_get(0x1));
	ddrp_reg_set_range(0x1, 6, 1, 1);
	//printf("PHY REG-01 :  0x%x \n", ddrp_reg_get(0x1));
#elif defined(CONFIG_DDR_TYPE_DDR2)
	//printf("PHY REG-01 :  0x%x \n",readl(0xb3011004));
	writel(0x51,0xb3011004);
	//printf("PHY REG-01 :  0x%x \n",readl(0xb3011004));
#endif
	//fifo need set reg 0x01 bit6  to 1
	//printf("PHY REG-0xa :  0x%x \n", ddrp_reg_get(0xa));
	ddrp_reg_set_range(0xa, 1, 3, 3);
	//printf("PHY REG-0xa :  0x%x \n", ddrp_reg_get(0xa));

	//TX Write Pointer adjust
	//printf("PHY REG-0x8 :  0x%x \n", ddrp_reg_get(0x8));
	ddrp_reg_set_range(0x8, 0, 2, 3);
	//printf("PHY REG-0x8 :  0x%x \n", ddrp_reg_get(0x8));

	//extend rx dqs gateing window
	//ddrp_reg_set_range(0x9, 7, 1, 1);

	//ddrp_software_writeleveling_rx();
#endif
	//ddrp_try_per_bit_de_skew();
	//ddrp_set_per_bit_de_skew();
	//dump_ddrp_per_bit_de_skew();
	debug("sdram init finished\n");
}


phys_size_t initdram(int board_type)
{
	/* SDRAM size was calculated when compiling. */
#ifndef EMC_LOW_SDRAM_SPACE_SIZE
#define EMC_LOW_SDRAM_SPACE_SIZE 0x20000000 /* 512M */
#endif /* EMC_LOW_SDRAM_SPACE_SIZE */
		unsigned int ram_size;
#ifdef CONFIG_A1ALL
	ram_size = (unsigned int)(pddr_params->DDR_CHIP_0_SIZE) + (unsigned int)(pddr_params->DDR_CHIP_1_SIZE);
#else
	ram_size = (unsigned int)(DDR_CHIP_0_SIZE) + (unsigned int)(DDR_CHIP_1_SIZE);
#endif
	if (ram_size > EMC_LOW_SDRAM_SPACE_SIZE)
		ram_size = EMC_LOW_SDRAM_SPACE_SIZE;

	return ram_size;
}
