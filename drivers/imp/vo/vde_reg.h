#ifndef __VO_REG_H__
#define __VO_REG_H__

#define BIT(nr)							(1UL << (nr))
#define DMA_OFFSET						0x10000
#define MIXER_OFFSET					0x20000
#define DSC_OFFSET						0x30000
#define INTP_OFFSET						0x60000
#define HDMI_OFFSET						0x80000

#define TOP_VERSION						0x0000
#define TOP_SHADOW_SET0					0x0010
#define TOP_SHADOW_SET1					0x0014
#define TOP_MODULE_EN					0x0040
#define TOP_RST							0x0030
#define TOP_LAYER_EN					0x0044

#define LAYER_DISPLAY_SIZE(ID)			(0x0050 + (ID)*4)

//V0layer
#define TOP_V0_AREA_ST					0x0084
#define TOP_V0_AREA_END					0x0088
//V0 v1 yuv color format
#define TOP_V0_FMT						0x0080
#define TOP_V1_FMT						0x0580
#define TOP_M_FMT						0x05e0
#define TOP_G_FMT						0x0624

//v0/v1 window
#define TOP_V0_WINDOW_START(W)			(0x00f0 + (W)*0x20)
#define TOP_V0_WINDOW_SIZE(W)			(0x00f4 + (W)*0x20)
#define TOP_V0_WINDOW_Y_ADDR(W)			(0x00f8 + (W)*0x20)
#define TOP_V0_WINDOW_Y_STRIDE(W)		(0x00fc + (W)*0x20)
#define TOP_V0_WINDOW_UV_ADDR(W)		(0x0100 + (W)*0x20)
#define TOP_V0_WINDOW_UV_STRIDE(W)		(0x0104 + (W)*0x20)

#define TOP_V1_WINDOW_START(W)			(0x0594 + (W)*0x20)
#define TOP_V1_WINDOW_SIZE(W)			(0x0590 + (W)*0x20)
#define TOP_V1_WINDOW_Y_ADDR(W)			(0x0598 + (W)*0x20)
#define TOP_V1_WINDOW_Y_STRIDE(W)		(0x059c + (W)*0x20)
#define TOP_V1_WINDOW_UV_ADDR(W)		(0x05a0 + (W)*0x20)
#define TOP_V1_WINDOW_UV_STRIDE(W)		(0x05a4	+ (W)*0x20)

#define VDE_LAYER_V0 0
#define TOP_V_WINDOW_START(L,W)			((L) == VDE_LAYER_V0 ? TOP_V0_WINDOW_START((W)):TOP_V1_WINDOW_START((W)))
#define TOP_V_WINDOW_SIZE(L,W)			((L) == VDE_LAYER_V0 ? TOP_V0_WINDOW_SIZE((W)):TOP_V1_WINDOW_SIZE((W)))
#define TOP_V_WINDOW_Y_ADDR(L,W)		((L) == VDE_LAYER_V0 ? TOP_V0_WINDOW_Y_ADDR((W)):TOP_V1_WINDOW_Y_ADDR((W)))
#define TOP_V_WINDOW_Y_STRIDE(L,W)  	((L) == VDE_LAYER_V0 ? TOP_V0_WINDOW_Y_STRIDE((W)):TOP_V1_WINDOW_Y_STRIDE((W)))
#define TOP_V_WINDOW_UV_ADDR(L,W)		((L) == VDE_LAYER_V0 ? TOP_V0_WINDOW_UV_ADDR((W)):TOP_V1_WINDOW_UV_ADDR((W)))
#define TOP_V_WINDOW_UV_STRIDE(L,W) 	((L) == VDE_LAYER_V0 ? TOP_V0_WINDOW_UV_STRIDE((W)):TOP_V1_WINDOW_UV_STRIDE((W)))

//M layer
#define TOP_MWIN_PAR					0x05e0
#define	TOP_MWIN_PAR0					0x05e4
#define	TOP_MWIN_PAR1					0x05e8
#define	TOP_MWIN_PAR2					0x05ec
#define	TOP_MWIN_PAR3					0x05f0
//G layer
#define TOP_GWIN_PAR1					0x0624

#define GWIN_COMPRESS_ADDR(CHN)			(0x062c + (CHN) * 4)
#define GWIN_COMPRESS_BURST(CHN) 		(0x0654 + (CHN) * 4)
#define GWIN_CHECK_POINT(INDEX)  		(0x0664 + (INDEX) * 4)

#define TOP_GWIN_PAR7					0x063c
#define TOP_GWIN_PAR8					0x0640

#define TOP_WRITE_SEL0					0x7d0
#define TOP_WRITE_SEL1					0x7d4
#define TOP_READ_SEL0					0x7e0
#define TOP_READ_SEL1					0x7e4

#define DMA_STOP_ON						(DMA_OFFSET + 0x0000)
#define DMA_STOP_MODE					(DMA_OFFSET + 0x0004)
#define DMA_STOP_RSV					(DMA_OFFSET + 0x0008)
#define DMA_STOP_STATE					(DMA_OFFSET + 0x000c)
#define DMA_DESC_ADDR					(DMA_OFFSET + 0x0080)
#define DMA_DESC_START					(DMA_OFFSET + 0x0084)
#define DMA_DESC_DMA_NOW_BLK			(DMA_OFFSET + 0x0088)
#define DMA_DESC_DMA_NXT_BLK			(DMA_OFFSET + 0x008c)
#define DMA_DESC_WRITE_NXT_BLK			(DMA_OFFSET + 0x0090)
#define DMA_DESC_WRITE_INFO				(DMA_OFFSET + 0x0094)
#define DMA_AREAD_STA					(DMA_OFFSET + 0x0100)
#define DMA_AREAD_PRIO					(DMA_OFFSET + 0x0104)
#define DMA_AREAD_FAST_CMD				(DMA_OFFSET + 0x0108)
#define DMA_AWRITE_STA					(DMA_OFFSET + 0x0120)
#define DMA_AWRITE_PRIO					(DMA_OFFSET + 0x0124)
#define DMA_DMAO_PRIO					(DMA_OFFSET + 0x0140)
#define DMA_M_RAM_START					(DMA_OFFSET + 0x1000)

#define INTP_RD_CLR_COM4					BIT(4)
#define INTP_RD_CLR_COM3            		BIT(3)
#define INTP_RD_CLR_COM2            		BIT(2)
#define INTP_RD_CLR_COM1            		BIT(1)
#define INTP_RD_CLR_COM0            		BIT(0)
#define INTP_CTRL							(INTP_OFFSET + 0x0000)

#define INTP_FIRST_PIX_ERR					BIT(10)
#define INTP_DLI_STAT_DONE					BIT(5)
#define INTP_START_VDE_MOD					BIT(4)
#define INTP_FRM_START_HDMI					BIT(3)
#define INTP_FRM_START_VGA_TFT				BIT(2)
#define INTP_FRM_DONE_DSC					BIT(1)
#define INTP_FRM_DONE_MIXER					BIT(0)
#define INTP_COMMON_0_EN					(INTP_OFFSET + 0x0020)
#define INTP_COMMON_0_CLR					(INTP_OFFSET + 0x0024)
#define INTP_COMMON_0_INFO					(INTP_OFFSET + 0x0028)


#define	INTP_G_UNPACK_ERR11                 BIT(15)
#define	INTP_G_UNPACK_ERR10					BIT(14)
#define	INTP_G_UNPACK_ERR9					BIT(13)
#define	INTP_G_UNPACK_ERR8					BIT(12)
#define	INTP_G_UNPACK_ERR7					BIT(11)
#define	INTP_G_UNPACK_ERR6					BIT(10)
#define	INTP_G_UNPACK_ERR5					BIT(9)
#define	INTP_G_UNPACK_ERR4					BIT(8)
#define	INTP_G_UNPACK_ERR3					BIT(7)
#define	INTP_G_UNPACK_ERR2					BIT(6)
#define	INTP_G_UNPACK_ERR1					BIT(5)
#define	INTP_G_UNPACK_ERR0					BIT(4)
#define	INTP_DSC_UNDER_FLOW					BIT(0)
#define INTP_COMMON_1_EN					(INTP_OFFSET + 0x0030)
#define INTP_COMMON_1_CLR					(INTP_OFFSET + 0x0034)
#define INTP_COMMON_1_INFO					(INTP_OFFSET + 0x0038)


#define	INTP_V0_Y_DONE_WIN31				BIT(31)
#define	INTP_V0_Y_DONE_WIN30				BIT(30)
#define	INTP_V0_Y_DONE_WIN29				BIT(29)
#define	INTP_V0_Y_DONE_WIN28				BIT(28)
#define	INTP_V0_Y_DONE_WIN27				BIT(27)
#define	INTP_V0_Y_DONE_WIN26				BIT(26)
#define	INTP_V0_Y_DONE_WIN25				BIT(25)
#define	INTP_V0_Y_DONE_WIN24				BIT(24)
#define	INTP_V0_Y_DONE_WIN23				BIT(23)
#define	INTP_V0_Y_DONE_WIN22				BIT(22)
#define	INTP_V0_Y_DONE_WIN21				BIT(21)
#define	INTP_V0_Y_DONE_WIN20				BIT(20)
#define	INTP_V0_Y_DONE_WIN19				BIT(19)
#define	INTP_V0_Y_DONE_WIN18				BIT(18)
#define	INTP_V0_Y_DONE_WIN17				BIT(17)
#define	INTP_V0_Y_DONE_WIN16				BIT(16)
#define	INTP_V0_Y_DONE_WIN15				BIT(15)
#define	INTP_V0_Y_DONE_WIN14				BIT(14)
#define	INTP_V0_Y_DONE_WIN13				BIT(13)
#define	INTP_V0_Y_DONE_WIN12				BIT(12)
#define	INTP_V0_Y_DONE_WIN11				BIT(11)
#define	INTP_V0_Y_DONE_WIN10				BIT(10)
#define	INTP_V0_Y_DONE_WIN9 				BIT(9 )
#define	INTP_V0_Y_DONE_WIN8 				BIT(8 )
#define	INTP_V0_Y_DONE_WIN7 				BIT(7 )
#define	INTP_V0_Y_DONE_WIN6 				BIT(6 )
#define	INTP_V0_Y_DONE_WIN5 				BIT(5 )
#define	INTP_V0_Y_DONE_WIN4 				BIT(4 )
#define	INTP_V0_Y_DONE_WIN3 				BIT(3 )
#define	INTP_V0_Y_DONE_WIN2 				BIT(2 )
#define	INTP_V0_Y_DONE_WIN1 				BIT(1 )
#define	INTP_V0_Y_DONE_WIN0 				BIT(0 )
#define INTP_COMMON_2_EN					(INTP_OFFSET + 0x0040)
#define INTP_COMMON_2_CLR					(INTP_OFFSET + 0x0044)
#define INTP_COMMON_2_INFO					(INTP_OFFSET + 0x0048)

#define	INTP_V0_UV_DONE_WIN31				BIT(31)
#define	INTP_V0_UV_DONE_WIN30				BIT(30)
#define	INTP_V0_UV_DONE_WIN29				BIT(29)
#define	INTP_V0_UV_DONE_WIN28				BIT(28)
#define	INTP_V0_UV_DONE_WIN27				BIT(27)
#define	INTP_V0_UV_DONE_WIN26				BIT(26)
#define	INTP_V0_UV_DONE_WIN25				BIT(25)
#define	INTP_V0_UV_DONE_WIN24				BIT(24)
#define	INTP_V0_UV_DONE_WIN23				BIT(23)
#define	INTP_V0_UV_DONE_WIN22				BIT(22)
#define	INTP_V0_UV_DONE_WIN21				BIT(21)
#define	INTP_V0_UV_DONE_WIN20				BIT(20)
#define	INTP_V0_UV_DONE_WIN19				BIT(19)
#define	INTP_V0_UV_DONE_WIN18				BIT(18)
#define	INTP_V0_UV_DONE_WIN17				BIT(17)
#define	INTP_V0_UV_DONE_WIN16				BIT(16)
#define	INTP_V0_UV_DONE_WIN15				BIT(15)
#define	INTP_V0_UV_DONE_WIN14				BIT(14)
#define	INTP_V0_UV_DONE_WIN13				BIT(13)
#define	INTP_V0_UV_DONE_WIN12				BIT(12)
#define	INTP_V0_UV_DONE_WIN11				BIT(11)
#define	INTP_V0_UV_DONE_WIN10				BIT(10)
#define	INTP_V0_UV_DONE_WIN9 				BIT(9 )
#define	INTP_V0_UV_DONE_WIN8 				BIT(8 )
#define	INTP_V0_UV_DONE_WIN7 				BIT(7 )
#define	INTP_V0_UV_DONE_WIN6 				BIT(6 )
#define	INTP_V0_UV_DONE_WIN5 				BIT(5 )
#define	INTP_V0_UV_DONE_WIN4 				BIT(4 )
#define	INTP_V0_UV_DONE_WIN3 				BIT(3 )
#define	INTP_V0_UV_DONE_WIN2 				BIT(2 )
#define	INTP_V0_UV_DONE_WIN1 				BIT(1 )
#define	INTP_V0_UV_DONE_WIN0 				BIT(0 )
#define INTP_COMMON_3_EN					(INTP_OFFSET + 0x0050)
#define INTP_COMMON_3_CLR					(INTP_OFFSET + 0x0054)
#define INTP_COMMON_3_INFO					(INTP_OFFSET + 0x0058)


#define	INTP_V1_Y_DONE_WIN1 				BIT(11)
#define	INTP_V1_Y_DONE_WIN0 				BIT(10)
#define	INTP_V1_UV_DONE_WIN1 				BIT(9 )
#define	INTP_V1_UV_DONE_WIN0 				BIT(8 )
#define	INTP_V0_Y_DONE_WIN35 				BIT(7 )
#define	INTP_V0_Y_DONE_WIN34 				BIT(6 )
#define	INTP_V0_Y_DONE_WIN33 				BIT(5 )
#define	INTP_V0_Y_DONE_WIN32 				BIT(4 )
#define	INTP_V0_UV_DONE_WIN35 				BIT(3 )
#define	INTP_V0_UV_DONE_WIN34 				BIT(2 )
#define	INTP_V0_UV_DONE_WIN33 				BIT(1 )
#define	INTP_V0_UV_DONE_WIN32 				BIT(0 )
#define INTP_COMMON_4_EN					(INTP_OFFSET + 0x0060)
#define INTP_COMMON_4_CLR					(INTP_OFFSET + 0x0064)
#define INTP_COMMON_4_INFO					(INTP_OFFSET + 0x0068)

#define MIXER_ENABLE				(MIXER_OFFSET + 0x0004)
#define V0_BACKGROUND_COLOR	  		(MIXER_OFFSET + 0x0008)
#define V1_BACKGROUND_COLOR	  		(MIXER_OFFSET + 0x0270)

/*CROP*/
#define CROP_PARA0					(MIXER_OFFSET + 0x000c)
#define	CROP_PARA1					(MIXER_OFFSET + 0x0010)

/*ZOOMIN*/
#define ZOOMIN_PARA0				(MIXER_OFFSET + 0x0014)
#define ZOOMIN_PARA1				(MIXER_OFFSET + 0x0018)
#define ZOOMIN_PARA2				(MIXER_OFFSET + 0x001c)
#define ZOOMIN_PARA3				(MIXER_OFFSET + 0x0020)
#define ZOOMIN_PARA4				(MIXER_OFFSET + 0x0024)
#define ZOOMIN_CG					(MIXER_OFFSET + 0x0028)
#define ZOOMIN_VRAM_WR				(MIXER_OFFSET + 0x0030)
#define ZOOMIN_HRAM_WR				(MIXER_OFFSET + 0x0034)
#define ZOOMIN_ALLRAM_WR			(MIXER_OFFSET + 0x0038)

/*SCALER*/
#define SCALER_PARA0				(MIXER_OFFSET + 0x0300)
#define SCALER_PARA1				(MIXER_OFFSET + 0x0304)
#define SCALER_PARA2				(MIXER_OFFSET + 0x0308)
#define SCALER_PARA3				(MIXER_OFFSET + 0x030c)
#define SCALER_PARA4				(MIXER_OFFSET + 0x0310)
#define SCALER_CG					(MIXER_OFFSET + 0x0314)
#define SCALER_VRAM_WR				(MIXER_OFFSET + 0x0320)
#define SCALER_HRAM_WR				(MIXER_OFFSET + 0x0324)
#define SCALER_ALLRAM_WR			(MIXER_OFFSET + 0x0328)

/* PEAK */
#define PEAK_SIZE_CH(n)				(MIXER_OFFSET + 0x0090 + (n) * 4)
#define PEAK_INDEX_CH(n)      		(MIXER_OFFSET + 0x00b4 + (n) * 4)
#define PEAK_HSP              		(MIXER_OFFSET + 0x00d8)
#define PEAK_SMA_DTE_0        		(MIXER_OFFSET + 0x00dc)
#define PEAK_SMA_DTE_4        		(MIXER_OFFSET + 0x00e0)
#define PEAK_SMA_DTE_8        		(MIXER_OFFSET + 0x00e4)
#define PEAK_SMA_DTE_12       		(MIXER_OFFSET + 0x00e8)
#define PEAK_BIG_DTE_0        		(MIXER_OFFSET + 0x00ec)
#define PEAK_BIG_DTE_4        		(MIXER_OFFSET + 0x00f0)
#define PEAK_BIG_DTE_8        		(MIXER_OFFSET + 0x00f4)
#define PEAK_BIG_DTE_12       		(MIXER_OFFSET + 0x00f8)
#define PEAK_SB_DTE_16        		(MIXER_OFFSET + 0x00fc)
#define PEAK_MAX_SEL_0        		(MIXER_OFFSET + 0x0100)
#define PEAK_MAX_SEL_4        		(MIXER_OFFSET + 0x0104)
#define PEAK_MAX_SEL_8        		(MIXER_OFFSET + 0x0108)
#define PEAK_MAX_SEL_12       		(MIXER_OFFSET + 0x010c)
#define PEAK_MAX_SEL_16       		(MIXER_OFFSET + 0x0110)
#define PEAK_MAX_SEL_20       		(MIXER_OFFSET + 0x0114)
#define PEAK_MAX_SEL_24       		(MIXER_OFFSET + 0x0118)
#define PEAK_MIN_SEL_0        		(MIXER_OFFSET + 0x0120)
#define PEAK_MIN_SEL_4        		(MIXER_OFFSET + 0x0124)
#define PEAK_MIN_SEL_8        		(MIXER_OFFSET + 0x0128)
#define PEAK_MIN_SEL_12       		(MIXER_OFFSET + 0x012c)
#define PEAK_MM_SEL_16        		(MIXER_OFFSET + 0x0130)

/* DLI */
#define DLI_SIZE_CH(n)        		(MIXER_OFFSET + 0x0178 + (n) * 4)
#define DLI_INDEX_CH(n)       		(MIXER_OFFSET + 0x019c + (n) * 4)
#define DLI_RAM_CLR           		(MIXER_OFFSET + 0x0140)
#define DLI_RAM_CG            		(MIXER_OFFSET + 0x0144)
#define DLI_RAM_WR            		(MIXER_OFFSET + 0x0148)
#define DLI_RAM_FLAG          		(MIXER_OFFSET + 0x014c)
#define DLI_STA_RAM_FLAG      		(MIXER_OFFSET + 0x0160)
#define DLI_STA_RAM_CG        		(MIXER_OFFSET + 0x0164)
#define DLI_STA_RAM_ADDR      		(MIXER_OFFSET + 0x0168)
#define DLI_STA_RAM_EN        		(MIXER_OFFSET + 0x016c)
#define DLI_STA_RAM0_RD       		(MIXER_OFFSET + 0x0170)
#define DLI_STA_RAM1_RD       		(MIXER_OFFSET + 0x0174)

/* BCSH */
#define BCSH_CLIP_Y0          		(MIXER_OFFSET + 0x0200)
#define BCSH_CLIP_Y1          		(MIXER_OFFSET + 0x0204)
#define BCSH_CLIP_Y2          		(MIXER_OFFSET + 0x0208)
#define BCSH_CLIP_C0          		(MIXER_OFFSET + 0x020c)
#define BCSH_CLIP_C1          		(MIXER_OFFSET + 0x0210)
#define BCSH_CLIP_C2          		(MIXER_OFFSET + 0x0214)
#define BCSH_OFFSET_Y         		(MIXER_OFFSET + 0x0218)
#define BCSH_OFFSET_U         		(MIXER_OFFSET + 0x021c)
#define BCSH_OFFSET_V         		(MIXER_OFFSET + 0x0220)
#define BCSH_MATRIX_COEF0     		(MIXER_OFFSET + 0x0224)
#define BCSH_MATRIX_COEF1     		(MIXER_OFFSET + 0x0228)
#define BCSH_MATRIX_COEF2     		(MIXER_OFFSET + 0x022c)
#define BCSH_MATRIX_COEF3     		(MIXER_OFFSET + 0x0230)
#define BCSH_MATRIX_COEF4     		(MIXER_OFFSET + 0x0234)
#define BCSH_CON_ENABLE       		(MIXER_OFFSET + 0x0254)
#define BCSH_CON_S1           		(MIXER_OFFSET + 0x0258)
#define BCSH_CON_X            		(MIXER_OFFSET + 0x025c)
#define BCSH_CON_Y            		(MIXER_OFFSET + 0x0260)
#define BCSH_SAT_ENABLE       		(MIXER_OFFSET + 0x0264)
#define BCSH_SAT_TH           		(MIXER_OFFSET + 0x0268)
#define BCSH_SAT_STREN        		(MIXER_OFFSET + 0x026c)

/* CSC rgb2yuv */
#define CSC_YUV2RGB_COEF00    		(MIXER_OFFSET + 0x02a0)
#define CSC_YUV2RGB_COEF01    		(MIXER_OFFSET + 0x02a4)
#define CSC_YUV2RGB_COEF02    		(MIXER_OFFSET + 0x02a8)
#define CSC_YUV2RGB_COEF03    		(MIXER_OFFSET + 0x02ac)
#define CSC_YUV2RGB_COEF04    		(MIXER_OFFSET + 0x02b0)
#define CSC_YUV2RGB_THRESHOLD 		(MIXER_OFFSET + 0x02b4)
#define CSC_YUV2RGB_CLIP      		(MIXER_OFFSET + 0x02b8)

/* alpha & colorkey*/
#define ALPHA_VALUE					(MIXER_OFFSET + 0x02bc)
#define G_PARA0						(MIXER_OFFSET + 0x02c0)
#define G_PARA1						(MIXER_OFFSET + 0x02c4)
#define G_PARA2						(MIXER_OFFSET + 0x02d0)
#define G_PARA3						(MIXER_OFFSET + 0x02d4)
#define G_PARA4						(MIXER_OFFSET + 0x02d8)
#define M_PARA0						(MIXER_OFFSET + 0x02c8)
#define M_PARA1						(MIXER_OFFSET + 0x02cc)
#define M_PARA2						(MIXER_OFFSET + 0x02dc)
#define M_PARA3						(MIXER_OFFSET + 0x02e0)
#define M_PARA4						(MIXER_OFFSET + 0x02e4)

#define DSC_ENABLE					(DSC_OFFSET + 0x0000)
#define DSC_SIZE            		(DSC_OFFSET + 0x0010)
#define DSC_TOTAL           		(DSC_OFFSET + 0x0020)
#define DSC_HSYNC_HDMI      		(DSC_OFFSET + 0x0024)
#define DSC_VSYNC_HDMI      		(DSC_OFFSET + 0X0028)
#define DSC_HSYNC_VGA       		(DSC_OFFSET + 0x002c)
#define DSC_VSYNC_VGA       		(DSC_OFFSET + 0X0030)
#define DSC_HDE						(DSC_OFFSET + 0x0034)
#define DSC_VDE						(DSC_OFFSET + 0x0038)
#define DSC_POLARITY        		(DSC_OFFSET + 0x003c)
#define DSC_START_CON       		(DSC_OFFSET + 0x0040)
#define DSC_PATTERN         		(DSC_OFFSET + 0x0044)
#define DSC_CSC_CON0        		(DSC_OFFSET + 0x0050)
#define DSC_CSC_CON1        		(DSC_OFFSET + 0x0054)
#define DSC_CSC_CON2        		(DSC_OFFSET + 0x0058)
#define DSC_CSC_CON3        		(DSC_OFFSET + 0x005c)
#define DSC_CSC_CON4        		(DSC_OFFSET + 0x0060)
#define DSC_CSC_THRES       		(DSC_OFFSET + 0x0064)
#define DSC_CSC_CLIP        		(DSC_OFFSET + 0x0068)
#define DSC_TFT_FORMAT      		(DSC_OFFSET + 0x0080)
#define DSC_VGA_FORMAT      		(DSC_OFFSET + 0x0084)
#define DSC_CEC_OE          		(DSC_OFFSET + 0x0090)
#define DSC_YUV_MODE          		(DSC_OFFSET + 0x00B4)
#define DSC_VGA_RGB_ORDER          	(DSC_OFFSET + 0x00A4)

#endif
