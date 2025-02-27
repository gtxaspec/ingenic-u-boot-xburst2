#ifndef __HDEC_REG_H__
#define __HDEC_REG_H__

#include <asm/arch-a1/cpm.h>
#include <asm/io.h>
#include <asm/types.h>

#define BIT(x)				(0x1 << x)
#define REG_SCH_GLBC		0x00000
#define REG_SCH_STAT		0x00034
#define REG_VDMA_TASKRG     0x00084
#define TCSM_FLUSH          0xC0000
#define REG_JPGC_TRIG       0xE0000
#define REG_JPGC_GLBI       0xE0004
#define REG_JPGC_STAT       0xE0008
#define REG_JPGC_BSA        0xE000C
#define REG_JPGC_P0A        0xE0010
#define REG_JPGC_P1A        0xE0014
#define REG_JPGC_NMCU       0xE0028
#define REG_JPGC_NRSM       0xE002C
#define REG_JPGC_P0C        0xE0030
#define REG_JPGC_P1C        0xE0034
#define REG_JPGC_P2C        0xE0038
#define REG_JPGC_P3C        0xE003C
#define REG_JPGC_WIDTH      0xE0040
#define REG_JPGC_MCUS		0xE0064
#define REG_JPGC_HUFB       0xE1200
#define REG_JPGC_HUFM       0xE1300
#define REG_JPGC_QMEM       0xE1400
#define REG_JPGC_HUFS       0xE1800
#define HUFFMIN_LEN		(16)
#define HUFFBASE_LEN    (64)
#define HUFFSYM_LEN     (336)
#define QMEM_LEN        (256)
#define YUV420PVH           (0xa<<16)
#define JPGC_CORE_OPEN      (0x1<<0)
#define SCH_STAT_ENDF		(0x1<<0)
#define JPGC_BS_TRIG        (0x1<<1)
#define JPGC_PP_TRIG        (0x1<<2)
#define JPEG_DEC            (0x1<<3)
#define SCH_STAT_JPGEND     (0x1<<4)
#define SCH_GLBC_HIAXI      (0x1<<9)
#define SCH_INTE_ENDF       (0x1<<16)
#define SCH_INTE_BSERR      (0x1<<17)
#define SCH_INTE_ACFGERR    (0x1<<20)
#define SCH_INTE_RESERR     (0x1<<29)
#define VDMA_ACFG_TERM      (0x1<<30)
#define VDMA_ACFG_VLD       (0x1<<31)
#define JPGC_ACK			(0x14)
#define JPGC_STP			(0x15)
#define JPGC_SR				(0x16)

#define VDMA_ACFG_DHA(a)	(((uint32_t)(a)) & 0xFFFFFF80)
#define VDMA_ACFG_IDX(a)	(((uint32_t)(a)) & 0xFFFFC)
#define GEN_VDMA_ACFG(chn, reg, lk, val)	\
	do									\
	{								\
		(*((chn)++)) = (val);	\
		(*((chn)++)) = (VDMA_ACFG_VLD | (lk) | VDMA_ACFG_IDX(reg));	\
	} while(0)

void jpegWriteReg(u32 RegBase, u32 RegOffset, u32 RegData);
u32 jpegReadReg(u32 RegBase, u32 RegOffset);
void jpegSetBits(u32 RegBase, u32 RegOffset, u32 BitPos);
void jpegClearBits(u32 RegBase, u32 RegOffset, u32 BitPos);
bool jpegCheckBits(u32 RegBase, u32 RegOffset, u32 BitPos);

#endif
