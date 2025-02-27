/*
 * A1 clock common interface
 *
 * Copyright (C) 2017 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <ykli@ingenic.cn>
 * Based on: newxboot/modules/clk/jz4775_clk.c
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
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch-a1/cpm.h>
#include <asm/arch-a1/clk.h>
#ifdef CONFIG_A1ALL
#include <generated/ddr_reg_values_a1all.h>
#include <linux/string.h>
extern void get_a1chip_ddr_config();
extern struct a1_ddr_params *pddr_params;
#endif

DECLARE_GLOBAL_DATA_PTR;


//#define DUMP_CGU_SELECT
#ifdef DUMP_CGU_SELECT
static char clk_name[][10] = {
	[DDR] = {"ddr"},
	[MAC0PHY] = {"mac0phy"},
	[MAC0TXCDR] = {"mac0tx"},
	[MAC0PTPCDR] = {"mac0ptpcdr"},
	[MAC1PHY] = {"mac1phy"},
	[MAC1TXCDR] = {"mac1tx"},
	[MSC] = {"msc"},
	[MSC1] = {"msc1"},
	[SFC0] = {"sfc0"},
	[SFC1] = {"sfc1"},
	[VDEV] = {"vdev"},
	[VDEA] = {"vdea"},
	[VDEM] = {"vdem"},
	[IPU] = {"ipu"},
};

static char * cgu_name(int clk) {
	return clk_name[clk];
}
#endif

struct cgu cgu_clk_sel[CGU_CNT] = {
	[DDR] = {1, CPM_DDRCDR, 30, CONFIG_DDR_SEL_PLL, {0, APLL, MPLL, -1}, 29, 28, 27},
	[MAC0PHY] = {1, CPM_MAC0CDR, 30, EPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27},
	[MAC0TXCDR] = {1, CPM_MAC0TXCDR, 30,EPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27},
	[MAC0PTPCDR] = {1, CPM_MAC0PTPCDR, 30, EPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27},
	[MAC1PHY] = {1, CPM_MAC1CDR, 30, EPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27},
	[MAC1TXCDR] = {1, CPM_MAC1TXCDR, 30, EPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27},
	[MSC] = {1, CPM_MSC0CDR, 30, APLL, {APLL, MPLL, EPLL, -1}, 29, 28, 27},
	[MSC1] = {0, CPM_MSC1CDR, 30, APLL, {APLL, MPLL, EPLL, -1}, 29, 28, 27},
	[I2S] = {1, CPM_I2S0TCDR, 30, APLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27}, //需要再配置; i2s 使用VPLL  /*i2s还有iis1 cdr，请clk核对人员注意*/
	[SFC0] = {1, CPM_SFC0CDR, 30, MPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27}, /* TODO */
	[SFC1] = {1, CPM_SFC1CDR, 30, MPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27}, /* TODO */
	[VDEV] = {1, CPM_VDEVCDR, 30, VPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27},
	[VDEA] = {1, CPM_VDEACDR, 30, VPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27},
	[VDEM] = {1, CPM_VDEMCDR, 30, VPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27},
	[IPU] = {1, CPM_IPUMCDR, 30, VPLL, {APLL, MPLL, VPLL, EPLL}, 29, 28, 27},
};

void clk_prepare(void)
{
	/*stop clk and set div max*/
	int id;
	struct cgu *cgu = NULL;
	unsigned regval = 0, reg = 0;

	for (id = 0; id < CGU_CNT; id++) {
		cgu = &(cgu_clk_sel[id]);
		reg = CPM_BASE + cgu->off;

		if(id == MSC || id == MSC1 || I2S)
			continue;

		regval = readl(reg);
		/*set div max*/
		regval |= 0xff | (1 << cgu->ce);
		while (readl(reg) & (1 << cgu->busy));
		writel(regval, reg);

		/*stop clk*/
		while (readl(reg) & (1 << cgu->busy));
		regval = readl(reg);
		regval |= ((1 << cgu->stop) | (1 << cgu->ce));
		writel(regval, reg);
		while (readl(reg) & (1 << cgu->busy));

		/*clear ce*/
		regval = readl(reg);
		regval &= ~(1 << cgu->ce);
		writel(regval, reg);
#ifdef DUMP_CGU_SELECT
		printf("%s(0x%x) :0x%x\n",clk_name[id] ,reg,  readl(reg));
#endif
	}
}

/***************外设时钟源选择30 31位******************/
void cgu_clks_set(struct cgu *cgu_clks, int nr_cgu_clks)
{
	int i, j, id;
	unsigned int xcdr = 0;
	unsigned int reg = 0;
	extern struct cgu_clk_src cgu_clk_src[];

	for (i = 0; cgu_clk_src[i].cgu_clk != SRC_EOF; i++) {
		id = cgu_clk_src[i].cgu_clk;
		cgu_clks[id].sel_src = cgu_clk_src[i].src;
	}

	for(i = 0; i < nr_cgu_clks; i++) {
		for (j = 0; j < 4; j++) {
			if (cgu_clks[i].sel_src == cgu_clks[i].sel[j] &&
					cgu_clks[i].en == 1) {
				reg = CPM_BASE + cgu_clks[i].off;
				xcdr = readl(reg);
				xcdr &= ~(3 << 30);

				xcdr |= j << cgu_clks[i].sel_bit;
				writel(xcdr, reg);

#ifdef DUMP_CGU_SELECT
				printf("%s: 0x%X: value=0x%X j = %d\n", cgu_name(i), reg, readl(reg), j);
#endif
				break;
			}
		}

	}
}

static unsigned int pll_get_rate(int pll)
{
	unsigned int cpxpcr = 0;
	unsigned int m, n, od0, od1;

	switch (pll) {
	case APLL:
#ifdef CONFIG_A1ALL
		return pddr_params->config_sys_apll_freq;
#else
#ifdef CONFIG_SYS_APLL_FREQ
		return CONFIG_SYS_APLL_FREQ;
#endif
#endif
		cpxpcr = cpm_inl(CPM_CPAPCR);
		break;
	case MPLL:
#ifdef CONFIG_A1ALL
		return pddr_params->config_sys_mpll_freq;
#else
#ifdef CONFIG_SYS_MPLL_FREQ
		return CONFIG_SYS_MPLL_FREQ;
#endif
#endif
		cpxpcr = cpm_inl(CPM_CPMPCR);
		break;
	case VPLL:
#ifdef CONFIG_SYS_VPLL_FREQ
		return CONFIG_SYS_VPLL_FREQ;
#endif
		cpxpcr = cpm_inl(CPM_CPVPCR);
		break;
	case EPLL:
#ifdef CONFIG_SYS_EPLL_FREQ
		return CONFIG_SYS_EPLL_FREQ;
#endif
		cpxpcr = cpm_inl(CPM_CPEPCR);
		break;
	default:
		return 0;
	}

	m = (cpxpcr >> 20) & 0xfff;
	n = (cpxpcr >> 14) & 0x3f;
	od1 = (cpxpcr >> 11) & 0x7;
	od0 = (cpxpcr >> 8) & 0x7;
#ifdef CONFIG_BURNER
	return (unsigned int)((unsigned long)gd->arch.gi->extal * m / n / od0 / od1);
#else
	return (unsigned int)((unsigned long)(CONFIG_SYS_EXTAL / 4000) * m / n / od0 / od1 * 4000);
#endif
}

static unsigned int get_ddr_rate(void)
{
	unsigned int ddrcdr  = cpm_inl(CPM_DDRCDR);

	switch ((ddrcdr >> 30) & 3) {
	case 1:
		return pll_get_rate(APLL) / ((ddrcdr & 0xf) + 1);
	case 2:
		return pll_get_rate(MPLL) / ((ddrcdr & 0xf) + 1);
	}
	return 0;
}

static unsigned int get_cclk_rate(void)
{
	unsigned int cpccr  = cpm_inl(CPM_CPCCR);

	switch ((cpccr >> 28) & 3) {
	case 1:
		return pll_get_rate(APLL) / ((cpccr & 0xf) + 1);
	case 2:
		return pll_get_rate(MPLL) / ((cpccr & 0xf) + 1);
	}
	return 0;
}

static unsigned int get_mac0_rate(unsigned int xcdr)
{
	unsigned int maccdr  = cpm_inl( CPM_MAC0CDR );

	switch ( maccdr >> 30 ) {
	case 0:
		pll_get_rate(APLL) / ((maccdr & 0xff) + 1);
		break;
	case 1:
		pll_get_rate(MPLL) / ((maccdr & 0xff) + 1);
		break;
	case 2:
		pll_get_rate(VPLL) / ((maccdr & 0xff) + 1);
		break;
	default:
		break;
	}

	return 0;
}

static unsigned int get_mac1_rate(unsigned int xcdr)
{
	unsigned int maccdr  = cpm_inl( CPM_MAC1CDR );

	switch ( maccdr >> 30 ) {
	case 0:
		pll_get_rate(APLL) / ((maccdr & 0xff) + 1);
		break;
	case 1:
		pll_get_rate(MPLL) / ((maccdr & 0xff) + 1);
		break;
	case 2:
		pll_get_rate(VPLL) / ((maccdr & 0xff) + 1);
		break;
	default:
		break;
	}

	return 0;
}

static unsigned int get_msc_rate(unsigned int xcdr)
{
	unsigned int msc0cdr  = cpm_inl(CPM_MSC0CDR);
	unsigned int mscxcdr  = cpm_inl(xcdr);
	unsigned int ret = 1;

	switch (msc0cdr >> 30) {
	case 0:
		ret = pll_get_rate(APLL) / (((mscxcdr & 0xff) + 1) * 2);
		break;
	case 1:
		ret = pll_get_rate(MPLL) / (((mscxcdr & 0xff) + 1) * 2);
		break;
	case 2:
		ret = pll_get_rate(VPLL) / (((mscxcdr & 0xff) + 1) * 2);
		break;
	case 3:
		ret = pll_get_rate(EPLL) / (((mscxcdr & 0xff) + 1) * 2);
		break;
	default:
		break;
	}

	return ret;
}

static unsigned int get_vdev_rate(unsigned int xcdr)
{
	unsigned int vdevcdr  = cpm_inl(xcdr);
	unsigned int ret = 0;

	switch (vdevcdr >> 30) {
	case 0:
		ret = pll_get_rate(APLL) / ((vdevcdr & 0xff) + 1);
		break;
	case 1:
		ret = pll_get_rate(MPLL) / ((vdevcdr & 0xff) + 1);
		break;
	case 2:
		ret = pll_get_rate(EPLL) / ((vdevcdr & 0xff) + 1);
		break;
	default:
		break;
	}
	return ret;
}

static unsigned int get_vdea_rate(unsigned int xcdr)
{
	unsigned int vdeacdr  = cpm_inl(xcdr);
	unsigned int ret = 0;

	switch (vdeacdr >> 30) {
	case 0:
		ret = pll_get_rate(APLL) / ((vdeacdr & 0xff) + 1);
		break;
	case 1:
		ret = pll_get_rate(MPLL) / ((vdeacdr & 0xff) + 1);
		break;
	case 2:
		ret = pll_get_rate(VPLL) / ((vdeacdr & 0xff) + 1);
		break;
	case 3:
		ret = pll_get_rate(EPLL) / ((vdeacdr & 0xff) + 1);
		break;
	default:
		break;
	}
	return ret;
}

static unsigned int get_vdem_rate(unsigned int xcdr)
{
	unsigned int vdemcdr  = cpm_inl(xcdr);
	unsigned int ret = 0;

	switch (vdemcdr >> 30) {
	case 0:
		ret = pll_get_rate(APLL) / ((vdemcdr & 0xff) + 1);
		break;
	case 1:
		ret = pll_get_rate(MPLL) / ((vdemcdr & 0xff) + 1);
		break;
	case 2:
		ret = pll_get_rate(VPLL) / ((vdemcdr & 0xff) + 1);
		break;
	case 3:
		ret = pll_get_rate(EPLL) / ((vdemcdr & 0xff) + 1);
		break;
	default:
		break;
	}
	return ret;
}

unsigned int cpm_get_h2clk(void)
{
	int h2clk_div;
	unsigned int cpccr  = cpm_inl(CPM_CPCCR);

	h2clk_div = (cpccr >> 12) & 0xf;

	switch ((cpccr >> 24) & 3) {
	case 1:
		return pll_get_rate(APLL) / (h2clk_div + 1);
	case 2:
		return pll_get_rate(MPLL) / (h2clk_div + 1);
	}
	return 0;
}

unsigned int clk_get_rate(int clk)
{
	switch (clk){
		case DDR:
			return get_ddr_rate();
		case CPU:
			return get_cclk_rate();
		case H2CLK:
			return cpm_get_h2clk();
		case MAC0PHY:
			return get_mac0_rate(CPM_MAC0CDR);
		case MAC1PHY:
			return get_mac1_rate(CPM_MAC1CDR);
		case MSC0:
			return get_msc_rate(CPM_MSC0CDR);
		case MSC1:
			return get_msc_rate(CPM_MSC1CDR);
		case VDEV:
			return get_vdev_rate(CPM_VDEVCDR);
		case VDEA:
			return get_vdev_rate(CPM_VDEACDR);
		case VDEM:
			return get_vdev_rate(CPM_VDEMCDR);
		case APLL:
			return pll_get_rate(APLL);
		case MPLL:
			return pll_get_rate(MPLL);
		case VPLL:
			return pll_get_rate(VPLL);
		case EPLL:
			return pll_get_rate(EPLL);
	}

	return 0;
}

/*
 *  设置外设的分频值 以及 使能位
 *
 ***/
void clk_set_rate(int clk, unsigned long rate)
{
	unsigned int cdr;
	unsigned int pll_rate;
	struct cgu *cgu = NULL;
	unsigned regval = 0, value = 0;
	int j = 0;

	if (clk >= CGU_CNT)	{
		printf("set clk id error\n");
		return;
	}

	if (clk == SFC0 || clk == SFC1)
		rate *= 4;
	cgu = &(cgu_clk_sel[clk]);
	regval = cpm_inl(cgu->off);

	pll_rate = pll_get_rate(cgu->sel_src);

	if (!pll_rate) {
		printf("clk id %d: get pll error\n", clk);
		return;
	}

	if (pll_rate % rate >= rate/ 2)
		pll_rate += rate - ( pll_rate % rate );
	else
		pll_rate -= ( pll_rate % rate );

	/****************低7位 分频值**************/

	if (clk == MSC0 || clk == MSC1 )
	{
		if(rate > 400000)
		{
			cdr = ((pll_rate/rate)/4 - 1) & 0xff;
		}else{
			/* PLL过高，无法分出200K时钟，使用外部时钟 */
			value   = regval & ~(3 << cgu->sel_bit | 1 << cgu->ce);
			value  |= (3 << cgu->sel_bit) | 1 << 21 | 1 << 27;
			regval &= ~(3 << cgu->sel_bit);
			regval |= (3 << cgu->sel_bit) | 1 << 21;
			cpm_outl(value, cgu->off);
			while (cpm_inl(cgu->off) & (1 << cgu->busy));
			cdr = ((24000000/rate)/4 - 1) & 0xff;
		}
	}else{
		cdr = (pll_rate/rate - 1 ) & 0xff;
	}
	//printf("pll_rate = %d, rate = %d, cdr = %d value:0x%08x regval:0x%08x\n",pll_rate,rate,cdr,value, cpm_inl(cgu->off));

	/****************改变低30位****************/
	if (clk == DDR)
		regval &= ~(0xf | 0x3f << 24);
	else
		regval &= ~(3 << cgu->stop | 0xff);
	regval |= ((1 << cgu->ce) | cdr);
	cpm_outl(regval, cgu->off);
	while (cpm_inl(cgu->off) & (1 << cgu->busy));
#ifdef DUMP_CGU_SELECT
	printf("%s(0x%x) :0x%x\n",clk_name[clk] ,regval,  cpm_inl(cgu->off));
#endif
	return;
}

void clk_init(void)
{
	/*********打开外设时钟门控开关**********************/
	unsigned int reg_clkgr = cpm_inl(CPM_CLKGR0);
	unsigned int reg_clkgr1 = cpm_inl(CPM_CLKGR1);
	unsigned int gate = 0
#ifdef CONFIG_JZ_MMC_MSC0
		| CPM_CLKGR_MSC0
#endif
#ifdef CONFIG_JZ_MMC_MSC1
		| CPM_CLKGR_MSC1
#endif
#ifdef CONFIG_SFC_NOR
		| CPM_CLKGR_SFC0 | CPM_CLKGR_SFC1
#endif
#ifdef CONFIG_VDE
		| CPM_CLKGR_VDE
#endif
#ifdef CONFIG_HDMI
		| CPM_CLKGR_HDMI
#endif
#ifdef CONFIG_VGA
		|CPM_CLKGR_VGA
#endif
		;
	reg_clkgr &= ~gate;
	cpm_outl(reg_clkgr,CPM_CLKGR0);

	gate = 0
#ifdef CONFIG_XGMAC
		| CPM_CLKGR_GMAC0 |CPM_CLKGR_GMAC1
#endif
#ifdef CONFIG_JPEG
		|CPM_CLKGR_JPEG
#endif
		;

	reg_clkgr1 &= ~gate;
	cpm_outl(reg_clkgr1,CPM_CLKGR1);

	/*************设置外设的时钟源 31:30 ***************/
	cgu_clks_set(cgu_clk_sel, ARRAY_SIZE(cgu_clk_sel));
}

/*************开启串口时钟门控**********************/
void enable_uart_clk(void)
{
	unsigned int clkgr = cpm_inl(CPM_CLKGR0);

	switch (gd->arch.gi->uart_idx) {
#define _CASE(U, N) case U: clkgr &= ~N; break
		_CASE(0, CPM_CLKGR_UART0);
		_CASE(1, CPM_CLKGR_UART1);
		_CASE(2, CPM_CLKGR_UART2);
	default:
		break;
	}
	cpm_outl(clkgr, CPM_CLKGR0);
}

void dump_usb_reg(void)
{
	printf("CPM_USB0PCR1 %#x\n",CPM_USB0PCR1);
	printf("CPM_USB0PCR %#x\n",CPM_USB0PCR);
	printf("CPM_OPCR %#x\n",CPM_OPCR);
	printf("OPCR_SPENDN0 %#x\n",OPCR_SPENDN0);
	printf("USBPCR_POR %#x\n",USBPCR_POR);
	printf("CPM_CLKGR0 %#x\n",CPM_CLKGR0);
	printf("CPM_CLKGR_OTG0 %#x\n",CPM_CLKGR_OTG0);
}

void otg_phy_init(enum otg_mode_t mode, unsigned extclk)
{
#ifndef CONFIG_SPL_BUILD
	int ext_sel = 0;
	int tmp_reg = 0;
	int timeout = 0x7fffff;

	tmp_reg = cpm_inl(CPM_USB0PCR1);
	tmp_reg &= ~(USBPCR1_REFCLKSEL_MSK | USBPCR1_REFCLKDIV_MSK);
	tmp_reg |= USBPCR1_REFCLKSEL_CORE | USBPCR1_WORD_IF0_16_30;
	switch (extclk/1000000) {
		case 12:
			tmp_reg |= USBPCR1_REFCLKDIV_12M;
			break;
		case 19:
			tmp_reg |= USBPCR1_REFCLKDIV_19_2M;
			break;
		case 48:
			tmp_reg |= USBPCR1_REFCLKDIV_48M;
			break;
		default:
			ext_sel = 1;
		case 24:
			tmp_reg |= USBPCR1_REFCLKDIV_24M;
			break;
	}
	cpm_outl(tmp_reg,CPM_USB0PCR1);

#if 0 /* no usb cdr in t5 */
	/*set usb cdr clk*/
	tmp_reg = cpm_inl(CPM_USBCDR);
	tmp_reg &= ~USBCDR_UCS_PLL;
	cpm_outl(tmp_reg, CPM_USBCDR);
	if (ext_sel) {
		unsigned int pll_rate = pll_get_rate(APLL);	//FIXME: default apll
		unsigned int cdr = pll_rate/24000000;
		cdr = cdr ? cdr - 1 : cdr;
		tmp_reg |= (cdr & USBCDR_USBCDR_MSK) | USBCDR_CE_USB;
		tmp_reg &= ~USBCDR_USB_STOP;
		cpm_outl(tmp_reg, CPM_USBCDR);
		while ((cpm_inl(CPM_USBCDR) & USBCDR_USB_BUSY) || timeout--);
		tmp_reg = cpm_inl(CPM_USBCDR);
		tmp_reg &= ~USBCDR_UPCS_MPLL;
		tmp_reg |= USBCDR_UCS_PLL;
		cpm_outl(tmp_reg, CPM_USBCDR);
	} else {
		tmp_reg |= USBCDR_USB_STOP;
		cpm_outl(tmp_reg, CPM_USBCDR);
		while ((cpm_inl(CPM_USBCDR) & USBCDR_USB_BUSY) || timeout--);
	}
	tmp_reg = cpm_inl(CPM_USBCDR);
	tmp_reg &= ~USBCDR_USB_DIS;
	cpm_outl(tmp_reg, CPM_USBCDR);
	if (!timeout)
		printf("USBCDR wait busy bit failed\n");
#endif /* #if 0 */

	tmp_reg = cpm_inl(CPM_USB0PCR);
	switch (mode) {
		case OTG_MODE:
		case HOST_ONLY_MODE:
			tmp_reg |= USBPCR_USB_MODE_ORG;
			tmp_reg &= ~(USBPCR_VBUSVLDEXTSEL|USBPCR_VBUSVLDEXT|USBPCR_OTG_DISABLE);
			break;
		case DEVICE_ONLY_MODE:
			tmp_reg &= ~USBPCR_USB_MODE_ORG;
			tmp_reg |= USBPCR_VBUSVLDEXTSEL|USBPCR_VBUSVLDEXT|USBPCR_OTG_DISABLE;
	}
	cpm_outl(tmp_reg, CPM_USB0PCR);

	tmp_reg = cpm_inl(CPM_OPCR);
	tmp_reg |= OPCR_SPENDN0;
	cpm_outl(tmp_reg, CPM_OPCR);

	tmp_reg = cpm_inl(CPM_USB0PCR);
	tmp_reg |= USBPCR_POR;
	cpm_outl(tmp_reg, CPM_USB0PCR);
	udelay(30);
	tmp_reg = cpm_inl(CPM_USB0PCR);
	tmp_reg &= ~USBPCR_POR;
	cpm_outl(tmp_reg, CPM_USB0PCR);
	udelay(300);

	tmp_reg = cpm_inl(CPM_CLKGR0);
	tmp_reg &= ~CPM_CLKGR_OTG0;
	cpm_outl(tmp_reg, CPM_CLKGR0);
#endif
}

int jpeg_reset()
{
	int tmp_cpm = 0;
	int timeout = 10000;
	tmp_cpm = cpm_inl(CPM_SRBC0);

	cpm_outl(tmp_cpm | CPM_JPEG_STP, CPM_SRBC0);
	while (!(cpm_inl(CPM_SRBC0) & CPM_JPEG_ACK) && --timeout);
	if (timeout == 0) {
		printf("jpeg wait stop ack timeout!\n");
		return -1;
	}
	cpm_outl(tmp_cpm | CPM_JPEG_SR, CPM_SRBC0);
	cpm_outl(tmp_cpm, CPM_SRBC0);
	return 0;
}

void jpeg_clk_gate_open()
{
	int tmp_cpm = 0;
	tmp_cpm = cpm_inl(CPM_CLKGR1);
	cpm_outl(tmp_cpm & (~CPM_CLKGR_JPEG), CPM_CLKGR1);
}

void jpeg_clk_gate_close()
{
	int tmp_cpm = 0;
	tmp_cpm = cpm_inl(CPM_CLKGR1);
	cpm_outl(tmp_cpm | CPM_CLKGR_JPEG, CPM_CLKGR1);
}
