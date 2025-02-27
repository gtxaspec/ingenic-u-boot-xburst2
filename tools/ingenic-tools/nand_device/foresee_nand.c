#include <stdio.h>
#include "nand_common.h"

#define ATO_MID			    0xCD

static unsigned char fw_xaw[] = {0x05, 0x06};
static unsigned char fs_xaw[] = {0x02, 0x03};

static struct device_struct device[] = {
	DEVICE_STRUCT(0xEA, 2048, 2, 4, 3, 2, fw_xaw),//FS35ND01G
	DEVICE_STRUCT(0x71, 2048, 2, 4, 2, 2, fs_xaw),
};

static struct nand_desc foresee_nand = {

	.id_manufactory = ATO_MID,
	.device_counts = sizeof(device) / sizeof(device[0]),
	.device = device,
};

int foresee_nand_register_func(void) {
	return nand_register(&foresee_nand);
}
