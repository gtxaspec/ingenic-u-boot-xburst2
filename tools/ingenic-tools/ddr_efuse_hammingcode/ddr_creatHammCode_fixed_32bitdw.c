#include <stdio.h>
#include <string.h>

struct ddr3_16bit {
	int odt_pd;
	int odt_pu;
	int drvcmd_pd;
	int drvcmd_pu ;
	int drvcmdck_pd;
	int drvcmdck_pu;
	int drvalah_pd;
	int drvalah_pu;
    int drvblbh_pd;
    int drvblbh_pu;
	int dqA;
    int dqB;
	int vref;
	int kgd_odt;
	int kgd_ds;
};

struct ddr3_16bit_de {
	int odt_pd;
	int odt_pu;
	int drvcmd_pd;
	int drvcmd_pu ;
	int drvcmdck_pd;
	int drvcmdck_pu;
	int drvalah_pd;
	int drvalah_pu;
    int drvblbh_pd;
    int drvblbh_pu;
	int dqA;
    int dqB;
	int vref;
	int kgd_odt;
	int kgd_ds;
};

/*
ddr3_param_t a1nt_ddr3_param = {
    .version = "A1NT@V20230825a",
    .params = {
        [odt_pd] = 3,
        [odt_pu] = 1,
        [drvcmd_pd] = 0xe,
        [drvcmd_pu] = 0xe,
        [drvcmdck_pd] = 0xe,
        [drvcmdck_pu] = 0xe,
        [drvalah_pd] = 0x14,
        [drvalah_pu] = 0x14,
        [drvblbh_pd] = 0x14,
        [drvblbh_pu] = 0x14,
        [dqA] = 0x10,
        [dqB] = 0xf,
        [vref] = 0x80,
        [kgd_odt] = 2,
        [kgd_ds] = 1,
        [dqsA] = 0x20,
        [dqsB] = 0x21
    }
};

const int A1_DDR_ODT_REMAP[16] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB, 0xD,0x18,0x1B,0x1F};

const int A1_DDR_DRVCMD_REMAP[16] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB, 0xE,0x18,0x1B,0x1F};
const int A1_DDR_DRVDATA_REMAP[16] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB, 0x14,0x18,0x1B,0x1F};


const int A1_DDR_DQ_REMAP[16] = { 5,7,9,12,13,14,15,16,17,18,19,20,21,22,24,26};//{7,12,14,16,18,20,22,24};
const int A1_DDR_VREF_REMAP[8] = {118,128,138,148,158,168,178,188}; // 定一个中间值，左右偏。

*/

struct ddr3_16bit ddr3_data = {
	.odt_pd = 3,
	.odt_pu = 0,
	.drvcmd_pd = 13,
	.drvcmd_pu = 13,
	.drvcmdck_pd = 13,
	.drvcmdck_pu = 13,

	.drvalah_pd = 13,
	.drvalah_pu = 13,
	.drvblbh_pd = 13,
	.drvblbh_pu = 13,
	.dqA = 7,
	.dqB = 5,
	.vref = 2,
    .kgd_odt = 0x2, // 010
    .kgd_ds = 0x1,  // 1
};

typedef struct {
	int odt_pd[4];
	int odt_pu[4];
	int drvcmd_pd[4];
	int drvcmd_pu[4] ;
	int drvcmdck_pd[4];
	int drvcmdck_pu[4];
	int drvalah_pd[4];
	int drvalah_pu[4];
	int drvblbh_pd[4];
	int drvblbh_pu[4];
	int dqA[4];
	int dqB[4];
	int vref[3];
	int kgd_odt[3];
	int kgd_ds[2];

}hanming_ddr_data;

int check_ParityBits(int dataBits) {
	int m = 1;
	while ((1 << m) < (dataBits + m + 1)) {
		m++;
	}
	return m;
}

void creat_HammingCode(int data[], int dataBits, int hammingCode[]) {
	int numParityBits = check_ParityBits(dataBits);
	int i, j, k = 0;
	int position = 0;
	int bitMask = 0;
	int parity = 0;

	printf("ParityBits num is %d\n", numParityBits);
	for (i = 0, j = 0, k = 0; i < dataBits + numParityBits; i++) {
		/*校验位先入0*/
		if ((i + 1) == (1 << j)) {
			hammingCode[i] = 0;
			j++;
		}
		else {
			hammingCode[i] = data[k++];
		}
	}

	for (i = 0; i < numParityBits; i++) {
		position = (1 << i);
		bitMask = position;
		parity = 0;

		/*用异或运算计算奇偶校验位的值*/
		for (j = position; j <= dataBits + numParityBits; j++) {
			if (j & bitMask) {
				/* 0 ^ 0 = 0; 1 ^ 0 = 1*/
				parity ^= hammingCode[j - 1];
			}
		}

		hammingCode[position - 1] = parity;
	}
	parity = 0;
	for(int i =0; i < dataBits + numParityBits; i++)
	{
		parity ^= hammingCode[i];
	}
	hammingCode[dataBits + numParityBits] = parity;
}
/*
void get_index_ddr(int index, int data)
{
	struct ddr3_16bit_de ddr3_data;
	switch(index)
	{
		case 0x0:
			ddr3_data.odt_pd = data;
			printf("get ddr3_data.odt_pd is %x\n", ddr3_data.odt_pd);
			break;
		case 0x1:
			ddr3_data.odt_pu = data;
			printf("get ddr3_data.odt_pu is %x\n", ddr3_data.odt_pu);
			break;
		case 0x2:
			ddr3_data.drvcmd_pd = data;
			printf("get ddr3_data.drvcmd_pd is %x\n", ddr3_data.drvcmd_pd);
			break;
		case 0x3:
			ddr3_data.drvcmd_pu  = data;
			printf("get ddr3_data.drvcmd_pu  is %x\n", ddr3_data.drvcmd_pu );
			break;
		case 0x4:
			ddr3_data.drvcmdck_pd = data;
			printf("get ddr3_data.drvcmdck_pd is %x\n", ddr3_data.drvcmdck_pd);
			break;
		case 0x5:
			ddr3_data.drvcmdck_pu = data;
			printf("get ddr3_data.drvcmdck_pu is %x\n", ddr3_data.drvcmdck_pu);
			break;
		case 0x6:
			ddr3_data.drval_pd = data;
			printf("get ddr3_data.drval_pd is %x\n", ddr3_data.drval_pd);
			break;
		case 0x7:
			ddr3_data.drvah_pu = data;
			printf("get ddr3_data.drvah_pu is %x\n", ddr3_data.drvah_pu);
			break;
        case 0x8:
            ddr3_data.dqA = data;
            printf("get ddr3_data.dqA is %x\n", ddr3_data.dqA);
            break;
        case 0x9:
            ddr3_data.vref = data;
            printf("get ddr3_data.vref is %x\n", ddr3_data.vref);
            break;
		case 0xa:
			ddr3_data.kgd_odt = data & 0x7;
			ddr3_data.kgd_ds = (data >> 3) & 0x3;
			printf("get ddr3_data.kgd_odt is %x, ddr3_data.kgd_ds is %x\n", ddr3_data.kgd_odt, ddr3_data.kgd_ds);
			break;
		default:
			printf("index not support!\n");
			break;
	}
}
*/
void get_ddr_data(int data[], int numBits)
{
	struct ddr3_16bit_de ddr_data = {0};
	int i = 0, k = 0;
	int index_0 = 0, data_0 = 0;
	int index_1 = 0, data_1 = 0;
	int index_2 = 0, data_2 = 0;
	unsigned int j = 0;
	printf("data is: ");
	for(i; i < numBits; i++)
		printf("%d ", data[i]);
	printf("\n");

	if(data[0])
	{
		printf("index mode:\n");

		for(i = 1, j = 0; j < 5; i++, j++)
			data_0 |= (data[i] & 0x1) << j;
		for(i, j = 0; j < 3; i++, j++)
			index_0 |= (data[i] & 0x1) << j;
		for(i, j = 0; j < 5; i++, j++)
			data_1 |= (data[i] & 0x1) << j;
		for(i, j = 0; j < 3; i++, j++)
			index_1 |= (data[i] & 0x1) << j;
		for(i, j = 0; j < 5; i++, j++)
			data_2 |= (data[i] & 0x1) << j;
		for(i, j = 0; j < 4; i++, j++)
			index_2 |= (data[i] & 0x1) << j;
	/*	get_index_ddr(index_0, data_0);
		get_index_ddr(index_1, data_1);
		get_index_ddr(index_2, data_2);*/
	}
	else{
		printf("normal mode:\n");
		for(i = 1, j = 0; j < (sizeof(((hanming_ddr_data*)0)->odt_pd) / 4); j++, i++)
			ddr_data.odt_pd |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->odt_pu) / 4); j++, i++)
			ddr_data.odt_pu |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvcmd_pd) / 4); j++, i++)
			ddr_data.drvcmd_pd |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvcmd_pu ) / 4); j++, i++)
			ddr_data.drvcmd_pu  |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvcmdck_pd ) / 4); j++, i++)
			ddr_data.drvcmdck_pd  |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvcmdck_pu ) / 4); j++, i++)
			ddr_data.drvcmdck_pu  |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvalah_pd ) / 4); j++, i++)
			ddr_data.drvalah_pd  |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvalah_pu ) / 4); j++, i++)
			ddr_data.drvalah_pu  |= (data[i] & 0x1) << j;
        for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvblbh_pd ) / 4); j++, i++)
			ddr_data.drvblbh_pd  |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvblbh_pu ) / 4); j++, i++)
			ddr_data.drvblbh_pu  |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->dqA ) / 4); j++, i++)
			ddr_data.dqA  |= (data[i] & 0x1) << j;
        for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->dqB ) / 4); j++, i++)
			ddr_data.dqB  |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->vref ) / 4); j++, i++)
			ddr_data.vref  |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->kgd_odt) / 4); j++, i++)
			ddr_data.kgd_odt |= (data[i] & 0x1) << j;
		for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->kgd_ds) / 4); j++, i++)
			ddr_data.kgd_ds |= (data[i] & 0x1) << j;

        printf("odt_pd %d\nodt_pu %d\ndrvcmd_pd %d\ndrvcmd_pu %d\ndrvcmdck_pd %d\ndrvcmdck_pu %d\n"\
                "drval_pd %d\ndrval_pu %d\ndqA %d\nvref %d\nkgd_odt %d\nkgd_ds %d\n",
                ddr_data.odt_pd, ddr_data.odt_pu, ddr_data.drvcmd_pd, ddr_data.drvcmd_pu, ddr_data.drvcmdck_pd, ddr_data.drvcmdck_pu,
                ddr_data.drvalah_pd,ddr_data.drvalah_pu,ddr_data.dqA,ddr_data.vref,ddr_data.kgd_odt,ddr_data.kgd_ds
                );

	}
}

int check_HammingCode(int hammingCode[], int numBits) {
	int numParityBits = check_ParityBits(numBits);
	unsigned int zero_parity, pp = 0;
	int position = 0;
	int bitMask = 0;
	int parity = 0;
	int i, j;
	int decodedData = 0;
	int dataBits[numBits];
	int dataIndex = 0;

	/*计算整体异或值，判断是否2bit错误*/
	for(int i =0; i < numBits + numParityBits; i++)
	{
		parity ^= hammingCode[i];
	}
	pp = parity ^ hammingCode[numBits + numParityBits];
	for (i = 0; i < numParityBits; i++) {
		position = (1 << i);
		bitMask = position;
		parity = 0;

		for (j = position; j <= numBits + numParityBits; j++) {
			if (j & bitMask) {
				/* 0 ^ 0 = 0; 1 ^ 0 = 1*/
				parity ^= hammingCode[j - 1];
			}
		}
		zero_parity |= parity << i;
	}

	if(zero_parity != 0)
	{
		if(pp == 0)
		{
			printf("Err 2 bit\n");
			return 0;
		}else{
			printf("Err at bit position: %d\n", zero_parity);
			hammingCode[zero_parity - 1] ^= 1;
		}
	}else{
		printf("data is right\n");
	}
	/*从汉明码中提取数据位*/
	for (i = 0, j = 0; i < numBits + numParityBits; i++) {
		if ((i + 1) == (1 << j)) {
			j++;
		}
		else {
			dataBits[dataIndex++] = hammingCode[i];
		}
	}

	for (i = 0; i < numBits; i++)
	{
		decodedData |= dataBits[i] << i;
	}
	get_ddr_data(dataBits, numBits);
	return decodedData;
}

int set_ddr_data(int data[])
{
	int i = 0;
	unsigned int j = 0;

	data[0] = 0;
	for(i = 1, j = 0; j < (sizeof(((hanming_ddr_data*)0)->odt_pd) / 4); j++, i++)
		data[i] = (ddr3_data.odt_pd >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->odt_pu) / 4); j++, i++)
		data[i] = (ddr3_data.odt_pu >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvcmd_pd) / 4); j++, i++)
		data[i] = (ddr3_data.drvcmd_pd >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvcmd_pu) / 4); j++, i++)
		data[i] = (ddr3_data.drvcmd_pu >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvcmdck_pd) / 4); j++, i++)
		data[i] = (ddr3_data.drvcmdck_pd >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvcmdck_pu) / 4); j++, i++)
		data[i] = (ddr3_data.drvcmdck_pu >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvalah_pd) / 4); j++, i++)
		data[i] = (ddr3_data.drvalah_pd >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvalah_pu) / 4); j++, i++)
		data[i] = (ddr3_data.drvalah_pu >> j) & 0x1;
    for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvblbh_pd) / 4); j++, i++)
		data[i] = (ddr3_data.drvblbh_pd >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->drvblbh_pu) / 4); j++, i++)
		data[i] = (ddr3_data.drvblbh_pu >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->dqA) / 4); j++, i++)
		data[i] = (ddr3_data.dqA >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->dqB) / 4); j++, i++)
		data[i] = (ddr3_data.dqB >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->vref) / 4); j++, i++)
		data[i] = (ddr3_data.vref >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->kgd_odt) / 4); j++, i++)
		data[i] = (ddr3_data.kgd_odt >> j) & 0x1;
	for(i, j = 0; j < (sizeof(((hanming_ddr_data*)0)->kgd_ds) / 4); j++, i++)
		data[i] = (ddr3_data.kgd_ds >> j) & 0x1;
	return 0;
}

int main(int argc, char* argv[])
{
	unsigned int data_hex = 0;
	int dataBits = sizeof(hanming_ddr_data) / 4 + 1;
	int numParityBits = check_ParityBits(dataBits);
	int hammingCode[dataBits + numParityBits + 1];

	int decodedData = 0;
	int i = 0, ret = 0;
	int data[sizeof(hanming_ddr_data) / 4] = {};
	ret = set_ddr_data(data);
	if(ret)
	{
		printf("set_ddr_data err!\n");
		return 0;
	}
	printf("2data is: ");
	for(i = 0; i < dataBits; i++)
	{
		data_hex |= data[i] << i;
		printf("%d", data[i]);
	}

	printf("\n");

	printf("\ndata_hex is %x\n", data_hex);


	/*生成汉明码*/
	creat_HammingCode(data, dataBits, hammingCode);

	printf("Hamming Code:");
	for (i = 0; i < dataBits + numParityBits +1; i++)
	{
		printf("%d", hammingCode[i]);
	}
	printf("\n");

	/*模拟数据错误*/
	hammingCode[1] = 0;
	//hammingCode[3] = 1;
	//hammingCode[11] = 0;

	printf("Errored Code:");
	for(i = 0;i < dataBits + numParityBits; i++)
	{
		printf("%d",hammingCode[i]);
	}
	printf("\n");

	printf("222dataBits:%d %d\n", dataBits,sizeof(hanming_ddr_data) / 4 + 1);




	/*检测并纠正汉明码*/
	decodedData = check_HammingCode(hammingCode, dataBits);
	printf("Decoded Data: %x\n", decodedData);

	return 0;
}
