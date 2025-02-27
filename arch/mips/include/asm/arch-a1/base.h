/*
 * A1 REG_BASE definitions
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
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

#ifndef __BASE_H__
#define __BASE_H__

/*
 * Define the module base addresses
 */

/* AHB0 BUS Devices Base */
#define G_OST_BASE	0xb2000000
#define N_OST_BASE	0xb2100000
#define CCU_BASE	0xb2200000
#define INTCN_BASE	0xb2300000
#define SRAM_BASE	0xb2400000
#define NNDMA_BASE	0xb2500000
#define LEPCCU_BASE	0xb2a00000

#define HARB0_BASE	0xb3000000
#define DDRC_BASE	0xb34f0000
#define DDR_PHY_OFFSET	(-0x4e0000 + 0x1000)
#define DDRC_APB_OFFSET	(-0x4e0000 + 0x2000)
#define MSC0_BASE	0xb3060000
#define MSC1_BASE	0xb3070000
#define IPU_BASE	0xb3080000
#define AIP_BASE	0xb3090000
#define MONITOR_BASE	0xb30a0000
#define GMAC0_BASE	0xb30b0000
#define GMAC1_BASE	0xb30c0000
#define SATA_BASE	0xb30d0000
#define VO_BASE		0xb30e0000
#define VDE_BASE	0xb3300000
#define HDMI_BASE	0xb3380000
#define DDR_PHY_BASE    0xb3011000

/* AHB1 BUS Devices Base */
#define VC8000D_BASE	0xb3100000
#define JPEG_BASE	0xb3200000

/* AHB2 BUS Devices Base */
#define HARB2_BASE	0xb3400000
#define NEMC_BASE	0xb3410000
#define PDMA_BASE	0xb3420000
#define AES_BASE	0xb3430000
#ifdef  CONFIG_SFC0
#define SFC_BASE	0xb3440000
#endif  /* CONFIG_SFC0 */
#ifdef  CONFIG_SFC1
#define SFC_BASE	0xb3450000
#endif  /* CONFIG_SFC1 */
#define PWM_BASE	0xb3460000
#define HASH_BASE	0xb3480000
// #define RSA_BASE	0xb3480000
#define OTG0_BASE	0xb3600000
#define OTG1_BASE	0xb3640000
#define OTG2_BASE	0xb3680000
#define EFUSE_BASE	0xb3540000
#define INTC_BASE	0xb0010000

/* APB BUS Devices Base */
#define	CPM_BASE	0xb0000000
#define	TCU_BASE	0xb0002000
#define	RTC_BASE	0xb0003000
#define	GPIO_BASE	0xb0010000
#define	AIC0_BASE	0xb0020000
#define	AIC1_BASE	0xb0021000
#define	CODEC_BASE	0xb0022000
#define	UART0_BASE	0xb0030000
#define	UART1_BASE	0xb0031000
#define	UART2_BASE	0xb0032000
#define	SSI0_BASE	0xb0043000
#define	SSI1_BASE	0xb0044000
#define	I2C0_BASE	0xb0050000
#define	I2C1_BASE	0xb0051000
#define	USB_PHY_BASE	0xb0060000
#define	DES_BASE	0xb0061000
#define	DTRNG_BASE	0xb0072000
#define	HDMI_PHY_BASE	0xb0075000
#define	VDAC_BASE	0xb0076000
#define	SATA_PHY0_BASE	0xb0080000
#define	SATA_PHY1_BASE	0xb0090000
#define WDT_BASE	0xb0002000

/* NAND CHIP Base Address*/
#define NEMC_CS1_BASE 0xbb000000
#define NEMC_CS2_BASE 0xba000000
#define NEMC_CS3_BASE 0xb9000000
#define NEMC_CS4_BASE 0xb8000000
#define NEMC_CS5_BASE 0xb7000000
#define NEMC_CS6_BASE 0xb6000000

#define AUX_BASE	0xb32a0000
#endif /* __BASE_H__ */
