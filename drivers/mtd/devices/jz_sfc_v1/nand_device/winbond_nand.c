#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_nand.h"
#include "nand_common.h"

#define TSETUP		5
#define THOLD		3
#define	TSHSL_R		10
#define	TSHSL_W		50

#define WINDOND_DIE_SELECT	0xC2
#define WINDOND_RESET		0xFF

static struct jz_nand_base_param winbond_param[] = {
	[0] = {
		/*W25N01GV*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  =TSETUP,
		.tHOLD   =THOLD,
		.tSHSL_R =TSHSL_R,
		.tSHSL_W =TSHSL_W,

		.ecc_max = 0x4,
#ifdef CONFIG_SPI_STANDARD
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},

	[1] = {
		/*W25M02GV */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  =TSETUP,
		.tHOLD   =THOLD,
		.tSHSL_R =TSHSL_R,
		.tSHSL_W =TSHSL_W,

		.ecc_max = 0x4,
#ifdef CONFIG_SPI_STANDARD
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},

    [2] = {
		/*W25M01KV */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  =TSETUP,
		.tHOLD   =THOLD,
		.tSHSL_R =TSHSL_R,
		.tSHSL_W =TSHSL_W,

		.ecc_max = 0x4,
#ifdef CONFIG_SPI_STANDARD
		.need_quad = 0,
#else
		.need_quad = 1,
#endif

	},
};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0xAA, "W25N01GV", &winbond_param[0]),
	DEVICE_ID_STRUCT(0xAB, "W25M02GV", &winbond_param[1]),
	DEVICE_ID_STRUCT(0xAE, "W25M01KV", &winbond_param[2]),
};

static void active_die(struct sfc_flash *flash, uint8_t die_id) {

	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));
	sfc_message_init(&message);

	cmd.cmd = WINDOND_DIE_SELECT;
	transfer.sfc_mode = TM_STD_SPI;

	transfer.addr = die_id;
	transfer.addr_len = 1;

	cmd.dataen = DISABLE;
	transfer.len = 0;

	transfer.data_dummy_bits = 0;
	transfer.cmd_info = &cmd;
	transfer.ops_mode = CPU_OPS;

	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message)) {
	        printf("sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return;
	}
}
#ifdef WINBOND_DEBUG
static void winbond_reset(struct sfc_flash *flash) {

	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));
	sfc_message_init(&message);

	cmd.cmd = WINDOND_RESET;
	transfer.sfc_mode = TM_STD_SPI;

	transfer.addr = 0;
	transfer.addr_len = 0;

	cmd.dataen = DISABLE;
	transfer.len = 0;

	transfer.data_dummy_bits = 0;
	transfer.cmd_info = &cmd;
	transfer.ops_mode = CPU_OPS;

	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message)) {
	        printf("sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return;
	}
}

static void winbond_print_register(struct sfc_flash *flash, uint8_t register_addr) {

	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;
	uint8_t ret = 0;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));

	sfc_message_init(&message);
	nand_get_feature(&transfer, &cmd, register_addr, &ret);
	sfc_message_add_tail(&transfer, &message);

	if(sfc_sync(flash->sfc, &message)) {
	        printf("sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return;
	}

	printf("printk register (%x) = %x\n", register_addr, ret);

}

#endif

static void winbond_pageread_to_cache(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	uint8_t device_id = nand_desc->id_device;

	switch(device_id) {
		case 0xAB:
			if(op_info->pageaddr > 65535)
				active_die(flash, 1);
			else
				active_die(flash, 0);
		case 0xAA:
			break;
        case 0xAE:
			break;
		default:
			pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
	}

	cmd->cmd = SPINAND_CMD_PARD;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = op_info->pageaddr;
	transfer->addr_len = 3;

	cmd->dataen = DISABLE;
	transfer->len = 0;

	transfer->data_dummy_bits = 0;
	transfer->cmd_info = cmd;
	transfer->ops_mode = CPU_OPS;
	return;
}

static void winbond_single_read(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info) {

	cmd->cmd = SPINAND_CMD_FRCH;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = op_info->columnaddr;
	transfer->addr_len = 2;

	cmd->dataen = ENABLE;
	transfer->data = op_info->buffer;
	transfer->len = op_info->len;
	transfer->direction = GLB_TRAN_DIR_READ;

	transfer->data_dummy_bits = 8;
	transfer->cmd_info = cmd;
	transfer->ops_mode = DMA_OPS;
	return;
}

static void winbond_quad_read(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info) {

	cmd->cmd = SPINAND_CMD_RDCH_X4;
	transfer->sfc_mode = TM_QI_QO_SPI;

	transfer->addr = op_info->columnaddr;
	transfer->addr_len = 2;

	cmd->dataen = ENABLE;
	transfer->data = op_info->buffer;
	transfer->len = op_info->len;
	transfer->direction = GLB_TRAN_DIR_READ;

	transfer->data_dummy_bits = 8;
	transfer->cmd_info = cmd;
	transfer->ops_mode = DMA_OPS;
	return;
}

static int32_t winbond_get_read_feature(struct flash_operation_message *op_info) {

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
		case 0xAA ... 0xAB:
        case 0XAE:
			switch((ecc_status >> 4) & 0x3) {
			    case 0x2:
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

	if(ret < 0)
		printf("%s %s %d, ecc_status = %x, ret = %d\n",
			__FILE__, __func__, __LINE__, ecc_status, ret);
	return ret;
}

static void winbond_set_register(struct sfc_flash *flash, uint8_t register_addr, uint8_t val) {

	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));

	sfc_message_init(&message);
	nand_set_feature(&transfer, &cmd, register_addr, val);
	sfc_message_add_tail(&transfer, &message);

	if(sfc_sync(flash->sfc, &message)) {
	        printf("sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return;
	}

}

static void winbond_write_enable(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	uint8_t device_id = nand_desc->id_device;

	switch(device_id) {
		case 0xAB:
			if(op_info->pageaddr > 65535) {
				active_die(flash, 1);
				/*clear protect bits, because each die
				 * has a set of state registers. */
				winbond_set_register(flash, SPINAND_ADDR_PROTECT, 0);
				winbond_set_register(flash, SPINAND_ADDR_FEATURE, (1 << 4) | (1 << 3));
			} else {
				active_die(flash, 0);
			}
		case 0xAA:
		    break;
        case 0xAE:
		    break;
		default:
			pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
	}

	cmd->cmd = SPINAND_CMD_WREN;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = 0;
	transfer->addr_len = 0;

	cmd->dataen = DISABLE;
	transfer->len = 0;

	transfer->data_dummy_bits = 0;
	transfer->cmd_info = cmd;
	transfer->ops_mode = CPU_OPS;
	return;
}

static void winbond_program_exec(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	uint8_t device_id = nand_desc->id_device;
	uint32_t pageaddr = op_info->pageaddr;

	switch(device_id) {
	    case 0xAB:
		if(pageaddr > 65535)
		    pageaddr -= 65536;
	    case 0xAA:
		break;
        case 0xAE:
		break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
	}

	cmd->cmd = SPINAND_CMD_PRO_EN;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = pageaddr;
	transfer->addr_len = 3;

	cmd->dataen = DISABLE;
	transfer->len = 0;

	transfer->data_dummy_bits = 0;
	transfer->cmd_info = cmd;
	transfer->ops_mode = CPU_OPS;
}

static void winbond_block_erase(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	uint8_t device_id = nand_desc->id_device;
	uint32_t pageaddr = op_info->pageaddr;

	switch(device_id) {
	    case 0xAB:
		if(pageaddr > 65535)
		    pageaddr -= 65536;
	    case 0xAA:
		break;
        case 0xAE:
		break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
	}

	cmd->cmd = SPINAND_CMD_ERASE_128K;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = pageaddr;
	transfer->addr_len = 3;

	cmd->dataen = DISABLE;
	transfer->len = 0;

	transfer->data_dummy_bits = 0;
	transfer->cmd_info = cmd;
	transfer->ops_mode = CPU_OPS;

}

int winbond_nand_init(void) {
	struct jz_nand_device *winbond_nand;
	winbond_nand = kzalloc(sizeof(*winbond_nand), GFP_KERNEL);
	if(!winbond_nand) {
		pr_err("alloc winbond_nand struct fail\n");
		return -ENOMEM;
	}

	winbond_nand->id_manufactory = 0xEF;
	winbond_nand->id_device_list = device_id;
	winbond_nand->id_device_count = sizeof(device_id) / sizeof(device_id[0]);

	winbond_nand->ops.nand_read_ops.pageread_to_cache = winbond_pageread_to_cache;
	winbond_nand->ops.nand_read_ops.single_read = winbond_single_read;
	winbond_nand->ops.nand_read_ops.quad_read = winbond_quad_read;
	winbond_nand->ops.nand_read_ops.get_feature = winbond_get_read_feature;

	winbond_nand->ops.nand_write_ops.write_enable = winbond_write_enable;
	winbond_nand->ops.nand_write_ops.program_exec = winbond_program_exec;

	winbond_nand->ops.nand_erase_ops.write_enable = winbond_write_enable;
	winbond_nand->ops.nand_erase_ops.block_erase = winbond_block_erase;

	return jz_spinand_register(winbond_nand);
}
SPINAND_MOUDLE_INIT(winbond_nand_init);
