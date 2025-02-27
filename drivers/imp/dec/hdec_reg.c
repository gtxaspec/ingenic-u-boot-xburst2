#include <common.h>
#include <asm/io.h>
#include <asm/types.h>
#include <command.h>
#include "hdec_reg.h"

#ifdef JPEG_DEBUG
#define TR(fmt, args...)                \
	do {                                \
		printf("[JPEG]: " fmt, ##args);    \
	} while(0)
#else
	#define TR(fmt, args...)
#endif

void jpegWriteReg(u32 RegBase, u32 RegOffset, u32 RegData)
{
	u32 addr = (u32)RegBase + RegOffset;
	TR("=====>%s RegBase = 0x%08x RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, (u32)RegBase, RegOffset, RegData);
	*((volatile unsigned int *)addr) = RegData;
	return;
}

u32 jpegReadReg(u32 RegBase, u32 RegOffset)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);
	TR("=====>%s: RegBase = 0x%08x RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, (u32)RegBase, RegOffset, data);
	return data;
}

void jpegSetBits(u32 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);
	data |= BitPos;
	TR("=====>%s  RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data);
	writel(data,(void *)addr);
	TR("=====>%s  RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data);
	return;
}

void jpegClearBits(u32 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);
	data &= (~BitPos);
	TR("=====>%%s RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data);
	writel(data,(void *)addr);
	TR("=====>%%s RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data);
	return;
}

bool jpegCheckBits(u32 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readl((void *)addr);
	data &= BitPos;
	if(data)  return true;
	else    return false;
}
