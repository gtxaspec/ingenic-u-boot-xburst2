/*
 * (C) Copyright 2021
 *
 */
#include <config.h>
#include <common.h>
#include <malloc.h>
#include <net.h>
#include <command.h>
#include <asm/io.h>
#include <asm/cache.h>
#include <asm/dma-default.h>
#include <miiphy.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <asm/arch/clk.h>

#include "a1_xgmac.h"

//#define DEBUG_A1_XGMAC
//#define DEBUG1_A1_XGMAC

struct dmamacdescr _tx_mac_descrtable[CONFIG_TX_DESCR_NUM] __attribute__((aligned(1024)));
struct dmamacdescr _rx_mac_descrtable[CONFIG_RX_DESCR_NUM] __attribute__((aligned(1024)));

char _txbuffs[TX_TOTAL_BUFSIZE] __attribute__((aligned(1024)));
char _rxbuffs[RX_TOTAL_BUFSIZE] __attribute__((aligned(1024)));

#ifdef CONFIG_JZ_NET_ETHERNET_DUAL
struct dmamacdescr _tx_mac1_descrtable[CONFIG_TX_DESCR_NUM] __attribute__((aligned(1024)));
struct dmamacdescr _rx_mac1_descrtable[CONFIG_RX_DESCR_NUM] __attribute__((aligned(1024)));

char _txbuffs_mac1[TX_TOTAL_BUFSIZE] __attribute__((aligned(1024)));
char _rxbuffs_mac1[RX_TOTAL_BUFSIZE] __attribute__((aligned(1024)));
#endif

static void dump_xgmac_mdio_regs(int idx)
{
	u32 base;
	if (0 == idx) {
		base = A1_XGMAC0_BASE;
		printf("XGAMC0 MDIO:\n");
	} else {
		base = A1_XGMAC1_BASE;
		printf("XGAMC1 MDIO:\n");
	}
	printf("MDIO_ADDR = 0x%x\n", readl(base+A1_XGMAC_MDIO_ADDR_OFFSET));
	printf("MDIO_DATA = 0x%x\n", readl(base+A1_XGMAC_MDIO_DATA_OFFSET));
	printf("MDIO_C22P = 0x%x\n", readl(base+A1_XGMAC_MDIO_C22P_OFFSET));
}


static void dump_xgmac_regs(int idx)
{
	u32 base;
	u32 i;
	u32 reg;
	if (0 == idx) {
		base = A1_XGMAC0_BASE;
		printf("XGAMC0:\n");
	} else {
		base = A1_XGMAC1_BASE;
		printf("XGAMC1:\n");
	}
#if 0
	printf("XGMAC_ADDRx_LOW_0  = 0x%x\n", readl(base+XGMAC_ADDRx_LOW(0)));
	printf("XGMAC_ADDRx_LOW_1  = 0x%x\n", readl(base+XGMAC_ADDRx_LOW(1)));
	printf("XGMAC_ADDRx_HIGH_0 = 0x%x\n", readl(base+XGMAC_ADDRx_HIGH(0)));
	printf("XGMAC_ADDRx_HIGH_1 = 0x%x\n", readl(base+XGMAC_ADDRx_HIGH(1)));
	printf("XGMAC_DMA_MODE     = 0x%x\n", readl(base+XGMAC_DMA_MODE));
#endif
	for (i = 0; i < XGMAC_MAC_REGSIZE; i++) {
		reg = readl(base+i*4);
		if (reg)
			printf("mac reg-%08x = 0x%08x\n", base+i*4, reg);
	}
	for (i = (XGMAC_DMA_MODE / 4); i < XGMAC_REGSIZE; i++) {
		reg = readl(base+i*4);
		if (reg)
			printf("dma reg-%08x = 0x%08x\n", base+i*4, reg);
	}
}

static u32 gmac_read_reg(unsigned int addr)
{
	u32 data = 0;
	data = *(volatile unsigned int *)(addr);
	return data;
}

static void gmac_write_reg(u32 addr, u32 data)
{
	*(volatile unsigned int *)(addr) = data;
}


static int a1_mido_wait_idle(int idx)
{
	u32 reg = 0;
	u32 loop = 0;
	loop = 100;
	do {
		if (idx) {
			reg = readl(A1_XGMAC1_BASE+A1_XGMAC_MDIO_DATA_OFFSET);
		} else {
			reg = readl(A1_XGMAC0_BASE+A1_XGMAC_MDIO_DATA_OFFSET);
		}
		if (!(reg&MII_XGMAC_BUSY))
			break;
		else
			mdelay(1);
	} while(--loop);
	if (loop <=0 ) {
		printf("Failed to wait a1 mdio idle\n");
		return -1;
	}
	return 0;
}


static int a1_mdio_reset(struct mii_dev *bus)
{
#if 0
	u32 base = 0;

	if(!strcmp("mii0", bus->name)){
		base = A1_XGMAC0_BASE;
	}else if(!strcmp("mii1", bus->name)){
		base = A1_XGMAC1_BASE;
	}else{
		printf("has no phy,mdio name:%s!\n",bus->name);
		return -1;
	}
#endif
	return 0;
}

static int a1_phy_read(struct mii_dev *bus, int phyaddr, int dev_addr, int regnum)
{
	u32 tmp = 0;
	u32 addr = 0, val = 0 ,base = 0;
	int idx = 0;

	if(!strcmp(A1_XGMAC0_NAME_MDIO, bus->name)){
		base = A1_XGMAC0_BASE;
		idx=0;
	}else if(!strcmp(A1_XGMAC1_NAME_MDIO, bus->name)){
		base = A1_XGMAC1_BASE;
		idx=1;
	}else{
		printf("has no phy,mdio name:%s!\n",bus->name);
		return -1;
	}

	/* Set port as Clause 22 */
	tmp = readl(base + A1_XGMAC_MDIO_C22P_OFFSET);
	tmp |= (1<<phyaddr);
	writel(tmp, base + A1_XGMAC_MDIO_C22P_OFFSET);

	addr = (addr << MII_XGMAC_PA_SHIFT) | (regnum & 0x1f);
	val = MII_XGMAC_BUSY|MII_XGMAC_SADDR|MII_XGMAC_READ;
	/* Set clock divider */
	//close CRS, or read error on fpga
	//val |= MII_XGMAC_CRS;
	val |= 0<<MII_XGMAC_CR_SHIFT;
	/* Set the MII address register to read */
	writel(addr, base+A1_XGMAC_MDIO_ADDR_OFFSET);
	writel(val, base+A1_XGMAC_MDIO_DATA_OFFSET);
	/* Wait until any existing MII operation is complete */
	if (a1_mido_wait_idle(idx)<0) {
		printf("Wait  mdio idle timeout!\n");
		return -1;
	}
	/* Read the data from the MII data register */
	tmp = readl(base+A1_XGMAC_MDIO_DATA_OFFSET);
	tmp &= 0xffff;

#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d, phyaddr = %x, dev_addr = %x, regnum = %x value = %x\n",
			__func__, __LINE__, phyaddr, dev_addr, regnum, tmp);
#endif
	return tmp;
}

static int a1_phy_write(struct mii_dev *bus, int phyaddr, int dev_addr, int regnum, u16 value)
{
	u32 tmp = 0;
	u32 addr = 0, val = 0, base = 0;
	int idx = 0;

	if(!strcmp(A1_XGMAC0_NAME_MDIO, bus->name)){
		base = A1_XGMAC0_BASE;
		idx=0;
	}else if(!strcmp(A1_XGMAC1_NAME_MDIO, bus->name)){
		base = A1_XGMAC1_BASE;
		idx=1;
	}else{
		printf("has no phy,mdio name:%s!\n",bus->name);
		return -1;
	}

	//printf("debug info: %s, %d, phyaddr = %x, dev_addr = %x, regnum = %x value = %x\n",
	//	   __func__, __LINE__, phyaddr, dev_addr, regnum, value);
	/* Set port as Clause 22 */
	tmp = readl(base + A1_XGMAC_MDIO_C22P_OFFSET);
	tmp |= (1<<phyaddr);
	writel(tmp, base + A1_XGMAC_MDIO_C22P_OFFSET);

	addr = (addr << MII_XGMAC_PA_SHIFT) | (regnum & 0x1f);
	val = MII_XGMAC_BUSY|MII_XGMAC_SADDR|MII_XGMAC_WRITE;
	/* Set clock divider */

	//close CRS, or read error on fpga
	//val |= MII_XGMAC_CRS;
	val |= 0<<MII_XGMAC_CR_SHIFT;
	/* Set value */
	val |= (value&0xffff);
	/* Set the MII address register to read */
	writel(addr, base+A1_XGMAC_MDIO_ADDR_OFFSET);
	writel(val, base+A1_XGMAC_MDIO_DATA_OFFSET);

	/* Wait until any existing MII operation is complete */
	if (a1_mido_wait_idle(idx)<0) {
		printf("Wait  mdio idle timeout!\n");
		return -1;
	}
	return 0;
}

int a1_mdio_init(bd_t *bis, char* name)
{
	int rv;
	struct mii_dev *bus = mdio_alloc();

	if (!bus) {
		printf("Failed to allocate a1 mdio bus\n");
		return -1;
	}
	bus->read = a1_phy_read;
	bus->write = a1_phy_write;
	bus->reset = a1_mdio_reset;
	sprintf(bus->name, name);
	//bus->priv = info->regs;
	rv = mdio_register(bus);
	return rv;
}

static void tx_descs_init(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct dmamacdescr *desc_table_p = priv->tx_mac_descrtable;
	char *txbuffs = priv->txbuffs;
	struct dmamacdescr *desc_p;
	u32 idx;

	for (idx = 0; idx < CONFIG_TX_DESCR_NUM; idx++) {
		desc_p = &desc_table_p[idx];
		desc_p->des0 = 0;
		desc_p->des1 = 0;
		desc_p->des2 = 0;
		desc_p->des3 = 0;
	}
}

static void rx_descs_init(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct dmamacdescr *desc_table_p = priv->rx_mac_descrtable;
	char *rxbuffs = priv->rxbuffs;
	struct dmamacdescr *desc_p;
	u32 idx;
	for (idx = 0; idx < CONFIG_RX_DESCR_NUM; idx++) {
		desc_p = &desc_table_p[idx];
		desc_p->des0 = virt_to_phys(&rxbuffs[idx * CONFIG_ETH_BUFSIZE]);
		desc_p->des1 = 0;
		desc_p->des2 = 0;
		desc_p->des3 = XGMAC_RDES3_OWN;
		//desc_p->des3 |= XGMAC_RDES3_IOC;
	}
}

static void descs_init(struct eth_device *dev)
{
	tx_descs_init(dev);
	rx_descs_init(dev);

	flush_dcache_all();
	flush_l2cache_all();
}

#define readl_poll_timeout(addr, value, cond, delay_us, timeout_us, name) \
	({																	\
	 int loop = timeout_us/delay_us;									\
	 int timeout = 0;												\
	 do {															\
	 value = readl(addr);										\
	 if (cond)													\
	 break;													\
	 udelay(delay_us);											\
	 } while(timeout_us< delay_us?1:--loop);							\
	 if (loop <=0 ) {												\
	 printf("Failed to wait %s\n", name);						\
	 timeout = 1;												\
	 }																\
	 (timeout);														\
	 })

static int dw_write_hwaddr(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	u32 macid_lo, macid_hi;
	u8 *mac_id = &dev->enetaddr[0];

	macid_lo = mac_id[0] + (mac_id[1] << 8) + \
		   (mac_id[2] << 16) + (mac_id[3] << 24);
	macid_hi = mac_id[4] + (mac_id[5] << 8);
	writel(macid_hi, dev->iobase +XGMAC_ADDRx_HIGH(0));
	writel(macid_lo, dev->iobase+XGMAC_ADDRx_LOW(0));
	return 0;
}

static void a1_xgmac_get_hw_feature(struct dma_features *dma_cap,u32 base)
{
	u32 hw_cap;

	/*  MAC HW feature 0 */
	hw_cap = readl(base + XGMAC_HW_FEATURE0);
	dma_cap->vlins = (hw_cap & XGMAC_HWFEAT_SAVLANINS) >> 27;
	dma_cap->rx_coe = (hw_cap & XGMAC_HWFEAT_RXCOESEL) >> 16;
	dma_cap->tx_coe = (hw_cap & XGMAC_HWFEAT_TXCOESEL) >> 14;
	dma_cap->eee = (hw_cap & XGMAC_HWFEAT_EEESEL) >> 13;
	dma_cap->atime_stamp = (hw_cap & XGMAC_HWFEAT_TSSEL) >> 12;
	dma_cap->av = (hw_cap & XGMAC_HWFEAT_AVSEL) >> 11;
	dma_cap->av &= !((hw_cap & XGMAC_HWFEAT_RAVSEL) >> 10);
	dma_cap->arpoffsel = (hw_cap & XGMAC_HWFEAT_ARPOFFSEL) >> 9;
	dma_cap->rmon = (hw_cap & XGMAC_HWFEAT_MMCSEL) >> 8;
	dma_cap->pmt_magic_frame = (hw_cap & XGMAC_HWFEAT_MGKSEL) >> 7;
	dma_cap->pmt_remote_wake_up = (hw_cap & XGMAC_HWFEAT_RWKSEL) >> 6;
	dma_cap->vlhash = (hw_cap & XGMAC_HWFEAT_VLHASH) >> 4;
	dma_cap->mbps_1000 = (hw_cap & XGMAC_HWFEAT_GMIISEL) >> 1;

	/* MAC HW feature 1 */
	hw_cap = readl(base + XGMAC_HW_FEATURE1);
	dma_cap->l3l4fnum = (hw_cap & XGMAC_HWFEAT_L3L4FNUM) >> 27;
	dma_cap->hash_tb_sz = (hw_cap & XGMAC_HWFEAT_HASHTBLSZ) >> 24;
	dma_cap->rssen = (hw_cap & XGMAC_HWFEAT_RSSEN) >> 20;
	dma_cap->tsoen = (hw_cap & XGMAC_HWFEAT_TSOEN) >> 18;
	dma_cap->sphen = (hw_cap & XGMAC_HWFEAT_SPHEN) >> 17;

	dma_cap->addr64 = (hw_cap & XGMAC_HWFEAT_ADDR64) >> 14;
	switch (dma_cap->addr64) {
		case 0:
			dma_cap->addr64 = 32;
			break;
		case 1:
			dma_cap->addr64 = 40;
			break;
		case 2:
			dma_cap->addr64 = 48;
			break;
		default:
			dma_cap->addr64 = 32;
			break;
	}

	dma_cap->tx_fifo_size =
		128 << ((hw_cap & XGMAC_HWFEAT_TXFIFOSIZE) >> 6);
	dma_cap->rx_fifo_size =
		128 << ((hw_cap & XGMAC_HWFEAT_RXFIFOSIZE) >> 0);

	/* MAC HW feature 2 */
	hw_cap = readl(base + XGMAC_HW_FEATURE2);
	dma_cap->pps_out_num = (hw_cap & XGMAC_HWFEAT_PPSOUTNUM) >> 24;
	dma_cap->number_tx_channel =
		((hw_cap & XGMAC_HWFEAT_TXCHCNT) >> 18) + 1;
	dma_cap->number_rx_channel =
		((hw_cap & XGMAC_HWFEAT_RXCHCNT) >> 12) + 1;
	dma_cap->number_tx_queues =
		((hw_cap & XGMAC_HWFEAT_TXQCNT) >> 6) + 1;
	dma_cap->number_rx_queues =
		((hw_cap & XGMAC_HWFEAT_RXQCNT) >> 0) + 1;

	/* MAC HW feature 3 */
	hw_cap = readl(base + XGMAC_HW_FEATURE3);
	dma_cap->tbssel = (hw_cap & XGMAC_HWFEAT_TBSSEL) >> 27;
	dma_cap->fpesel = (hw_cap & XGMAC_HWFEAT_FPESEL) >> 26;
	dma_cap->estwid = (hw_cap & XGMAC_HWFEAT_ESTWID) >> 23;
	dma_cap->estdep = (hw_cap & XGMAC_HWFEAT_ESTDEP) >> 20;
	dma_cap->estsel = (hw_cap & XGMAC_HWFEAT_ESTSEL) >> 19;
	dma_cap->asp = (hw_cap & XGMAC_HWFEAT_ASP) >> 14;
	dma_cap->dvlan = (hw_cap & XGMAC_HWFEAT_DVLAN) >> 13;
	dma_cap->frpes = (hw_cap & XGMAC_HWFEAT_FRPES) >> 11;
	dma_cap->frpbs = (hw_cap & XGMAC_HWFEAT_FRPPB) >> 9;
	dma_cap->frpsel = (hw_cap & XGMAC_HWFEAT_FRPSEL) >> 3;
}

int a1_eth_config_phy(struct eth_device *dev)
{
	int ret;
	unsigned short id1,id2,reg;
	struct dw_eth_dev *priv = dev->priv;
	u8 phy_addr = priv->address;
	u16 bmsr;
	u16 bmcr;
	u32 timeout;
	ulong start;

	ret = miiphy_read(dev->name, phy_addr, 2, &id1);
	if (ret) {
		printf("gmac: %s, %d, phy read failed!\n", __func__, __LINE__); return -1;
	}

	ret = miiphy_read(dev->name, phy_addr, 3, &id2);
	if (ret) {
		printf("gmac: %s, %d, phy read failed!\n", __func__, __LINE__); return -1;
	}
	printf("gmac: phyid %x-%x,%x\n", id1, id2,phy_addr);

	ret = miiphy_read(dev->name, phy_addr, MII_BMCR, &bmcr);
	if (ret) {
		printf("gmac: %s, %d, phy read failed!\n", __func__, __LINE__); return -1;
	}


	if (bmcr&BMCR_ANENABLE) {
		timeout = (5 * CONFIG_SYS_HZ);
		start = get_timer(0);
		puts("Waiting for PHY auto negotiation to complete");
		while (get_timer(start) < timeout) {
			miiphy_read(dev->name, phy_addr, MII_BMSR, &bmsr);
			if (bmsr & BMSR_ANEGCOMPLETE) {
				priv->phy_configured = 1;
				break;
			}
			/* Print dot all 1s to show progress */
			if ((get_timer(start) % 1000) == 0)
				putc('.');
			/* Try again after 1msec */
			udelay(1000);
		};
		if (!(bmsr & BMSR_ANEGCOMPLETE))
			puts(" TIMEOUT!\n");
		else
			puts(" done\n");
	}
	else {
		priv->phy_configured = 1;
	}


	if ((0x7 == id1)&&(0xc0f1 == id2)) {
		ret = miiphy_read(dev->name, phy_addr, 31, &reg);
		if (ret) {
			printf("gmac: %s, %d, phy read failed!\n", __func__, __LINE__); return -1;
		}
		reg = (reg>>2)&0x7;
		if (0x1 == reg) {
			priv->speed = _10BASET;
			priv->duplex = HALF;
		} else if (0x5 == reg) {
			priv->speed = _10BASET;
			priv->duplex = FULL;
		} else if (0x2 == reg) {
			priv->speed = _100BASET;
			priv->duplex = HALF;
		} else if (0x6 == reg) {
			priv->speed = _100BASET;
			priv->duplex = FULL;
		} else {
			printf("gmac: %s, %d, PHY STATUS ERROR %d!\n", __func__, __LINE__, reg); return -1;
		}
	} else {
		priv->speed = miiphy_speed(dev->name, phy_addr);
		priv->duplex = miiphy_duplex(dev->name, phy_addr);
	}

	if (priv->phy_configured == 1) {
		printf("gmac: phy speed %d, duplex %d\n", priv->speed, priv->duplex);
	}
	return 0;
}


static int a1_eth_init(struct eth_device *dev, bd_t *bis)
{
	u32 value;
	u32 base;
	struct dma_features dma_cap;
	struct dw_eth_dev *priv = dev->priv;
	base = dev->iobase;
	unsigned int clk_id = 0;

	if(!strcmp(A1_XGMAC0_NAME_DEV, dev->name)){

		/* select rmii or rgmii */
		u32 cpm_mac0_ctrl = gmac_read_reg(A1_CPM_XGMAC_MAC0PHYC);
		cpm_mac0_ctrl &= (~0x3);
		if(dev->mii == GMAC_PHY_RMII) // rmii
			cpm_mac0_ctrl |= (A1_CPM_XGMAC_MACPHYC_RMII);
		else 						  // rgmii
			cpm_mac0_ctrl |= (A1_CPM_XGMAC_MACPHYC_RGMII);
		gmac_write_reg(A1_CPM_XGMAC_MAC0PHYC,cpm_mac0_ctrl);
#ifdef DEBUG1_A1_XGMAC
		printf("set %s A1_CPM_XGMAC_MAC0PHYC=%#x!\n",dev->name,gmac_read_reg(A1_CPM_XGMAC_MAC0PHYC));
#endif

		clk_id=MAC0TXCDR;
	}
	else if(!strcmp(A1_XGMAC1_NAME_DEV, dev->name)){

		/* select rmii or rgmii */
		u32 cpm_mac1_ctrl = gmac_read_reg(A1_CPM_XGMAC_MAC1PHYC);
		cpm_mac1_ctrl &= (~0x3);
		if(dev->mii == GMAC_PHY_RMII) // rmii
			cpm_mac1_ctrl |= (A1_CPM_XGMAC_MACPHYC_RMII);
		else 						  // rgmii
			cpm_mac1_ctrl |= (A1_CPM_XGMAC_MACPHYC_RGMII);
		gmac_write_reg(A1_CPM_XGMAC_MAC1PHYC,cpm_mac1_ctrl);
#ifdef DEBUG1_A1_XGMAC
		printf("set %s A1_CPM_XGMAC_MAC1PHYC=%#x!\n",dev->name,gmac_read_reg(A1_CPM_XGMAC_MAC1PHYC));
#endif

		clk_id=MAC1TXCDR;
	}else{

		printf("has no eth,eth name:%s!\n",dev->name);
		return -1;
	}

	a1_eth_config_phy(dev);

	descs_init(dev);
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);

	/* dma reset */
	printf("XGMAC_DMA_MODE = 0x%x\n", readl(base+XGMAC_DMA_MODE));
#endif
	value = readl(base + XGMAC_DMA_MODE);
	/* DMA SW reset */
	writel(value | XGMAC_SWR, base + XGMAC_DMA_MODE);
#ifdef DEBUG1_A1_XGMAC
	printf("XGMAC_DMA_MODE = 0x%x\n", readl(base+XGMAC_DMA_MODE));
#endif



	if (readl_poll_timeout(base + XGMAC_DMA_MODE, value, !(value & XGMAC_SWR), 10, 100000, "dma reset")) {
		printf("XGMAC_DMA_MODE  = 0x%x\n", readl(base+XGMAC_DMA_MODE));
		printf("Failed to reset dma!\n");
		return -1;
	}

#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* dma init */
	value = readl(base + XGMAC_DMA_SYSBUS_MODE);
	//value |= XGMAC_AAL;
	//value |= XGMAC_EAME;
	writel(value, base + XGMAC_DMA_SYSBUS_MODE);

	/* axi config */
	value = readl(base + XGMAC_DMA_SYSBUS_MODE);
	//value |= XGMAC_EN_LPI;
	//value |= XGMAC_LPI_XIT_PKT;
	value &= ~XGMAC_WR_OSR_LMT;
	value |= (1 << XGMAC_WR_OSR_LMT_SHIFT) &
		XGMAC_WR_OSR_LMT;
	value &= ~XGMAC_RD_OSR_LMT;
	value |= (1 << XGMAC_RD_OSR_LMT_SHIFT) &
		XGMAC_RD_OSR_LMT;
	value |= XGMAC_UNDEF;
	value &= ~XGMAC_BLEN;
	//value |= XGMAC_BLEN256;
	//value |= XGMAC_BLEN128;
	//value |= XGMAC_BLEN64;
	value |= XGMAC_BLEN32;
	//value |= XGMAC_BLEN16;
	//value |= XGMAC_BLEN8;
	//value |= XGMAC_BLEN4;
	writel(value, base + XGMAC_DMA_SYSBUS_MODE);
	//writel(XGMAC_TDPS, base + XGMAC_TX_EDMA_CTRL);
	//writel(0xc0000000, base + XGMAC_TX_EDMA_CTRL);
	//writel(XGMAC_RDPS, base + XGMAC_RX_EDMA_CTRL);
	writel(0xc0000000, base + XGMAC_RX_EDMA_CTRL);
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d, value = 0x%x, XGMAC_RDPS = 0x%08x\n",
			__func__, __LINE__, value, XGMAC_RDPS);
#endif
	/* DMA CSR Channel configuration */
	u32 chan;
	for (chan = 0; chan < 1; chan++) {
		value = readl(base + XGMAC_DMA_CH_CONTROL(chan));
		value |= XGMAC_PBLx8;
		writel(value, base + XGMAC_DMA_CH_CONTROL(chan));
		//interrupt
		//writel(XGMAC_DMA_INT_DEFAULT_EN, base + XGMAC_DMA_CH_INT_EN(chan));
	}
#ifdef DEBUG_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* DMA RX Channel Configuration */
	for (chan = 0; chan < 1; chan++) {
		value = readl(base + XGMAC_DMA_CH_RX_CONTROL(chan));
		value &= ~XGMAC_RxPBL;
		value |= (8 << XGMAC_RxPBL_SHIFT) & XGMAC_RxPBL;

		value &= ~XGMAC_RBSZ;
		value |= (CONFIG_ETH_BUFSIZE << XGMAC_RBSZ_SHIFT) & XGMAC_RBSZ;

		writel(value, base + XGMAC_DMA_CH_RX_CONTROL(chan));
		writel(0, base + XGMAC_DMA_CH_RxDESC_HADDR(chan));
		writel(virt_to_phys(priv->rx_mac_descrtable),
				base + XGMAC_DMA_CH_RxDESC_LADDR(chan));
		writel(virt_to_phys(&priv->rx_mac_descrtable[CONFIG_RX_DESCR_NUM-1]),
				base + XGMAC_DMA_CH_RxDESC_TAIL_LPTR(chan));
	}
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* DMA TX Channel Configuration */
	for (chan = 0; chan < 1; chan++) {
		value = readl(base + XGMAC_DMA_CH_TX_CONTROL(chan));
		value &= ~XGMAC_TxPBL;
		value |= (8 << XGMAC_TxPBL_SHIFT) & XGMAC_TxPBL;
		value |= XGMAC_OSP;
		writel(value, base + XGMAC_DMA_CH_TX_CONTROL(chan));
		writel(0, base + XGMAC_DMA_CH_TxDESC_HADDR(chan));
		writel(virt_to_phys(priv->tx_mac_descrtable),
				base + XGMAC_DMA_CH_TxDESC_LADDR(chan));
		writel(virt_to_phys(priv->tx_mac_descrtable),
				base + XGMAC_DMA_CH_TxDESC_TAIL_LPTR(chan));
	}
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* Copy the MAC addr into the HW  */
	dw_write_hwaddr(dev);
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* Initialize the MAC Core */
	u32 tx,rx;
	tx = readl(base + XGMAC_TX_CONFIG);
	rx = readl(base + XGMAC_RX_CONFIG);
	tx |= XGMAC_CORE_INIT_TX;
	rx |= XGMAC_CORE_INIT_RX;

	tx |= XGMAC_CONFIG_TE;
	tx &= ~XGMAC_CONFIG_SS_MASK;

	if (priv->speed == 100) {
		tx |= XGMAC_CONFIG_SS_100_MII;
		if(dev->mii == GMAC_PHY_RMII){
			if(!strcmp(dev->name ,A1_XGMAC1_NAME_DEV )){
                                clk_set_rate(MAC1PHY, A1_MAC1PHY_RMII1_MACCDR);
			}else{
                                clk_set_rate(MAC0PHY, A1_MAC0PHY_RMII0_MACCDR);
			}
		}else{
			if(!strcmp(dev->name ,A1_XGMAC1_NAME_DEV )){
				clk_set_rate(MAC1TXCDR, A1_CPM_MACCDR_25MHZ);

			}else{
				clk_set_rate(MAC0TXCDR, A1_CPM_MACCDR_25MHZ);
			}
		}
	}
	else if(priv->speed == 1000){
		tx |= XGMAC_CONFIG_SS_1000_GMII;

		if(!strcmp(dev->name ,A1_XGMAC1_NAME_DEV )){
			clk_set_rate(MAC1TXCDR, A1_CPM_MACCDR_125MHZ);
		}else{
			clk_set_rate(MAC0TXCDR, A1_CPM_MACCDR_125MHZ);
		}
	}
	else{
		tx |= XGMAC_CONFIG_SS_10_MII;
		if(dev->mii == GMAC_PHY_RMII){
			if(!strcmp(dev->name ,A1_XGMAC1_NAME_DEV )){
				clk_set_rate(MAC1PHY, A1_CPM_MACCDR_50MHZ);
			}else{
				clk_set_rate(MAC0PHY, A1_CPM_MACCDR_50MHZ);
			}
		}else{
			if(!strcmp(dev->name ,A1_XGMAC1_NAME_DEV )){
				clk_set_rate(MAC1TXCDR, A1_CPM_MACCDR_2_5MHZ);
			}else{
				clk_set_rate(MAC0TXCDR, A1_CPM_MACCDR_2_5MHZ);
			}
		}
	}

	writel(tx, base + XGMAC_TX_CONFIG);
	writel(rx, base + XGMAC_RX_CONFIG);
	writel(XGMAC_INT_DEFAULT_EN, base + XGMAC_INT_EN);
#ifdef DEBUG_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* Initialize MTL*/
	/* Configure MTL RX algorithms */
	// need config when  queues >1
	/* Configure MTL TX algorithms */
	// need config when  queues >1
	/* Configure CBS in AVB TX queues */
	// need config when  queues >1
	/* Map RX MTL to DMA channels */
	u32 queue;
	for (queue = 0; queue < 1; queue++) {
		u32 value, reg;
		reg = (queue < 4) ? XGMAC_MTL_RXQ_DMA_MAP0 : XGMAC_MTL_RXQ_DMA_MAP1;
		if (queue >= 4)
			queue -= 4;
		value = readl(base + reg);
		value &= ~XGMAC_QxMDMACH(queue);
		value |= (chan << XGMAC_QxMDMACH_SHIFT(queue)) & XGMAC_QxMDMACH(queue);
		writel(value, base + reg);
	}
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* Enable MAC RX Queues */
	for (queue = 0; queue < 1; queue++) {
		u32 value;
		u8 mode = MTL_QUEUE_DCB;
		value = readl(base + XGMAC_RXQ_CTRL0) & ~XGMAC_RXQEN(queue);
		if (mode == MTL_QUEUE_AVB)
			value |= 0x1 << XGMAC_RXQEN_SHIFT(queue);
		else if (mode == MTL_QUEUE_DCB)
			value |= 0x2 << XGMAC_RXQEN_SHIFT(queue);
		writel(value, base + XGMAC_RXQ_CTRL0);
	}
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* Set RX priorities */
	// need config when  queues >1
	/* Set TX priorities */
	// need config when  queues >1
	/* Set RX routing */
	// need config when  queues >1
	/* Receive Side Scaling */
	// need config when  queues >1
#if 0
	/* RX IPC Checksum Offload */
	value = readl(base + XGMAC_RX_CONFIG);
	value |= XGMAC_CONFIG_IPC;
	writel(value, base + XGMAC_RX_CONFIG);
	if(!(readl(base + XGMAC_RX_CONFIG) & XGMAC_CONFIG_IPC)) {
		printf("Failed to set RX IPC Checksum Offload!\n");
	}
#endif
#if 1
	value = readl(base + XGMAC_PACKET_FILTER);
	value |= XGMAC_FILTER_RA;
	writel(value, base + XGMAC_PACKET_FILTER);
#endif
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* Enable the MAC Rx/Tx */
	tx = readl(base + XGMAC_TX_CONFIG);
	rx = readl(base + XGMAC_RX_CONFIG);
#ifdef DEBUG_A1_XGMAC
	printf("debug info: %s, %d,XGMAC_TX_CONFIG F:%#x\n", __func__, __LINE__,tx);
#endif
	tx |= XGMAC_CONFIG_TE;
	tx |= XGMAC_CONFIG_TC;
	rx |= XGMAC_CONFIG_RE;

	writel(tx, base + XGMAC_TX_CONFIG);
	writel(rx, base + XGMAC_RX_CONFIG);
#ifdef DEBUG_A1_XGMAC
	printf("debug info: %s, %d,XGMAC_TX_CONFIG E:%#x\n", __func__, __LINE__,tx);
#endif

	a1_xgmac_get_hw_feature(&dma_cap,base);
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* Set the HW DMA mode and the COE */
	for (chan = 0; chan < 1; chan++) {

		u32 value = readl(base + XGMAC_MTL_RXQ_OPMODE(chan));
#if 0
		value |= XGMAC_RSF;

		value &= ~XGMAC_RQS;
		u32 rxfifosz = dma_cap.rx_fifo_size/16;
		u32 rqs = rxfifosz/256-1;
		printf("debug info: %s, %d rxfifosz = %d, rqs = %d\n",
				__func__, __LINE__, rxfifosz, rqs);
		value |= (7/*rqs*/ << XGMAC_RQS_SHIFT) & XGMAC_RQS;

		/* FUF */
		//value |= 0x78;
		/* flow control: todo */

		writel(value, base + XGMAC_MTL_RXQ_OPMODE(chan));
#endif
		/* Enable MTL RX overflow */
		value = readl(base + XGMAC_MTL_QINTEN(chan));
		writel(value | XGMAC_RXOIE, base + XGMAC_MTL_QINTEN(chan));
	}
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	/* set TX and RX rings length */
	for (chan = 0; chan < 1; chan++) {
		writel(CONFIG_TX_DESCR_NUM-1, base + XGMAC_DMA_CH_TxDESC_RING_LEN(chan));
	}
	for (chan = 0; chan < 1; chan++) {
		writel(CONFIG_RX_DESCR_NUM-1, base + XGMAC_DMA_CH_RxDESC_RING_LEN(chan));
	}

	/* Start the ball rolling... */
	//RX
	for (chan = 0; chan < 1; chan++) {
		value = readl(base + XGMAC_DMA_CH_RX_CONTROL(chan));
		value |= XGMAC_RXST;
		writel(value, base + XGMAC_DMA_CH_RX_CONTROL(chan));

		value = readl(base + XGMAC_RX_CONFIG);
		value |= XGMAC_CONFIG_RE;
		writel(value, base + XGMAC_RX_CONFIG);
	}
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	//TX
	for (chan = 0; chan < 1; chan++) {
		value = readl(base + XGMAC_DMA_CH_TX_CONTROL(chan));
		value |= XGMAC_TXST;
		writel(value, base + XGMAC_DMA_CH_TX_CONTROL(chan));
		value = readl(base + XGMAC_TX_CONFIG);
		value |= XGMAC_CONFIG_TE;
		writel(value, base + XGMAC_TX_CONFIG);
	}
	priv->tx_currdescnum = 0;
	priv->rx_currdescnum = 0;
#ifdef DEBUG1_A1_XGMAC
	printf("debug info: %s, %d\n", __func__, __LINE__);
#endif
	return 0;
}

static void dump_tx_pkt_data(unsigned char *data, int len)
{
	int i = 0;
	printf("tx:\n");
	printf("\t0x0000: ");
	for (i = 0; i < len; i++) {
		printf("%02x ", data[i]);

		if ((i % 8) == 7)
			printf(" ");

		if ( (i != 0) && ((i % 16) == 15) )
			printf("\n\t0x%04x: ", i+1);
	}
	printf("\n");
}

static void dump_tx_des(void* des , struct dw_eth_dev *priv ,int idx)
{
	int i;
	struct dmamacdescr *desc_p = des;
	if (des)
		printf("\ntx desp(0x%x): 0x%08x 0x%08x 0x%08x 0x%08x\n",
				desc_p,
				*(volatile u32 *)virt_uncached((u32)&desc_p->des0),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des1),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des2),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des3));

	printf("-------------------tx---------------------\n");
	for (i = 0; i < CONFIG_TX_DESCR_NUM; i++) {
		if(idx == 0)
			desc_p = &_tx_mac_descrtable[i];
		else
			desc_p = &_tx_mac1_descrtable[i];
		printf("desp-%2d(0x%x): 0x%08x 0x%08x 0x%08x 0x%08x\n",
				i, desc_p,
				*(volatile u32 *)virt_uncached((u32)&desc_p->des0),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des1),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des2),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des3));
	}
}

u32 read_uncached(u32* addr)
{
	return *(volatile u32 *)virt_uncached((u32)addr);
}
void write_uncached(u32* addr, u32 data)
{
	*(volatile u32 *)virt_uncached((u32)addr) = data;
}


static void dump_rx_des(void* des , struct dw_eth_dev *priv ,int idx)
{
	int i;
	struct dmamacdescr *desc_p = des;
	if (des)
		printf("\nrx desp(0x%x): 0x%08x 0x%08x 0x%08x 0x%08x\n",
				desc_p,
				*(volatile u32 *)virt_uncached((u32)&desc_p->des0),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des1),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des2),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des3));
	printf("----------------rx------------------------\n");
	for (i = 0; i < CONFIG_RX_DESCR_NUM; i++) {
		if(idx == 0)
			desc_p =  &_rx_mac_descrtable[i];//&((priv->rx_mac_descrtable)[i]);
		else
			desc_p =  &_rx_mac1_descrtable[i];//&((priv->rx_mac_descrtable)[i]);

		printf("desp-%2d(0x%x): 0x%08x 0x%08x 0x%08x 0x%08x\n",
				i, desc_p,
				*(volatile u32 *)virt_uncached((u32)&desc_p->des0),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des1),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des2),
				*(volatile u32 *)virt_uncached((u32)&desc_p->des3));
		u32 tmp = *(volatile u32 *)virt_uncached((u32)&desc_p->des3);
		if (!(tmp & 0x80000000)) {
			printf("\t\t\t\t des0    = 0x%08x\n", desc_p->des0);
			printf("\t\t\t\t data[0] = 0x%08x\n", read_uncached(desc_p->des0|0x80000000));
		}
	}
}

static int a1_eth_send(struct eth_device *dev, void *packet, int length)
{
	u32 base;
	u32 wait_delay = 20000;
	struct dw_eth_dev *priv = dev->priv;
#ifdef DEBUG_A1_XGMAC
	printf("debug info: %s start, %d ,%s,tx_mac_descrtable= %#x,tx_currdescnum= %d\n", __func__, __LINE__ , dev->name , priv->tx_mac_descrtable ,priv->tx_currdescnum);
#endif
	struct dmamacdescr *desc_table_p = priv->tx_mac_descrtable;
	char *txbuffs = priv->txbuffs;
	struct dmamacdescr *desc_p,*desc_p_next;
	u32 cur = priv->tx_currdescnum;
	base = dev->iobase;
	int idx = 0;

	if(!strcmp(A1_XGMAC0_NAME_DEV, dev->name)){
		idx =0;
#ifdef DEBUG_A1_XGMAC
		printf("a1_eth_send %s 0xb00000c8=%#x!\n",dev->name,*(volatile u32 *)(0xb00000c8));
#endif
	}else if(!strcmp(A1_XGMAC1_NAME_DEV, dev->name)){
		idx =1;
#ifdef DEBUG_A1_XGMAC
		printf("a1_eth_send %s 0xb00000d8=%#x!\n",dev->name,*(volatile u32 *)(0xb00000d8));
#endif
	}else{
		printf("has no eth,eth name:%s!\n",dev->name);
		return -1;
	}

#ifdef DEBUG_A1_XGMAC
	dump_tx_pkt_data(packet, length);
#endif
	desc_p = &desc_table_p[cur];
	/* Check if the descriptor is owned by CPU */
	u32 des3 = read_uncached(&desc_p->des3);
	if (des3 & XGMAC_TDES3_OWN) {
		printf("CPU not owner of tx frame\n");
		return -1;
	}
	write_uncached(&desc_p->des0, virt_to_phys(packet));
	write_uncached(&desc_p->des1, 0);
	write_uncached(&desc_p->des2, length & XGMAC_TDES2_B1L);
	u32 tdes3 = length & XGMAC_TDES3_FL;
	tdes3 |= XGMAC_TDES3_FD;
	tdes3 |= XGMAC_TDES3_LD;
	tdes3 |= XGMAC_TDES3_OWN;
	write_uncached(&desc_p->des3, tdes3);

	flush_dcache_range(packet, (char*)packet+length);
	flush_l2cache_range(packet, (char*)packet+length);
#ifdef DEBUG_A1_XGMAC
	printf("debug info: %s, %d ,%s,%#x,%d\n", __func__, __LINE__ , dev->name , priv->tx_mac_descrtable ,priv->tx_currdescnum);

	dump_tx_des(desc_p,priv,idx);
	dump_rx_des(0,priv,idx);
#endif
	if (++cur >= CONFIG_TX_DESCR_NUM)
		cur = 0;
	priv->tx_currdescnum = cur;
	//set tail to next
	desc_p_next = &desc_table_p[cur];
	writel(virt_to_phys(desc_p_next), base + XGMAC_DMA_CH_TxDESC_TAIL_LPTR(0));

	while(--wait_delay && (read_uncached(&desc_p->des3) & XGMAC_TDES3_OWN)) {
		udelay(1000);
	}
#ifdef DEBUG_A1_XGMAC
	dump_tx_des(desc_p,priv,idx);
	dump_rx_des(0,priv,idx);
#endif
	if(wait_delay == 0)
	{
		printf("Failed to tx packet!\n");
		dump_xgmac_regs(idx);
		return -1;
	}
#ifdef DEBUG_A1_XGMAC
	dump_xgmac_regs(idx);
#endif
	return 0;
}


static void dump_rx_pkt_data(unsigned char *data, int len)
{
	int i = 0;
	printf("rx: data = 0x%08x len = %d\n", data, len);
	printf("\t0x0000: ");
	u32* buf = (u32*)data;
	for (i = 0; i < len/4; i++) {
		printf("%08x ", *(volatile u32 *)virt_uncached((u32)&buf[i]));

		if ((i % 8) == 7)
			printf(" ");

		if ( (i != 0) && ((i % 4) == 3))
			printf("\n\t0x%04x: ", (i+1)*4);
	}
	printf("\n");
	printf("\t0x0000: ");
	buf = (u32*)data;
	for (i = 0; i < len/4; i++) {
		printf("%08x ", buf[i]);
		if ((i % 8) == 7)
			printf(" ");

		if ( (i != 0) && ((i % 4) == 3))
			printf("\n\t0x%04x: ", (i+1)*4);
	}
	printf("\n");
}


static int a1_eth_recv(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	u32 cur = priv->rx_currdescnum;
	struct dmamacdescr *desc_table_p = priv->rx_mac_descrtable;
	struct dmamacdescr *desc_p, *desc_p_next;
	int length = -1;
	char *buf;
	char *rxbuffs;
	u32 base;
	int idx = 0;

	if(!strcmp(A1_XGMAC0_NAME_DEV, dev->name)){
		idx =0;
	}else if(!strcmp(A1_XGMAC1_NAME_DEV, dev->name)){
		idx =1;
	}else{
		printf("has no eth,eth name:%s!\n",dev->name);
		return -1;
	}

	base = dev->iobase;
	/* Check  if the owner is the CPU */
	desc_p = &desc_table_p[cur];
#ifdef DEBUG_A1_XGMAC
	dump_rx_des(desc_p,priv,idx);
	printf("debug info: %s, %d, cur = %d, %s ,rx_mac_descrtable= %#x\n", __func__, __LINE__, cur ,dev->name, priv->rx_mac_descrtable);
#endif
	u32 des3 = read_uncached(&desc_p->des3);
	rxbuffs = priv->rxbuffs;
	buf  = &rxbuffs[cur * CONFIG_ETH_BUFSIZE];
	if (!(des3 & XGMAC_RDES3_OWN)) {
		length = des3 & XGMAC_RDES3_PL;

		invalid_dcache_range(buf, buf+CONFIG_ETH_BUFSIZE);
		invalid_l2cache_range(buf, buf+CONFIG_ETH_BUFSIZE);
#ifdef DEBUG_A1_XGMAC
		dump_rx_pkt_data(buf, length);
#endif
		NetReceive(buf, length);

		memset(buf, 0xff, length);
		flush_dcache_range(buf, buf+CONFIG_ETH_BUFSIZE);
		flush_l2cache_range(buf, buf+CONFIG_ETH_BUFSIZE);

		write_uncached(&desc_p->des0, virt_to_phys(&rxbuffs[cur * CONFIG_ETH_BUFSIZE]));
		write_uncached(&desc_p->des1, 0);
		write_uncached(&desc_p->des2, 0);
		write_uncached(&desc_p->des3, XGMAC_RDES3_OWN);

		++cur;
		if (cur >= CONFIG_RX_DESCR_NUM)
			cur = 0;
		priv->rx_currdescnum = cur;
		desc_p_next = &desc_table_p[cur];
		writel(virt_to_phys(desc_p), base + XGMAC_DMA_CH_RxDESC_TAIL_LPTR(0));
	}
#ifdef DEBUG_A1_XGMAC
	//dump_rx_pkt_data(buf, 32);
	printf("### debug info: %s, %d, cur = %d\n", __func__, __LINE__, cur);
	dump_rx_des(desc_p,priv,idx);
	dump_xgmac_regs(idx);
	printf("debug info: %s, %d, length = %d\n", __func__, __LINE__, length);
#endif
	return length;
}

static void a1_eth_halt(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
}

static int a1_eth_special_config(struct eth_device *dev)
{
        int ret;
	unsigned short id1,id2,reg;
	struct dw_eth_dev *priv = dev->priv;
	u8 phy_addr = priv->address;

	ret = miiphy_read(dev->name, phy_addr, 2, &id1);
	if (ret) {
		printf("gmac: %s, %d, phy read failed!\n", __func__, __LINE__); return -1;
	}

	ret = miiphy_read(dev->name, phy_addr, 3, &id2);
	if (ret) {
		printf("gmac: %s, %d, phy read failed!\n", __func__, __LINE__); return -1;
	}
        /*
         sz18201 phy enabled sleep function default,it will disabled txc clock when unplug network cable 40s.
         And txc clock won't recover without a cable. disabled sleep function. Edwin.Wen@20230525
        */
	if ((0 == id1)&&(0x128 == id2)) {
                ret = miiphy_write(dev->name,phy_addr,0x1e,0x2027);
                if (ret) {
                        printf("gmac: %s, %d, phy write failed!\n", __func__, __LINE__); return -1;
                }
                ret = miiphy_write(dev->name,phy_addr,0x1f,0x2026);
                if (ret) {
                        printf("gmac: %s, %d, phy write failed!\n", __func__, __LINE__); return -1;
                }
                printf("gmac: sz18201 disabled sleep\n");
        }
}
#ifdef CONFIG_JZ_NET_ETHERNET_DUAL
int a1_eth1_initialize(bd_t *bis)
{
	int ret = -1;
	struct eth_device *dev;
	struct dw_eth_dev *priv;

	udelay(5000);


	//gpio init,rgmii pins and mdio pins
	gpio_set_func(A1_GMAC1_PORTS_GROUP, A1_GMAC1_PORT_INIT_FUNC, A1_GMAC1_PORT_PINS);

	ret = a1_mdio_init(bis, A1_XGMAC1_NAME_MDIO);
	if (ret) {
		printf("gmac: mdio1 init failed! ret = %d\n", ret);
		return -1;
	}
	dev = (struct eth_device *) malloc(sizeof(struct eth_device));
	if (!dev)
		return -ENOMEM;
	/*
	 * Since the priv structure contains the descriptors which need a strict
	 * buswidth alignment, memalign is used to allocate memory
	 */
	priv = (struct dw_eth_dev *) memalign(16, sizeof(struct dw_eth_dev));
	if (!priv) {
		free(dev);
		return -ENOMEM;
	}

	memset(dev, 0, sizeof(struct eth_device));
	memset(priv, 0, sizeof(struct dw_eth_dev));

	priv->tx_mac_descrtable = _tx_mac1_descrtable;
	priv->rx_mac_descrtable = _rx_mac1_descrtable;
	priv->txbuffs = _txbuffs_mac1;
	priv->rxbuffs = _rxbuffs_mac1;
	printf("gmac: tx_mac1_descrtable : 0x%08x\n", priv->tx_mac_descrtable);
	printf("gmac: rx_mac1_descrtable : 0x%08x\n", priv->rx_mac_descrtable);
	printf("gmac: txbuffs_mac1           : 0x%08x\n", priv->txbuffs);
	printf("gmac: rxbuffs_mac1           : 0x%08x\n", priv->rxbuffs);
	memset(_tx_mac1_descrtable, 0, sizeof(_tx_mac1_descrtable[CONFIG_TX_DESCR_NUM]));
	memset(_rx_mac1_descrtable, 0, sizeof(_rx_mac1_descrtable[CONFIG_TX_DESCR_NUM]));
	memset(_txbuffs_mac1, 0, sizeof(_txbuffs_mac1[TX_TOTAL_BUFSIZE]));
	memset(_rxbuffs_mac1, 0, sizeof(_rxbuffs_mac1[RX_TOTAL_BUFSIZE]));

	char *ethmii = getenv("eth1mii");
	printf("gmac debug: eth1mii = %s\n", ethmii);
	if(ethmii != NULL){
		if(!strcmp(ethmii,"rmii")){
			clk_set_rate(MAC1PHY, A1_MAC1PHY_RMII1_MACCDR);
#ifdef DEBUG1_A1_XGMAC
			printf(" a1_eth1_initialize set rate RMII1_MACCDR_%d\r\n",A1_MAC1PHY_RMII1_MACCDR);
#endif
			dev->mii = GMAC_PHY_RMII;
		}else{
			clk_set_rate(MAC1PHY, A1_CPM_MACCDR_25MHZ);
			clk_set_rate(MAC1TXCDR, A1_CPM_MACCDR_125MHZ);
#ifdef DEBUG1_A1_XGMAC
			printf(" a1_eth1_initialize set rate A1_CPM_MACCDR_125MHZ\r\n");
#endif
			dev->mii = GMAC_PHY_RGMII;
		}
	}
	else{
		printf("gmac debug: eth1mii env not set,default use rmii,set 50Mhz\n");
		dev->mii = GMAC_PHY_RMII;
		clk_set_rate(MAC1PHY, A1_CPM_MACCDR_50MHZ);
	}

	sprintf(dev->name, A1_XGMAC1_NAME_DEV);
	dev->iobase = (int)A1_XGMAC1_BASE;
	dev->priv = priv;
	dev->index = 1;
	eth_getenv_enetaddr_by_index("eth", dev->index, &dev->enetaddr[0]);

	priv->dev = dev;
	priv->address = CONFIG_ETH1PHYADDR;   /*phy_addr*/
	priv->phy_configured = 0;
	priv->interface = 0;    /*interface*/

	dev->init = a1_eth_init;
	dev->send = a1_eth_send;
	dev->recv = a1_eth_recv;
	dev->halt = a1_eth_halt;
	dev->write_hwaddr = dw_write_hwaddr;

	eth_register(dev);

#ifdef CONFIG_BOARD_CALF_1_3
	gpio_direction_output(A1_GMAC1_PHY_RXD3_PIN, 0);
	gpio_direction_output(A1_GMAC1_PHY_RXDV_PIN, 1);
#endif
	//reset PHY chip
	//Edwin.Wen@2022/05/20 do not modify this reset sequence.start
	mdelay(5);
	gpio_direction_output(A1_GMAC1_PHY_RESET_PIN, A1_GMAC_PHY_RESET_ENLEVEL);
	mdelay(12);
	gpio_direction_output(A1_GMAC1_PHY_RESET_PIN, !A1_GMAC_PHY_RESET_ENLEVEL);
	mdelay(80);
	//Edwin.Wen@2022/05/20 end

        a1_eth_special_config(dev);

#ifdef DEBUG1_A1_XGMAC
	printf("\r\n\r\n");
#endif
	return 0;
}
#endif

int a1_eth_initialize(bd_t *bis)
{
	int ret = -1;
	struct eth_device *dev;
	struct dw_eth_dev *priv;

	udelay(5000);

	//gpio init,rgmii pins and mdio pins
	gpio_set_func(A1_GMAC0_PORTS_GROUP, A1_GMAC0_PORT_INIT_FUNC, A1_GMAC0_PORT_PINS);

	ret = a1_mdio_init(bis, A1_XGMAC0_NAME_MDIO);
	if (ret) {
		printf("gmac: mdio init failed! ret = %d\n", ret);
		return -1;
	}
	dev = (struct eth_device *) malloc(sizeof(struct eth_device));
	if (!dev)
		return -ENOMEM;
	/*
	 * Since the priv structure contains the descriptors which need a strict
	 * buswidth alignment, memalign is used to allocate memory
	 */
	priv = (struct dw_eth_dev *) memalign(16, sizeof(struct dw_eth_dev));
	if (!priv) {
		free(dev);
		return -ENOMEM;
	}

	memset(dev, 0, sizeof(struct eth_device));
	memset(priv, 0, sizeof(struct dw_eth_dev));

	priv->tx_mac_descrtable = _tx_mac_descrtable;
	priv->rx_mac_descrtable = _rx_mac_descrtable;
	priv->txbuffs = _txbuffs;
	priv->rxbuffs = _rxbuffs;
	printf("gmac: tx_mac_descrtable : 0x%08x\n", priv->tx_mac_descrtable);
	printf("gmac: rx_mac_descrtable : 0x%08x\n", priv->rx_mac_descrtable);
	printf("gmac: txbuffs           : 0x%08x\n", priv->txbuffs);
	printf("gmac: rxbuffs           : 0x%08x\n", priv->rxbuffs);
	memset(_tx_mac_descrtable, 0, sizeof(_tx_mac_descrtable[CONFIG_TX_DESCR_NUM]));
	memset(_rx_mac_descrtable, 0, sizeof(_rx_mac_descrtable[CONFIG_TX_DESCR_NUM]));
	memset(_txbuffs, 0, sizeof(_txbuffs[TX_TOTAL_BUFSIZE]));
	memset(_rxbuffs, 0, sizeof(_rxbuffs[RX_TOTAL_BUFSIZE]));

	/* mac0ptpcdr this clock not used, but needs to be initialized, set rate is 50MHz, */
	clk_set_rate(MAC0PTPCDR, 50000000);

	char *ethmii = getenv("eth0mii");
	printf("gmac debug: eth0mii = %s\n", ethmii);
	if(ethmii != NULL){
		if(!strcmp(ethmii,"rmii")){
			clk_set_rate(MAC0PHY, A1_MAC0PHY_RMII0_MACCDR);
#ifdef DEBUG1_A1_XGMAC
			printf(" a1_eth_initialize set rate RMII0_MACCDR_%d\r\n",A1_MAC0PHY_RMII0_MACCDR);
#endif
			dev->mii = GMAC_PHY_RMII;
		}else{
			clk_set_rate(MAC0PHY, A1_CPM_MACCDR_25MHZ);
			clk_set_rate(MAC0TXCDR, A1_CPM_MACCDR_125MHZ);
#ifdef DEBUG1_A1_XGMAC
			printf(" a1_eth_initialize set rate A1_CPM_MACCDR_125MHZ\r\n");
#endif
			dev->mii = GMAC_PHY_RGMII;
		}
	}
	else{
		printf("gmac debug: eth0mii env not set,default use rmii,set 50Mhz\n");
		dev->mii = GMAC_PHY_RMII;
		clk_set_rate(MAC0PHY, A1_CPM_MACCDR_50MHZ);
	}

	sprintf(dev->name, A1_XGMAC0_NAME_DEV);
	dev->iobase = (int)A1_XGMAC0_BASE;
	dev->priv = priv;
	dev->index = 0;

	eth_getenv_enetaddr_by_index("eth", dev->index, &dev->enetaddr[0]);

	priv->dev = dev;
	priv->address = CONFIG_ETH0PHYADDR;      /*phy_addr*/
	priv->phy_configured = 0;
	priv->interface = 0;    /*interface*/

	dev->init = a1_eth_init;
	dev->send = a1_eth_send;
	dev->recv = a1_eth_recv;
	dev->halt = a1_eth_halt;
	dev->write_hwaddr = dw_write_hwaddr;

	eth_register(dev);

        //reset PHY chip
	//Edwin.Wen@2022/05/20 do not modify this reset sequence.start
	mdelay(5);
	gpio_direction_output(A1_GMAC0_PHY_RESET_PIN, A1_GMAC_PHY_RESET_ENLEVEL);
	mdelay(12);
	gpio_direction_output(A1_GMAC0_PHY_RESET_PIN, !A1_GMAC_PHY_RESET_ENLEVEL);
	mdelay(80);
	//Edwin.Wen@2022/05/20 end

        a1_eth_special_config(dev);

#ifdef DEBUG1_A1_XGMAC
	printf("\r\n\r\n");
#endif
	return 0;
}
