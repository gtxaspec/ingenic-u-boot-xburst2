#include <common.h>
#include <command.h>
#include <serial.h>

#define SERIAL_NUM_ADDR1 0x13540200
#define SERIAL_NUM_ADDR2 0x13540204
#define SERIAL_NUM_ADDR3 0x13540208
#define SERIAL_NUM_ADDR4 0x13540238 // T10/T20/T30

int debug_socinfo = 0;

typedef struct {
	const char* name;
	unsigned int socId;
	unsigned int cppsr;
	unsigned int subremark;
	unsigned int subsoctype1;
	unsigned int subsoctype2;
} SocInfo;

SocInfo socInfoTable[] = {
	{"T40NN", 0x0040, 0x00ED, 0x0000, 0x0000, 0x8888},
	{"T40XP", 0x0040, 0x00f9, 0x0000, 0x0000, 0x7777},
	{"T41N", 0x0040, 0x0000, 0x0000, 0x0000, 0x1111},
	{"T41NQ", 0x0040, 0x0000, 0x0000, 0x0000, 0xAAAA},
	{"T41L", 0x0040, 0x0000, 0x0000, 0x0000, 0x3333},
	{"T41LQ", 0x0040, 0x0000, 0x0000, 0x0000, 0x9999},
	{"T41A", 0x0040, 0x0000, 0x0000, 0x0000, 0x4444},
	{"T41ZL", 0x0040, 0x0000, 0x0000, 0x0000, 0x5555},
	{"T41ZN", 0x0040, 0x0000, 0x0000, 0x0000, 0x7777},
	{"T41ZX", 0x0040, 0x0000, 0x0000, 0x0000, 0x6666},
};

static const char* get_soc_name(void) {
	unsigned int soc_id = *((volatile unsigned int *)(0x1300002C));
	unsigned int subsoctype1 = *((volatile unsigned int *)(0x13540238));
	unsigned int subsoctype2 = *((volatile unsigned int *)(0x13540250));
	unsigned int cpu_id = (soc_id >> 12) & 0xFFFF;
	unsigned int subsoctype1_shifted = (subsoctype1 >> 16) & 0xFFFF;
	unsigned int subsoctype2_shifted = (subsoctype2 >> 16) & 0xFFFF;

	uint32_t serial_part1 = readl(SERIAL_NUM_ADDR1);
	uint32_t serial_part2 = readl(SERIAL_NUM_ADDR2);
	uint32_t serial_part3 = readl(SERIAL_NUM_ADDR3);
	uint32_t serial_part4 = readl(SERIAL_NUM_ADDR4 + 4);

	if (debug_socinfo) {
		printf("soc_id:      0x%08X [0x%04X]\n", soc_id, cpu_id);
		printf("subsoctype1: 0x%08X [0x%04X]\n", subsoctype1, subsoctype1_shifted);
		printf("subsoctype2: 0x%08X [0x%04X]\n", subsoctype2, subsoctype2_shifted);
		printf("Serial number: %u %u %u %u\n", serial_part1, serial_part2, serial_part3, serial_part4);
	}

	int i;
	for (i = 0; i < sizeof(socInfoTable) / sizeof(SocInfo); i++) {
		SocInfo s = socInfoTable[i];
		if (cpu_id == s.socId &&
			subsoctype1_shifted == s.subsoctype1 &&
			subsoctype2_shifted == s.subsoctype2) {
			return s.name;
		}
	}

	return "Unknown";
}

int do_socinfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	const char *soc_name = get_soc_name();
	printf("SOC Name: %s\n", soc_name);
	return CMD_RET_SUCCESS;
}

int do_socinfo_cmd_wrapper(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	debug_socinfo = 1;
	int result = do_socinfo(cmdtp, flag, argc, argv);
	debug_socinfo = 0;
	return result;
}

U_BOOT_CMD(
	socinfo, CONFIG_SYS_MAXARGS, 1, do_socinfo_cmd_wrapper,
	"Display SOC information",
	"Usage: socinfo - Displays the SOC type based on the ingenic hardware registers"
);
 