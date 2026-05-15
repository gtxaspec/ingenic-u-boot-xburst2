#include <common.h>
#include <asm/io.h>
#include <asm/types.h>
#include <command.h>
#include <malloc.h>
#include "vde_hal.h"
#include "a1_vo.h"

#ifdef VDE_DEBUG
#define TR(fmt, args...)                \
	do {                                \
		printf("[VO]: " fmt, ##args);    \
	} while(0)
#else
	#define TR(fmt, args...)
#endif

#define ALIGNUP(a, b)	((a + (b - 1)) & ~(b - 1))
#define ALIGNDOWN(a, b)	(a & ~(b - 1))

static void __inline__ vdeWriteReg(u32 RegBase,u32 RegOffset,u32 RegData)
{
	u32 addr = (u32)RegBase + RegOffset;
	TR("=====>%s RegBase = 0x%08x RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__,(u32) RegBase, RegOffset, RegData);
	writel(RegData,(void *)addr);
	return;
}

static u32 __inline__ vdeReadReg(u32 RegBase,u32 RegOffset)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);
	TR("=====>%s: RegBase = 0x%08x RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, (u32)RegBase, RegOffset, data);
	return data;
}

static void __inline__ vdeSetBits(u32 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);
	data |= BitPos;
	TR("=====>%s  RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data);
	writel(data,(void *)addr);
	TR("=====>%s  RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data);
	return;
}

static void __inline__ vdeClearBits(u32 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);
	data &= (~BitPos);
	TR("=====>%%s RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data);
	writel(data,(void *)addr);
	TR("=====>%%s RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data);
	return;
}

static bool __inline__ vdeCheckBits(u32 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);
	data &= BitPos;
	if(data)  return true;
	else    return false;
}

static uint32_t rgb2yuv(uint8_t R,uint8_t G,uint8_t B)
{
	uint8_t Y;
	uint8_t U;
	uint8_t V;
	Y = ((R << 6) + (R << 3) + (R << 2) + R + (G << 7) + (G << 4) + (G << 2) + (G << 1) + (B << 4) + (B << 3) + (B << 2) + B) >> 8; //Y
	U = (-((R << 5) + (R << 3) + (R << 1)+ R) - ((G << 6) + (G << 4) + (G << 2)+G) + (B << 7) + 32768) >> 8; //U
	V = ((R << 7) - ((G << 6) + (G << 5) + (G << 3) + (G << 3) + G) - ((B << 4) + (B << 2) + B) + 32768 )>> 8; //V
	return (Y|(U<<8)|(V<<16));
}

u32 vde_get_hw_version(void)
{
	return vdeReadReg(VDE_BASE,TOP_VERSION);
}

void vde_set_sync_info(struct sync_info *sync)
{
	uint16_t htotal = 0;
	uint16_t vtotal = 0;
	uint16_t hde_st = 0;
	uint16_t hde_sp = 0;
	uint16_t vde_st = 0;
	uint16_t vde_sp = 0;
	uint16_t hsync_st = 0;
	uint16_t hsync_sp = 0;
	uint16_t vsync_st = 0;
	uint16_t vsync_sp = 0;
	uint32_t polarity = 0;

	vdeWriteReg(VDE_BASE,DSC_SIZE,sync->u16Vact << 16 | sync->u16Hact);
	htotal = sync->u16Hact + sync->u16Hfp + sync->u16Hbp + sync->u16Hpw;
	vtotal = sync->u16Vact + sync->u16Vfp + sync->u16Vbp + sync->u16Vpw;
	vdeWriteReg(VDE_BASE,DSC_TOTAL,vtotal << 16 | htotal);

	hde_st = sync->u16Hpw + sync->u16Hbp + 1;
	hde_sp = hde_st + sync->u16Hact;
	vdeWriteReg(VDE_BASE,DSC_HDE,hde_sp << 16 | hde_st);

	vde_st = sync->u16Vpw + sync->u16Vbp + 1;
	vde_sp = vde_st + sync->u16Vact;
	vdeWriteReg(VDE_BASE,DSC_VDE,vde_sp << 16 | vde_st);

	hsync_st = 1;
	hsync_sp = hsync_st + sync->u16Hpw;
	vdeWriteReg(VDE_BASE,DSC_HSYNC_VGA,hsync_sp << 16 | hsync_st);
	vdeWriteReg(VDE_BASE,DSC_HSYNC_HDMI,hsync_sp << 16 | hsync_st);
	vsync_st = 1;
	vsync_sp = vsync_st + sync->u16Vpw;
	vdeWriteReg(VDE_BASE,DSC_VSYNC_VGA,vsync_sp << 16 | vsync_st);
	vdeWriteReg(VDE_BASE,DSC_VSYNC_HDMI,vsync_sp << 16 | vsync_st);

	if(!sync->bIhs)
		polarity |= BIT(0)|BIT(8)|BIT(16);
	else
		polarity &= ~(BIT(0)|BIT(8)|BIT(16));

	if(!sync->bIvs)
		polarity |= BIT(4)|BIT(12)|BIT(20);
	else
		polarity &= ~(BIT(4)|BIT(12)|BIT(20));

	vdeWriteReg(VDE_BASE,DSC_POLARITY,polarity);
}

void vde_enable_output(u16 w,u16 h)
{
	u32 layer_type = LAYER_TYPE_V0;
	u16 clip_0_y[2] = {0,1023};
	u16 clip_0_c[2] = {0,1023};
	u16 clip_1_y[2] = {0,1023};
	u16 clip_1_c[2] = {0,1023};
	u16 clip_2_y[2] = {0,1023};
	u16 clip_2_c[2] = {0,1023};
	u16 offset_y[2] = {1024,1024};
	u16 offset_u[2] = {1024,1024};
	u16 offset_v[2] = {1024,1024};
	s32  matrix[9]   = {1024,0,0,0,1024,0,0,0,1024};
	u16 con_par[8]  = {1,0,1023,0,1023,1,1024,1};
	u16 sat_par[6]  = {1,1,33,1024,1024,0};
	u32 bg_color = 0;

	for(layer_type = LAYER_TYPE_V0; layer_type < LAYER_TYPE_MAX;layer_type++){
		vdeWriteReg(VDE_BASE,LAYER_DISPLAY_SIZE(layer_type),h<<16|w);
	}
	vdeWriteReg(VDE_BASE,BCSH_CLIP_Y0,((clip_0_y[0]&0x3fff)<<16)|((clip_0_y[1]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_CLIP_Y1,((clip_1_y[0]&0x3fff)<<16)|((clip_1_y[1]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_CLIP_Y2,((clip_2_y[0]&0x3fff)<<16)|((clip_2_y[1]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_CLIP_C0,((clip_0_c[0]&0x3fff)<<16)|((clip_0_c[1]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_CLIP_C1,((clip_1_c[0]&0x3fff)<<16)|((clip_1_c[1]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_CLIP_C2,((clip_2_c[0]&0x3fff)<<16)|((clip_2_c[1]&0x3fff)));

	vdeWriteReg(VDE_BASE,BCSH_OFFSET_Y,((offset_y[1]&0x3fff)<<16)|((offset_y[0]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_OFFSET_U,((offset_u[1]&0x3fff)<<16)|((offset_u[0]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_OFFSET_V,((offset_v[1]&0x3fff)<<16)|((offset_v[0]&0x3fff)));

	vdeWriteReg(VDE_BASE,BCSH_MATRIX_COEF0,((matrix[1]&0x3fff)<<16)|((matrix[0]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_MATRIX_COEF1,((matrix[3]&0x3fff)<<16)|((matrix[2]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_MATRIX_COEF2,((matrix[5]&0x3fff)<<16)|((matrix[4]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_MATRIX_COEF3,((matrix[7]&0x3fff)<<16)|((matrix[6]&0x3fff)));
	vdeWriteReg(VDE_BASE,BCSH_MATRIX_COEF4,((matrix[8]&0x3fff)));

	vdeWriteReg(VDE_BASE,BCSH_CON_ENABLE,((con_par[5]&0x7fff)<<16)|((con_par[0]&0x1) << 16));
	vdeWriteReg(VDE_BASE,BCSH_CON_S1,((con_par[7]&0x7fff)<<16)|((con_par[6]&0x7fff)));
	vdeWriteReg(VDE_BASE,BCSH_CON_X,((con_par[2]&0x3ff)<<16)|(con_par[1]&0x3ff));
	vdeWriteReg(VDE_BASE,BCSH_CON_Y,((con_par[4]&0x3ff)<<16)|(con_par[3]&0x3ff));

	vdeWriteReg(VDE_BASE,BCSH_SAT_ENABLE,((sat_par[5]&0x1fff)<<16)|(sat_par[0]&0x1));
	vdeWriteReg(VDE_BASE,BCSH_SAT_TH,((sat_par[2]&0x3ff)<<16)|((sat_par[1]&0x3ff)));
	vdeWriteReg(VDE_BASE,BCSH_SAT_STREN,((sat_par[4]&0xff)<<16)|((sat_par[3]&0xff)));

	vdeWriteReg(VDE_BASE,MIXER_ENABLE,BIT(12)|BIT(14)|BIT(16)|BIT(26)|BIT(28));
	vdeSetBits(VDE_BASE,TOP_MODULE_EN,BIT(0));

	vdeWriteReg(VDE_BASE,TOP_WRITE_SEL0,0xffffffff);
	vdeWriteReg(VDE_BASE,TOP_WRITE_SEL1,0xffffffff);
	vdeWriteReg(VDE_BASE,TOP_READ_SEL0,0xffffffff);
	vdeWriteReg(VDE_BASE,TOP_READ_SEL1,0xffffffff);
	bg_color = rgb2yuv(0x00,0x00,0x00);
	vdeWriteReg(VDE_BASE,V0_BACKGROUND_COLOR,bg_color);
	vdeWriteReg(VDE_BASE,V1_BACKGROUND_COLOR,bg_color);
	writel(0x0,(void *)0xb301206c);
	vdeSetBits(VDE_BASE,TOP_SHADOW_SET0,BIT(0));
	writel(0x88404002,(void *)0xb3012020);
	writel(0x88404002,(void *)0xb3012024);
	writel(0x8840400e,(void *)0xb3012028);
	writel(0x88404002,(void *)0xb301202c);
	writel(0x88404002,(void *)0xb3012030);
	writel(0x88404002,(void *)0xb3012034);
	writel(0x88404002,(void *)0xb3012038);
	writel(0x88404002,(void *)0xb301203c);
	writel(0x1,(void *)0xb301206c);
}

void vde_disable_output(void)
{
	vdeClearBits(VDE_BASE,TOP_MODULE_EN,BIT(0));
	vdeSetBits(VDE_BASE,TOP_SHADOW_SET0,BIT(0));
	vdeSetBits(VDE_BASE,TOP_RST,BIT(0));
}

void vde_set_background_color(u32 color)
{
	u32 bg_color = rgb2yuv((color&0x00ff0000)>>16,(color&0x0000ff00)>>8,color&0x000000ff);
	vdeWriteReg(VDE_BASE,V0_BACKGROUND_COLOR,bg_color);
	vdeWriteReg(VDE_BASE,V1_BACKGROUND_COLOR,bg_color);
	vdeSetBits(VDE_BASE,TOP_SHADOW_SET0,BIT(0));
}

void vde_vga_phy_enable(void)
{
	gpio_set_func(GPIO_PORT_C, GPIO_FUNC_0, 0x3C);
	vdeWriteReg(VDAC_BASE,0x16*4, 0x0);
	vdeWriteReg(VDAC_BASE,0x17*4, 0x0);
	vdeWriteReg(VDAC_BASE,0x1b*4, 0x0);
	vdeWriteReg(VDAC_BASE,0x1c*4, 0x0);
	vdeWriteReg(VDAC_BASE,0x20*4, 0x0);
	vdeWriteReg(VDAC_BASE,0x21*4, 0x0);

	vdeWriteReg(VDAC_BASE,0x00*4, 0x00);
	vdeWriteReg(VDAC_BASE,0x00*4, 0x1f);

	vdeWriteReg(VDAC_BASE,0x08*4, 0x64);
	vdeWriteReg(VDAC_BASE,0xae*4, 0x04);

	vdeWriteReg(VDAC_BASE,0xb3*4, 0x08);
	vdeWriteReg(VDAC_BASE,0xb0*4, 0x80);

	vdeWriteReg(VDAC_BASE,0xa2*4, 0x06);
	vdeWriteReg(VDAC_BASE,0xa5*4, 0x06);
	vdeWriteReg(VDAC_BASE,0xa8*4, 0x06);

	vdeWriteReg(VDAC_BASE,0xb1*4, 0x24);
	vdeWriteReg(VDAC_BASE,0xb2*4, 0x0);
	vdeWriteReg(VDAC_BASE,0xb6*4, 0x0);
	vdeWriteReg(VDAC_BASE,0xb8*4, 0x0);
	vdeWriteReg(VDAC_BASE,0xba*4, 0x0);
	vdeWriteReg(VDE_BASE,DSC_VGA_RGB_ORDER,0x102);//vga rgb order
	vdeSetBits(VDE_BASE,DSC_ENABLE,BIT(0));
}

void vde_vga_phy_disable(void)
{
	vdeClearBits(VDAC_BASE,0xa0*4,BIT(3));
	vdeClearBits(VDAC_BASE,0xa2*4,BIT(2));
	vdeClearBits(VDAC_BASE,0xa5*4,BIT(2));
	vdeClearBits(VDAC_BASE,0xa8*4,BIT(2));
}

void vde_tft_enable(int tft_fmt)
{
	vdeWriteReg(VDE_BASE,DSC_TFT_FORMAT,tft_fmt);
	vdeSetBits(VDE_BASE,DSC_ENABLE,BIT(8));
}

void vde_tft_disable(void)
{
	vdeClearBits(VDE_BASE,DSC_ENABLE,BIT(16));
}

void vde_enable_layer0(u32 yaddr,u32 uvaddr,u32 stride,u32 x,u32 y,u32 w,u32 h)
{
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_Y_ADDR(LAYER_TYPE_V0,0),(yaddr&(~0x80000000)));
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_Y_STRIDE(LAYER_TYPE_V0,0),stride);
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_UV_ADDR(LAYER_TYPE_V0,0),(uvaddr&(~0x80000000)));
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_UV_STRIDE(LAYER_TYPE_V0,0),stride);
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_START(LAYER_TYPE_V0,0),(y << 16)| x);
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_SIZE(LAYER_TYPE_V0,0),(ALIGNDOWN(h,16) << 16)| w);
	vdeWriteReg(VDE_BASE,TOP_V0_AREA_ST,(w << 16)| 0);
	vdeWriteReg(VDE_BASE,TOP_V0_AREA_END,(h << 16)| 0);
	vdeWriteReg(VDE_BASE,TOP_V0_FMT,64 << 16 | 0x20);
	vdeSetBits(VDE_BASE,TOP_LAYER_EN,BIT(0));
	vdeSetBits(VDE_BASE,TOP_SHADOW_SET0,BIT(0));
}

void vde_disable_layer0(void)
{
	vdeClearBits(VDE_BASE,TOP_LAYER_EN,BIT(0));
	vdeSetBits(VDE_BASE,TOP_SHADOW_SET0,BIT(0));
}

void vde_enable_layer1(u32 yaddr,u32 uvaddr,u32 stride,u32 x,u32 y,u32 w,u32 h)
{
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_Y_ADDR(LAYER_TYPE_V1,0),(yaddr&(~0x80000000)));
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_Y_STRIDE(LAYER_TYPE_V1,0),stride);
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_UV_ADDR(LAYER_TYPE_V1,0),(uvaddr&(~0x80000000)));
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_UV_STRIDE(LAYER_TYPE_V1,0),stride);
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_START(LAYER_TYPE_V1,0),(y << 16)| x);
	vdeWriteReg(VDE_BASE,TOP_V_WINDOW_SIZE(LAYER_TYPE_V1,0),(ALIGNDOWN(h,16) << 16)| w);
	vdeWriteReg(VDE_BASE,TOP_V1_FMT,64 << 16 | 0x20);
	vdeSetBits(VDE_BASE,TOP_LAYER_EN,BIT(4));
	vdeSetBits(VDE_BASE,TOP_SHADOW_SET0,BIT(0));
}

void vde_disable_layer1(void)
{
	vdeClearBits(VDE_BASE,TOP_LAYER_EN,BIT(4));
	vdeSetBits(VDE_BASE,TOP_SHADOW_SET0,BIT(0));
}

void vde_enable_layer2(u32 addr,u32 stride,u32 pixfmt,u32 w,u32 h)
{
	vdeWriteReg(VDE_BASE,TOP_G_FMT,pixfmt);
	vdeWriteReg(VDE_BASE,TOP_GWIN_PAR7,(addr&(~0x80000000)));
	vdeWriteReg(VDE_BASE,TOP_GWIN_PAR8,stride);

	vdeWriteReg(VDE_BASE,G_PARA0,0x00000000);
	vdeWriteReg(VDE_BASE,G_PARA1,0x00000000);
	vdeWriteReg(VDE_BASE,G_PARA2,0x00ff00ff);
	vdeWriteReg(VDE_BASE,G_PARA3,0x000000ff);
	vdeWriteReg(VDE_BASE,G_PARA4,0x100000ff);

	vdeSetBits(VDE_BASE,TOP_LAYER_EN,BIT(8));
	vdeSetBits(VDE_BASE,TOP_SHADOW_SET0,BIT(0));
}

void vde_disable_layer2(void)
{
	vdeClearBits(VDE_BASE,TOP_LAYER_EN,BIT(8));
	vdeSetBits(VDE_BASE,TOP_SHADOW_SET0,BIT(0));
}

void vde_displayclk_enable(VO_INTF_SYNC_E sync)
{
	vdeWriteReg(HDMI_BASE,0x10014,0x01);
	vdeWriteReg(HDMI_PHY_BASE,0x2c0,0x0E);
	vdeWriteReg(HDMI_PHY_BASE,0x330,0x0F);
	vdeWriteReg(HDMI_PHY_BASE,0x280,0x00);
	vdeWriteReg(HDMI_PHY_BASE,0x2A8,0x0E);
	switch(sync){
		case VO_OUTPUT_640x480_60:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x21);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x1A);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x41);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x28);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x03);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_480P60:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x24);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x1A);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x41);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x28);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x03);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_576P50:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x24);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x1A);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x41);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x28);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x0B);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_800x600_60:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x35);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x1A);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x41);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x14);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x09);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_1024x768_60:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x2B);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x15);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x14);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x09);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_720P50:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x63);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x1A);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x41);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x14);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x09);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_720P60:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x63);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x1A);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x41);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x14);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x09);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_1366x768_60:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x39);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x15);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x0A);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x08);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_1280x1024_60:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x24);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x10);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x42);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x0A);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x08);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_1280x800_60:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x37);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x15);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x0A);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x08);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_1440x900_60:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x3B);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x15);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x0A);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x08);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_1080P24:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x63);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x1A);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x41);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x14);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x09);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_1080P25:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x63);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x1A);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x41);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x14);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x09);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_1080P30:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x63);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x1A);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x41);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x14);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x09);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_1080P50:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x63);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x15);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x0A);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x08);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		case VO_OUTPUT_1080P60:
			vdeWriteReg(HDMI_PHY_BASE,0x284,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x288,0xF0);
			vdeWriteReg(HDMI_PHY_BASE,0x28c,0x63);
			vdeWriteReg(HDMI_PHY_BASE,0x290,0x15);
			vdeWriteReg(HDMI_PHY_BASE,0x294,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x298,0x64);
			vdeWriteReg(HDMI_PHY_BASE,0x2ac,0x01);
			vdeWriteReg(HDMI_PHY_BASE,0x2b0,0x0A);
			vdeWriteReg(HDMI_PHY_BASE,0x2b4,0x08);
			vdeWriteReg(HDMI_PHY_BASE,0x344,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x348,0x00);
			vdeWriteReg(HDMI_PHY_BASE,0x34c,0x00);
			break;
		default:
			break;
	}
	//turn on the phy LDO
	vdeWriteReg(HDMI_PHY_BASE,0x2d0,0x07);
	vdeWriteReg(HDMI_PHY_BASE,0x2f8,0x70);
	vdeWriteReg(HDMI_PHY_BASE,0x2c8,0x0F);
	//set up phy
	vdeClearBits(HDMI_PHY_BASE,0x008,BIT(0));
	mdelay(1);
	vdeSetBits(HDMI_PHY_BASE,0x008,BIT(0));
	//use phy clk
	vdeClearBits(CPM_BASE,0xb0,BIT(11));
}

void vde_hdmi_enable(VO_INTF_SYNC_E sync)
{
	vdeSetBits(VDE_BASE,0x30000,BIT(4));
	switch(sync){
		case VO_OUTPUT_640x480_60:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x18);
			vdeWriteReg(HDMI_BASE,0x4004, 0x80);
			vdeWriteReg(HDMI_BASE,0x4008, 0x02);
			vdeWriteReg(HDMI_BASE,0x400C, 0xA0);
			vdeWriteReg(HDMI_BASE,0x4010, 0x00);
			vdeWriteReg(HDMI_BASE,0x4014, 0xE0);
			vdeWriteReg(HDMI_BASE,0x4018, 0x01);
			vdeWriteReg(HDMI_BASE,0x4020, 0x10);
			vdeWriteReg(HDMI_BASE,0x401C, 0x2D);
			vdeWriteReg(HDMI_BASE,0x4028, 0x60);
			vdeWriteReg(HDMI_BASE,0x4030, 0x0A);
			vdeWriteReg(HDMI_BASE,0x4034, 0x02);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_480P60:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x18);
			vdeWriteReg(HDMI_BASE,0x4004, 0xD0);
			vdeWriteReg(HDMI_BASE,0x4008, 0x02);
			vdeWriteReg(HDMI_BASE,0x400C, 0x8A);
			vdeWriteReg(HDMI_BASE,0x4010, 0x00);
			vdeWriteReg(HDMI_BASE,0x4014, 0xE0);
			vdeWriteReg(HDMI_BASE,0x4018, 0x01);
			vdeWriteReg(HDMI_BASE,0x4020, 0x10);
			vdeWriteReg(HDMI_BASE,0x401C, 0x2D);
			vdeWriteReg(HDMI_BASE,0x4028, 0x3E);
			vdeWriteReg(HDMI_BASE,0x4030, 0x09);
			vdeWriteReg(HDMI_BASE,0x4034, 0x06);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_576P50:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0xD0);
			vdeWriteReg(HDMI_BASE,0x4008, 0x02);
			vdeWriteReg(HDMI_BASE,0x400C, 0x90);
			vdeWriteReg(HDMI_BASE,0x4010, 0x00);
			vdeWriteReg(HDMI_BASE,0x4014, 0x40);
			vdeWriteReg(HDMI_BASE,0x4018, 0x02);
			vdeWriteReg(HDMI_BASE,0x4020, 0x0C);
			vdeWriteReg(HDMI_BASE,0x401C, 0x31);
			vdeWriteReg(HDMI_BASE,0x4028, 0x40);
			vdeWriteReg(HDMI_BASE,0x4030, 0x05);
			vdeWriteReg(HDMI_BASE,0x4034, 0x05);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_800x600_60:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0x20);
			vdeWriteReg(HDMI_BASE,0x4008, 0x03);
			vdeWriteReg(HDMI_BASE,0x400C, 0x00);
			vdeWriteReg(HDMI_BASE,0x4010, 0x01);
			vdeWriteReg(HDMI_BASE,0x4014, 0x58);
			vdeWriteReg(HDMI_BASE,0x4018, 0x02);
			vdeWriteReg(HDMI_BASE,0x4020, 0x28);
			vdeWriteReg(HDMI_BASE,0x401C, 0x1C);
			vdeWriteReg(HDMI_BASE,0x4028, 0x80);
			vdeWriteReg(HDMI_BASE,0x4030, 0x01);
			vdeWriteReg(HDMI_BASE,0x4034, 0x04);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_1024x768_60:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x18);
			vdeWriteReg(HDMI_BASE,0x4004, 0x00);
			vdeWriteReg(HDMI_BASE,0x4008, 0x04);
			vdeWriteReg(HDMI_BASE,0x400C, 0x40);
			vdeWriteReg(HDMI_BASE,0x4010, 0x01);
			vdeWriteReg(HDMI_BASE,0x4014, 0x00);
			vdeWriteReg(HDMI_BASE,0x4018, 0x03);
			vdeWriteReg(HDMI_BASE,0x4020, 0x18);
			vdeWriteReg(HDMI_BASE,0x401C, 0x26);
			vdeWriteReg(HDMI_BASE,0x4028, 0xA0);
			vdeWriteReg(HDMI_BASE,0x4030, 0x03);
			vdeWriteReg(HDMI_BASE,0x4034, 0x06);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_720P50:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0x00);
			vdeWriteReg(HDMI_BASE,0x4008, 0x05);
			vdeWriteReg(HDMI_BASE,0x400C, 0xBC);
			vdeWriteReg(HDMI_BASE,0x4010, 0x02);
			vdeWriteReg(HDMI_BASE,0x4014, 0xD0);
			vdeWriteReg(HDMI_BASE,0x4018, 0x02);
			vdeWriteReg(HDMI_BASE,0x4020, 0xB8);
			vdeWriteReg(HDMI_BASE,0x401C, 0x1E);
			vdeWriteReg(HDMI_BASE,0x4028, 0x28);
			vdeWriteReg(HDMI_BASE,0x4030, 0x05);
			vdeWriteReg(HDMI_BASE,0x4034, 0x05);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_720P60:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0x00);
			vdeWriteReg(HDMI_BASE,0x4008, 0x05);
			vdeWriteReg(HDMI_BASE,0x400C, 0x72);
			vdeWriteReg(HDMI_BASE,0x4010, 0x01);
			vdeWriteReg(HDMI_BASE,0x4014, 0xD0);
			vdeWriteReg(HDMI_BASE,0x4018, 0x02);
			vdeWriteReg(HDMI_BASE,0x4020, 0x6E);
			vdeWriteReg(HDMI_BASE,0x401C, 0x1E);
			vdeWriteReg(HDMI_BASE,0x4028, 0x28);
			vdeWriteReg(HDMI_BASE,0x4030, 0x05);
			vdeWriteReg(HDMI_BASE,0x4034, 0x05);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_1366x768_60:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0x56);
			vdeWriteReg(HDMI_BASE,0x4008, 0x05);
			vdeWriteReg(HDMI_BASE,0x400C, 0xA4);
			vdeWriteReg(HDMI_BASE,0x4010, 0x01);
			vdeWriteReg(HDMI_BASE,0x4014, 0x00);
			vdeWriteReg(HDMI_BASE,0x4018, 0x03);
			vdeWriteReg(HDMI_BASE,0x4020, 0x46);
			vdeWriteReg(HDMI_BASE,0x401C, 0x1E);
			vdeWriteReg(HDMI_BASE,0x4028, 0x8F);
			vdeWriteReg(HDMI_BASE,0x4030, 0x03);
			vdeWriteReg(HDMI_BASE,0x4034, 0x03);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_1280x1024_60:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0x00);
			vdeWriteReg(HDMI_BASE,0x4008, 0x05);
			vdeWriteReg(HDMI_BASE,0x400C, 0x98);
			vdeWriteReg(HDMI_BASE,0x4010, 0x01);
			vdeWriteReg(HDMI_BASE,0x4014, 0x00);
			vdeWriteReg(HDMI_BASE,0x4018, 0x04);
			vdeWriteReg(HDMI_BASE,0x4020, 0x30);
			vdeWriteReg(HDMI_BASE,0x401C, 0x2C);
			vdeWriteReg(HDMI_BASE,0x4028, 0x70);
			vdeWriteReg(HDMI_BASE,0x4030, 0x01);
			vdeWriteReg(HDMI_BASE,0x4034, 0x05);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_1280x800_60:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x58);
			vdeWriteReg(HDMI_BASE,0x4004, 0x00);
			vdeWriteReg(HDMI_BASE,0x4008, 0x05);
			vdeWriteReg(HDMI_BASE,0x400C, 0x90);
			vdeWriteReg(HDMI_BASE,0x4010, 0x01);
			vdeWriteReg(HDMI_BASE,0x4014, 0x20);
			vdeWriteReg(HDMI_BASE,0x4018, 0x03);
			vdeWriteReg(HDMI_BASE,0x4020, 0x48);
			vdeWriteReg(HDMI_BASE,0x401C, 0x1F);
			vdeWriteReg(HDMI_BASE,0x4028, 0x80);
			vdeWriteReg(HDMI_BASE,0x4030, 0x03);
			vdeWriteReg(HDMI_BASE,0x4034, 0x06);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_1440x900_60:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x38);
			vdeWriteReg(HDMI_BASE,0x4004, 0xA0);
			vdeWriteReg(HDMI_BASE,0x4008, 0x05);
			vdeWriteReg(HDMI_BASE,0x400C, 0xA0);
			vdeWriteReg(HDMI_BASE,0x4010, 0x00);
			vdeWriteReg(HDMI_BASE,0x4014, 0x84);
			vdeWriteReg(HDMI_BASE,0x4018, 0x03);
			vdeWriteReg(HDMI_BASE,0x4020, 0x30);
			vdeWriteReg(HDMI_BASE,0x401C, 0x1A);
			vdeWriteReg(HDMI_BASE,0x4028, 0x20);
			vdeWriteReg(HDMI_BASE,0x4030, 0x03);
			vdeWriteReg(HDMI_BASE,0x4034, 0x06);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_1080P24:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0x80);
			vdeWriteReg(HDMI_BASE,0x4008, 0x07);
			vdeWriteReg(HDMI_BASE,0x400C, 0x3E);
			vdeWriteReg(HDMI_BASE,0x4010, 0x03);
			vdeWriteReg(HDMI_BASE,0x4014, 0x38);
			vdeWriteReg(HDMI_BASE,0x4018, 0x04);
			vdeWriteReg(HDMI_BASE,0x4020, 0x7E);
			vdeWriteReg(HDMI_BASE,0x401C, 0x2D);
			vdeWriteReg(HDMI_BASE,0x4028, 0x2C);
			vdeWriteReg(HDMI_BASE,0x4030, 0x04);
			vdeWriteReg(HDMI_BASE,0x4034, 0x05);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_1080P25:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0x80);
			vdeWriteReg(HDMI_BASE,0x4008, 0x07);
			vdeWriteReg(HDMI_BASE,0x400C, 0xD0);
			vdeWriteReg(HDMI_BASE,0x4010, 0x02);
			vdeWriteReg(HDMI_BASE,0x4014, 0x38);
			vdeWriteReg(HDMI_BASE,0x4018, 0x04);
			vdeWriteReg(HDMI_BASE,0x4020, 0x10);
			vdeWriteReg(HDMI_BASE,0x401C, 0x2D);
			vdeWriteReg(HDMI_BASE,0x4028, 0x2C);
			vdeWriteReg(HDMI_BASE,0x4030, 0x04);
			vdeWriteReg(HDMI_BASE,0x4034, 0x05);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_1080P30:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0x80);
			vdeWriteReg(HDMI_BASE,0x4008, 0x07);
			vdeWriteReg(HDMI_BASE,0x400C, 0x18);
			vdeWriteReg(HDMI_BASE,0x4010, 0x01);
			vdeWriteReg(HDMI_BASE,0x4014, 0x38);
			vdeWriteReg(HDMI_BASE,0x4018, 0x04);
			vdeWriteReg(HDMI_BASE,0x4020, 0x58);
			vdeWriteReg(HDMI_BASE,0x401C, 0x2D);
			vdeWriteReg(HDMI_BASE,0x4028, 0x2C);
			vdeWriteReg(HDMI_BASE,0x4030, 0x04);
			vdeWriteReg(HDMI_BASE,0x4034, 0x05);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_1080P50:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0x80);
			vdeWriteReg(HDMI_BASE,0x4008, 0x07);
			vdeWriteReg(HDMI_BASE,0x400C, 0xD0);
			vdeWriteReg(HDMI_BASE,0x4010, 0x02);
			vdeWriteReg(HDMI_BASE,0x4014, 0x38);
			vdeWriteReg(HDMI_BASE,0x4018, 0x04);
			vdeWriteReg(HDMI_BASE,0x4020, 0x10);
			vdeWriteReg(HDMI_BASE,0x401C, 0x2D);
			vdeWriteReg(HDMI_BASE,0x4028, 0x2C);
			vdeWriteReg(HDMI_BASE,0x4030, 0x04);
			vdeWriteReg(HDMI_BASE,0x4034, 0x05);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		case VO_OUTPUT_1080P60:
			vdeWriteReg(HDMI_BASE,0x0800, 0x01);
			vdeWriteReg(HDMI_BASE,0x4000, 0x78);
			vdeWriteReg(HDMI_BASE,0x4004, 0x80);
			vdeWriteReg(HDMI_BASE,0x4008, 0x07);
			vdeWriteReg(HDMI_BASE,0x400C, 0x18);
			vdeWriteReg(HDMI_BASE,0x4010, 0x01);
			vdeWriteReg(HDMI_BASE,0x4014, 0x38);
			vdeWriteReg(HDMI_BASE,0x4018, 0x04);
			vdeWriteReg(HDMI_BASE,0x4020, 0x58);
			vdeWriteReg(HDMI_BASE,0x401C, 0x2D);
			vdeWriteReg(HDMI_BASE,0x4028, 0x2C);
			vdeWriteReg(HDMI_BASE,0x4030, 0x04);
			vdeWriteReg(HDMI_BASE,0x4034, 0x05);
			vdeWriteReg(HDMI_BASE,0x4044, 0x0C);
			vdeWriteReg(HDMI_BASE,0x4048, 0x20);
			vdeWriteReg(HDMI_BASE,0x404C, 0x01);
			vdeWriteReg(HDMI_BASE,0x438C, 0x1F);
			break;
		default:
			break;
	}
}

void vde_displayclk_disable(void)
{
	vdeSetBits(CPM_BASE,0xb0,BIT(11));
	vdeWriteReg(HDMI_BASE,0x10014,0x00);
}

void vde_hdmi_disable(void)
{
	vdeClearBits(VDE_BASE,0x30000,BIT(4));
}

