#ifndef __VO_HAL_H__
#define __VO_HAL_H__

#include "vde_reg.h"
#include "a1_vo.h"

struct sync_info{
	u8  u8Vic;
	u8	bSyncMode;
	u8	bIop;
	u8   u8Intfb;
	u16  u16Freq;
	u16  u16Vact;  /* vertical active area */
	u16  u16Vbp;    /* vertical back blank porch */
	u16  u16Vfp;    /* vertical front blank porch */

	u16  u16Hact;   /* herizontal active area */
	u16  u16Hbp;    /* herizontal back blank porch */
	u16  u16Hfp;    /* herizontal front blank porch */

	u16  u16Hpw;    /* horizontal pulse width */
	u16  u16Vpw;    /* vertical pulse width */

	u8  bIhs;      /* inverse horizontal synch signal */
	u8  bIvs;      /* inverse vertical syncv signal */
};

enum layer_type{
	LAYER_TYPE_V0,
	LAYER_TYPE_V1,
	LAYER_TYPE_G ,
	LAYER_TYPE_M ,
	LAYER_TYPE_MAX
};

u32 vde_get_hw_version(void);
void vde_set_sync_info(struct sync_info *sync);
void vde_enable_output(u16 w,u16 h);
void vde_disable_output(void);
void vde_set_background_color(u32 color);
void vde_vga_phy_enable(void);
void vde_vga_phy_disable(void);
void vde_tft_enable(int tft_fmt);
void vde_tft_disable(void);
void vde_enable_layer0(u32 yaddr,u32 uvaddr,u32 stride,u32 x,u32 y,u32 w,u32 h);
void vde_disable_layer0(void);
void vde_enable_layer1(u32 yaddr,u32 uvaddr,u32 stride,u32 x,u32 y,u32 w,u32 h);
void vde_disable_layer1(void);
void vde_enable_layer2(u32 addr,u32 stride,u32 pixfmt,u32 w,u32 h);
void vde_disable_layer2(void);
void vde_displayclk_enable(VO_INTF_SYNC_E sync);
void vde_hdmi_enable(VO_INTF_SYNC_E sync);
void vde_hdmi_disable(void);
void vde_displayclk_disable(void);


#endif
