#include <stdio.h>
#include "nand_common.h"

#define MIDA1_MID			    0xA1

static unsigned char xtx_xaw[] = {0x2};
static unsigned char fm25s_01a[] = {0x2};
static unsigned char yxsc_er[] = {0x4, 0x7};

static struct device_struct device[] = {
	DEVICE_STRUCT(0xE1, 2048, 2, 4, 2, 1, xtx_xaw),  /* PN26G01AW -- XTX nand flash */
	DEVICE_STRUCT(0xE2, 2048, 2, 4, 2, 1, xtx_xaw),  /* PN26G02AW -- XTX nand flash */
	DEVICE_STRUCT(0xC1, 2048, 2, 4, 2, 1, xtx_xaw),	 /* PN26Q01AW -- FundanMicro nand flash*/
	DEVICE_STRUCT(0xF1, 2048, 2, 4, 2, 2, yxsc_er),	 /* TX25G01 -- FundanMicro nand flash*/
	DEVICE_STRUCT(0xE4, 2048, 2, 4, 2, 2, fm25s_01a),/* FM25S01A -- FundanMicro nand flash*/
	DEVICE_STRUCT(0xE5, 2048, 2, 4, 2, 2, fm25s_01a),/* FM25S02A -- FundanMicro nand flash*/
};

static struct nand_desc mida1_nand = {

	.id_manufactory = MIDA1_MID,
	.device_counts = sizeof(device) / sizeof(device[0]),
	.device = device,
};

int mida1_nand_register_func(void) {
	return nand_register(&mida1_nand);
}
