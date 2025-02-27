#include <common.h>
#include <command.h>
#include <version.h>
#include <asm/types.h>
#include <asm/arch/clk.h>
#include <asm/arch/cpm.h>
#include <asm/arch/clk.h>
#include <asm/arch/base.h>
#include <asm/io.h>
#include <config.h>
#include "a1_vo.h"
#include "vde_hal.h"

typedef enum{
	IMP_FALSE = 0,
	IMP_TRUE = 1
}IMP_BOOL;

struct sync_info g_stSyncInfo[]={
	//======VIC==================bSyncMode===bIop===u8Intfb===freq===u16Vact===u16Vbp===u16Vfp===u16Hact===u16Hbp===u16Hfp===u16Hpw====u16Vpw===bIhs===bIvs
	{VO_OUTPUT_1080P24,               1,       0,       0,    24,     1080,     36,      4,     1920,     148,     638,      44,        5,      1,      1},
	{VO_OUTPUT_1080P25,               1,       0,       0,    25,     1080,     36,      4,     1920,     148,     528,      44,        5,      1,      1},
	{VO_OUTPUT_1080P30,               1,       0,       0,    30,     1080,     36,      4,     1920,     148,     88,       44,        5,      1,      1},
	{VO_OUTPUT_720P50,                1,       0,       0,    50,     720,      20,      5,     1280,     220,     440,      40,        5,      1,      1},
	{VO_OUTPUT_720P60,                1,       0,       0,    60,     720,      20,      5,     1280,     220,     110,      40,        5,      1,      1},
	{VO_OUTPUT_1080P50,               1,       0,       0,    50,     1080,     36,      4,     1920,     148,     528,      44,        5,      1,      1},
	{VO_OUTPUT_1080P60,               1,       0,       0,    60,     1080,     36,      4,     1920,     148,     88,       44,        5,      1,      1},
	{VO_OUTPUT_576P50,				  1,       0,       0,    50,     576,      39,      5,     720,      68,      12,       64,        5,      1,      1},
	{VO_OUTPUT_480P60,                1,       0,       0,    60,     480,      30,      9,     720,      60,      16,       62,        6,      0,      0},
	{VO_OUTPUT_640x480_60,            1,       0,       0,    60,     480,      33,      10,    640,      48,      16,       96,        2,      0,      0},
	{VO_OUTPUT_800x600_60,            1,       0,       0,    60,     600,      23,      1,     800,      88,      40,       128,       4,      1,      1},
	{VO_OUTPUT_1024x768_60,           1,       0,       0,    60,     768,      29,      3,     1024,     136,     24,       160,       6,      0,      0},
	{VO_OUTPUT_1280x1024_60,          1,       0,       0,    60,     1024,     38,      1,     1280,     248,     48,       112,       5,      1,      1},
	{VO_OUTPUT_1366x768_60,           1,       0,       0,    60,     768,      24,      3,     1366,     207,     70,       143,       3,      1,      1},
	{VO_OUTPUT_1440x900_60,           1,       0,       0,    60,     900,      17,      3,     1440,     80,      48,       32,        6,      1,      0},
	{VO_OUTPUT_1280x800_60,           1,       0,       0,    60,     800,      22,      3,     1280,     200,     72,       128,       6,      0,      1},
};

IMP_BOOL g_bVoStart = IMP_FALSE;
u32 g_u32VdeaClk = 700000000;
u32 g_u32SyncVic = VO_OUTPUT_720P60;
u32 g_u32Interface = 0;
struct sync_info stSyncInfo;
unsigned int tft_fmt = 0x00;

int start_vo(unsigned int type,unsigned int sync)
{
	struct sync_info stSyncInfo;
	u16 htotal = 0;
	u16 vtotal = 0;
	u32 pixel_clk = 0;
	u32 work_parent_clk = 0;
	u32 work_clk = 0;
	u8 clk_div = 0;
	char *s = NULL;

	if(sync >= VO_OUTPUT_BUTT){
		printf("invalid parameters!sync.");
		return -1;
	}

	if(type & VO_INTF_VGA && type & VO_INTF_TFT){
		printf("invalid parameters!interface.");
		return -1;
	}

	if(g_bVoStart == IMP_FALSE){
		clk_set_rate(VDEA, g_u32VdeaClk);
		memcpy(&stSyncInfo,&g_stSyncInfo[sync],sizeof(struct sync_info));
		htotal = stSyncInfo.u16Hact + stSyncInfo.u16Hfp + stSyncInfo.u16Hbp + stSyncInfo.u16Hpw;
		vtotal = stSyncInfo.u16Vact + stSyncInfo.u16Vfp + stSyncInfo.u16Vbp + stSyncInfo.u16Vpw;
		pixel_clk = htotal * vtotal * stSyncInfo.u16Freq;
		work_parent_clk = clk_get_rate((cpm_inl(CPM_VDEMCDR)>>30)+APLL);
		//printf("work_parent_clk=%d\n",work_parent_clk);
		//printf("pixel_clk=%d\n",pixel_clk);
		clk_div	= 1;
		while(clk_div++){
			if(work_parent_clk / clk_div < pixel_clk){
				clk_div--;
				break;
			}
		}
		//printf("clk_div=%d\n",clk_div);
		work_clk = work_parent_clk / clk_div;
		//printf("work_clk=%d\n",work_clk);
		clk_set_rate(VDEM, work_clk);
		if(work_parent_clk == clk_get_rate(VDEM)){
			printf("vde work clk's parent clk is too low!\n");
			return -1;
		}
		clk_set_rate(VDEV, pixel_clk);
		//printf("vde he version=0x%08x\n",vde_get_hw_version());
		vde_set_sync_info(&stSyncInfo);
		vde_enable_output(stSyncInfo.u16Hact,stSyncInfo.u16Vact);
		g_u32SyncVic = sync;
		if(type & VO_INTF_HDMI){
			vde_hdmi_enable(g_u32SyncVic);
		}
		if(type & VO_INTF_VGA){
			vde_vga_phy_enable();
		}
		if(type & VO_INTF_TFT){
			s = getenv("tft_fmt");
			if (s != NULL){
				tft_fmt = simple_strtol(s, NULL, 0);;
			}
			vde_tft_enable(tft_fmt);
		}
		if(type & VO_INTF_HDMI || type & VO_INTF_VGA){
			vde_displayclk_enable(g_u32SyncVic);
		}
		g_u32Interface = type;
		g_bVoStart = IMP_TRUE;
	}
	return 0;
}

int stop_vo(void)
{
	if(g_bVoStart == IMP_FALSE){
		return -1;
	}

	if(g_u32Interface & VO_INTF_HDMI || g_u32Interface & VO_INTF_VGA){
		vde_displayclk_disable();
	}

	if(g_u32Interface & VO_INTF_TFT){
		vde_tft_disable();
	}
	if(g_u32Interface & VO_INTF_VGA){
		vde_vga_phy_disable();
	}
	if(g_u32Interface & VO_INTF_HDMI){
		vde_hdmi_disable();
	}
	vde_disable_output();
	g_u32Interface = 0;
	g_u32SyncVic = VO_OUTPUT_720P60;
	g_bVoStart = IMP_FALSE;
	return 0;
}

int set_vo_background_color(unsigned int color)
{
	if(g_bVoStart == IMP_FALSE){
		return -1;
	}
	vde_set_background_color(color);
	return 0;
}

int start_videolayer(unsigned int layer,unsigned int yaddr,unsigned int uvaddr,unsigned int stride,
		unsigned int x,unsigned int y,unsigned int w,unsigned int h)
{
	struct sync_info *pstSyncInfo;
	if(g_bVoStart == IMP_FALSE){
		printf("startvo first\n");
		return -1;
	}

	if(layer != VO_LAYER_DEVICE_0 &&  layer != VO_LAYER_DEVICE_1){
		printf("Invalid parameter!layer.\n");
		return -1;
	}

	if(x % 2 || y % 2 || w % 2 || h % 2){
		printf("Invalid parameter!x,y,w,h must be align to 2.\n");
		return -1;
	}

	if(w < PIC_MIN_LENGTH || h < PIC_MIN_LENGTH){
		printf("Invalid parameter!w,h must be greater than %d.\n",PIC_MIN_LENGTH);
		return -1;
	}

	if(stride > PIC_MAX_WIDTH*2 || stride == 0){
		printf("Invalid parameter!stride.\n");
		return -1;
	}
	pstSyncInfo = &g_stSyncInfo[g_u32SyncVic];
	if(w > pstSyncInfo->u16Hact || h > pstSyncInfo->u16Vact
		||(x+w) > pstSyncInfo->u16Hact ||(y+h) > pstSyncInfo->u16Vact){
		printf("The rect exceeds the screen resolution\n");
		return -1;
	}

	if(layer ==  VO_LAYER_DEVICE_0){
		vde_enable_layer0(yaddr,uvaddr,stride,x,y,w,h);
	}else{
		vde_enable_layer1(yaddr,uvaddr,stride,x,y,w,h);
	}
	return 0;
}

int stop_videolayer(unsigned int layer)
{
	if(layer ==  VO_LAYER_DEVICE_0){
		vde_disable_layer0();
	}else{
		vde_disable_layer1();
	}
	return 0;
}

int start_graphicslayer(unsigned int addr,unsigned int stride,unsigned int pixfmt,unsigned int w,unsigned int h)
{
	struct sync_info *pstSyncInfo;

	if(g_bVoStart == IMP_FALSE){
		printf("startvo first\n");
		return -1;
	}
	pstSyncInfo = &g_stSyncInfo[g_u32SyncVic];

	if(w != pstSyncInfo->u16Hact || h != pstSyncInfo->u16Vact){
		printf("The rect of graphics  must be same with the screen resolution\n");
		return -1;
	}

	if(stride % 8){
		printf("Invalid parameter!stride must be align to 8.\n");
		return -1;
	}
	if(pixfmt > 23 || pixfmt < 0){
		printf("Invalid parameter!pixfmt error.\n");
		return -1;
	}

	vde_enable_layer2(addr,stride,pixfmt,w,h);
	return 0;
}

int stop_graphicslayer(void)
{
	vde_disable_layer2();
	return 0;
}

