#include <stdio.h>
#include "nand_common.h"

#define DS_MID 0xE5

static unsigned char ds_x1ga[] = {0x2};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x71, 2048, 2, 4, 2, 1, ds_x1ga),
};

static struct nand_desc dosilicon_nand = {

	.id_manufactory = DS_MID,
	.device_counts = sizeof(device) / sizeof(device[0]),
	.device = device,
};

int dosilicon_nand_register_func(void)
{
	return nand_register(&dosilicon_nand);
}
