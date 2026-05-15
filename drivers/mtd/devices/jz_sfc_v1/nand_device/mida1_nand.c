#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_nand.h"
#include "nand_common.h"

#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		20
#define	TSHSL_W		20

static struct jz_nand_base_param mida1_param[] = {

	[0] = {
		/* PN26G01AW */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.ecc_max = 0x8,
#ifdef CONFIG_SPI_STANDARD
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
	[1] = {
		/* PN26G02AW */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.ecc_max = 0x8,
#ifdef CONFIG_SPI_STANDARD
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
	[2] = {
		/* TX25G01 */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP = TSETUP,
		.tHOLD = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.ecc_max = 0x8,
#ifdef CONFIG_SPI_STANDARD
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
    [3] = {
        /* PN26Q01AW */
        .pagesize = 2 * 1024,
        .blocksize = 2 * 1024 * 64,
        .oobsize = 128,
        .flashsize = 2 * 1024 * 64 * 1024,

        .tSETUP = TSETUP,
        .tHOLD = THOLD,
        .tSHSL_R = TSHSL_R,
        .tSHSL_W = TSHSL_W,

        .ecc_max = 0x8,
#ifdef CONFIG_SPI_STANDARD
        .need_quad = 0,
#else
        .need_quad = 1,
#endif
    },
	[4] = {
		/* FM25S01A */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP = TSETUP,
		.tHOLD = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.ecc_max = 0x2,
#ifdef CONFIG_SPI_STANDARD
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
	[5] = {
		/*FM25S02A*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.ecc_max = 0x1,
#ifdef CONFIG_SPI_STANDARD
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0xE1, "PN26G01AW", &mida1_param[0]),
	DEVICE_ID_STRUCT(0xE2, "PN26G02AW", &mida1_param[1]),
	DEVICE_ID_STRUCT(0xC1, "TX25G01", &mida1_param[2]),
	DEVICE_ID_STRUCT(0xF1, "PN26Q01AW", &mida1_param[3]),
	DEVICE_ID_STRUCT(0xE4, "FM25S01A",  &mida1_param[4]),
	DEVICE_ID_STRUCT(0xE5, "FM25S02A", &mida1_param[5]), //FM25S02A
};

static int32_t mida1_get_read_feature(struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;
	uint8_t device_id = nand_desc->id_device;
	uint8_t ecc_status = 0;
	int32_t ret = 0;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));
	sfc_message_init(&message);

	cmd.cmd = SPINAND_CMD_GET_FEATURE;
	transfer.sfc_mode = TM_STD_SPI;

	transfer.addr = SPINAND_ADDR_STATUS;
	transfer.addr_len = 1;

	cmd.dataen = DISABLE;
	transfer.len = 0;

	transfer.data_dummy_bits = 0;
	cmd.sta_exp = (0 << 0);
	cmd.sta_msk = SPINAND_IS_BUSY;
	transfer.cmd_info = &cmd;
	transfer.ops_mode = CPU_OPS;

	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message)) {
	        printf("sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	ecc_status = sfc_get_sta_rt(flash->sfc);

	switch(device_id) {
		case 0xE1 ... 0xE2:
		case 0xC1:
		case 0xE4:
		case 0xE5:
			switch((ecc_status >> 4) & 0x3) {
			    case 0x02:
				    ret = -EBADMSG;
				    break;
			    default:
				    ret = 0;
			}
			break;
		case 0xF1:
			switch((ecc_status >> 4) & 0x7) {
			    case 0x04:
			    case 0x07:
				    ret = -EBADMSG;
				    break;
			    default:
				    ret = 0;
			}
			break;
		default:
			printf("device_id err, it maybe don`t support this device, check your device id: device_id = 0x%02x\n", device_id);
			ret = -EIO;   //notice!!!

	}
	return ret;
}

int mida1_nand_init(void) {
	struct jz_nand_device *mida1_nand;
	mida1_nand = kzalloc(sizeof(*mida1_nand), GFP_KERNEL);
	if(!mida1_nand) {
		pr_err("alloc mida1_nand struct fail\n");
		return -ENOMEM;
	}

	mida1_nand->id_manufactory = 0xA1;
	mida1_nand->id_device_list = device_id;
	mida1_nand->id_device_count = sizeof(device_id) / sizeof(device_id[0]);

	mida1_nand->ops.nand_read_ops.get_feature = mida1_get_read_feature;
	return jz_spinand_register(mida1_nand);
}
SPINAND_MOUDLE_INIT(mida1_nand_init);
