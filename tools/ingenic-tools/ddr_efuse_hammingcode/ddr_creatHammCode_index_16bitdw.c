#include <stdio.h>
#include <string.h>

struct ddr3_16bit {
	int index0;
	int data0;
	int index1;
	int data1 ;
	int index2;
	int data2;
	int index3;
	int data3;
};

enum  ddr3_param_enum{
	 odt_pd,
	 odt_pu,
	 drvcmd_pd,
	 drvcmd_pu,
	 drvcmdck_pd,
	 drvcmdck_pu,
	 drvalah_pd,
	 drvalah_pu,
     drvblbh_pd,
     drvblbh_pu,
	 dqA,
     dqB,
	 vref,
	 kgd_odt,
	 kgd_ds=14,

	 dqsA,
	 dqsB,
	 max_index
};

typedef struct{
    char *version;
    char params[max_index];
}ddr3_param_t;

#define DEBUG_DDR_EFUSE_PARAM
#define A1_EFUSE_PARAM_REG0 0xb3540244
#define A1_EFUSE_PARAM_REG1 0xb3540248

#define A1_EFUSE_PARAM_HAMM_NUBBIT_16BIT 52
#define A1_EFUSE_PARAM_DATA_NUBBIT_16BIT 44
#define A1_EFUSE_PARAM_INDEX_MODE_BIT 1
#define A1_EFUSE_PARAM_LAST_XOR_BIT 1

#define A1_EFUSE_PARAM_HAMM_NUBBIT_32BIT 64
#define A1_EFUSE_PARAM_DATA_NUBBIT_32BIT 56

#define A1_EFUSE_INDEX_MODE_WIDTH 4

const int A1_DDR_ODT_REMAP[16] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB, 0xD,0x18,0x1B,0x1F};

const int A1_DDR_DRVCMD_REMAP[16] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB, 0xE,0x18,0x1B,0x1F};
const int A1_DDR_DRVDATA_REMAP[16] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB, 0x14,0x18,0x1B,0x1F};


const int A1_DDR_DQ_REMAP[16] = { 5,7,9,11,13,14,15,16,17,18,19,20,21,22,24,26};
const int A1_DDR_VREF_REMAP[8] = {118,128,138,148,158,168,178,188};

/*
ddr3_param_t a1n_ddr3_param = {
    .version = "A1N@V20230825a",
    .params = {
        [odt_pd] = 1,
        [odt_pu] = 0,
        [drvcmd_pd] = 0xe,
        [drvcmd_pu] = 0xe,
        [drvcmdck_pd] = 0xe,
        [drvcmdck_pu] = 0xe,
        [drvalah_pd] = 0x14,
        [drvalah_pu] = 0x14,
        [drvblbh_pd] = 0x14,
        [drvblbh_pu] = 0x14,
        [dqA] = 0x10,
        [dqB] = 7,
        [vref] = 0x80,
        [kgd_odt] = 1,//001
        [kgd_ds] = 1,//01
        [dqsA] = 0x20,
        [dqsB] = 0x20
    }
};
const int A1_DDR_ODT_REMAP[16] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB, 0xD,0x18,0x1B,0x1F};

const int A1_DDR_DRVCMD_REMAP[16] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB, 0xE,0x18,0x1B,0x1F};
const int A1_DDR_DRVDATA_REMAP[16] = {0,1,2,3,4,5,6,7,8,9,0xA,0xB, 0x14,0x18,0x1B,0x1F};


const int A1_DDR_DQ_REMAP[16] = { 5,7,9,12,13,14,15,16,17,18,19,20,21,22,24,26};//{7,12,14,16,18,20,22,24};
const int A1_DDR_VREF_REMAP[8] = {118,128,138,148,158,168,178,188}; // 定一个中间值，左右偏。

*/

struct ddr3_16bit ddr3_data = {
	.index0 = 10, //odt_pd
 	.data0 = 22,
    .index1 = 0, //cmdck pd
    .data1 = 0,
	.index2 = 0,
 	.data2 = 0,
    .index3 = 0,
    .data3 = 0,

};


ddr3_param_t a1n_ddr3_param = {
    .version = "A1N@V20230825a",
    .params = {
        [odt_pd] = 1,
        [odt_pu] = 0,
        [drvcmd_pd] = 0xe,
        [drvcmd_pu] = 0xe,
        [drvcmdck_pd] = 0xe,
        [drvcmdck_pu] = 0xe,
        [drvalah_pd] = 0x14,
        [drvalah_pu] = 0x14,
        [drvblbh_pd] = 0x14,
        [drvblbh_pu] = 0x14,
        [dqA] = 0x10,
        [dqB] = 7,
        [vref] = 0x80,
        [kgd_odt] = 1,//001
        [kgd_ds] = 1,//01
        [dqsA] = 0x20,
        [dqsB] = 0x20
    }
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



 void get_index_ddr(int index, int data,ddr3_param_t *ddr_data)
 {

     switch(index)
     {
         case odt_pd:
             ddr_data->params[odt_pd] = data;
             printf("get ddr3_data.odt_pd is %#x\n", data);
             break;
         case odt_pu:
             ddr_data->params[odt_pu]  = data;
             printf("get ddr3_data.odt_pu is %#x\n", data);
             break;
         case drvcmd_pd:
             ddr_data->params[drvcmd_pd] = data;
             printf("get ddr3_data.drvcmd_pd is %#x\n",data);
             break;
         case drvcmd_pu:
             ddr_data->params[drvcmd_pu]  = data;
             printf("get ddr3_data.drvcmd_pu  is %#x\n", data);
             break;
         case drvcmdck_pd:
             ddr_data->params[drvcmdck_pd] = data;
             printf("get ddr3_data.drvcmdck_pd is %#x\n", data);
             break;
         case drvcmdck_pu:
             ddr_data->params[drvcmdck_pu] = data;
             printf("get ddr3_data.drvcmdck_pu is %#x\n", data);
             break;
         case drvalah_pd:
             ddr_data->params[drvalah_pd] = data;
             printf("get ddr3_data.drvalah_pd is %#x\n", data);
             break;
         case drvalah_pu:
             ddr_data->params[drvalah_pu] = data;
             printf("get ddr3_data.drvah_pu is %#x\n", data);
             break;
         case drvblbh_pd:
             ddr_data->params[drvblbh_pd] = data;
             printf("get ddr3_data.drvblbh_pd is %#x\n", data);
             break;
         case drvblbh_pu:
             ddr_data->params[drvblbh_pu] = data;
             printf("get ddr3_data.drvblbh_pu is %#x\n", data);
             break;
         case dqA:
             ddr_data->params[dqA] = data;
             printf("get ddr3_data.dqA is %#x\n", data);
             break;
         case dqB:
             ddr_data->params[dqB] = data;
             printf("get ddr3_data.dqB is %#x\n", data);
             break;
         case vref:
             ddr_data->params[vref] = data;
             printf("get ddr3_data.vref is %#x\n", data);
             break;
         case kgd_odt:
             ddr_data->params[kgd_odt] = data;
             printf("get ddr3_data.kgd_odt is %#x\n", data);
             break;
         case kgd_ds:
             ddr_data->params[kgd_ds] = data;
             printf("get ddr3_data.kgd_ds is %#x\n", data);
             break;

         default:
         printf("index not support!\n");
         break;
     }
 }

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


void get_ddr_data(int data[], int numBits ,ddr3_param_t *ddr_data,int is16bitdw)
{
    int i = 0, k = 0 ,allbits = 0;
    int index_0 = 0, data_0 = 0, data0width = 0;
    int index_1 = 0, data_1 = 0, data1width = 0;
    int index_2 = 0, data_2 = 0, data2width = 0;
    int index_3 = 0, data_3 = 0, data3width = 0;
    int index_4 = 0, data_4 = 0, data4width = 0;
    int index_5 = 0, data_5 = 0, data5width = 0;
    unsigned int j = 0;

#ifdef DEBUG_DDR_EFUSE_PARAM
    printf("pure data is:\n");
    for(i; i < numBits; i++)
        printf("%d", data[i]);
    printf("\n");
#endif

    if(!ddr_data || !data)
        return;

    if(data[0])//index mode
    {

#ifdef DEBUG_DDR_EFUSE_PARAM
        printf("index mode:\n");
#endif

         if(is16bitdw){//16bit dw
            for(i = 1, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                index_0 |= (data[i] & 0x1) << j;
            data0width = get_param_width(index_0);
            allbits = A1_EFUSE_INDEX_MODE_WIDTH + data0width;
            for(i, j = 0; j < data0width; i++, j++)
                data_0 |= (data[i] & 0x1) << j;
            get_index_ddr(index_0, data_0,ddr_data);

            for(i, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                index_1 |= (data[i] & 0x1) << j;
            if(!index_1)
                goto END;

            data1width = get_param_width(index_1);
            allbits += A1_EFUSE_INDEX_MODE_WIDTH + data1width;
            for(i, j = 0; j < data1width; i++, j++)
                data_1 |= (data[i] & 0x1) << j;
            get_index_ddr(index_1, data_1,ddr_data);

            for(i, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                index_2 |= (data[i] & 0x1) << j;
            if(!index_2)
                goto END;
            data2width = get_param_width(index_2);
            for(i, j = 0; j < data2width; i++, j++)
                data_2 |= (data[i] & 0x1) << j;
            get_index_ddr(index_2, data_2,ddr_data);

            allbits += A1_EFUSE_INDEX_MODE_WIDTH + data4width;
            if(allbits > (A1_EFUSE_PARAM_HAMM_NUBBIT_16BIT - 4))
                return;

            for(i, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                 index_3 |= (data[i] & 0x1) << j;

            if(!index_3 )
                return;

            data3width = get_param_width(index_3);
            for(i, j = 0; j < data3width; i++, j++)
                data_3 |= (data[i] & 0x1) << j;
            get_index_ddr(index_3, data_3,ddr_data);

         }else{//32bit dw
                for(i = 1, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                    index_0 |= (data[i] & 0x1) << j;
                data0width = get_param_width(index_0);
                allbits = A1_EFUSE_INDEX_MODE_WIDTH + data0width;
                for(i, j = 0; j < data0width; i++, j++)
                    data_0 |= (data[i] & 0x1) << j;
                get_index_ddr(index_0, data_0,ddr_data);

                for(i, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                    index_1 |= (data[i] & 0x1) << j;
                if(!index_1)
                    goto END;
                data1width = get_param_width(index_1);
                allbits += A1_EFUSE_INDEX_MODE_WIDTH + data1width;
                for(i, j = 0; j < data1width; i++, j++)
                    data_1 |= (data[i] & 0x1) << j;
                get_index_ddr(index_1, data_1,ddr_data);


                for(i, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                    index_2 |= (data[i] & 0x1) << j;
                if(!index_2)
                    goto END;
                data2width = get_param_width(index_2);
                allbits += A1_EFUSE_INDEX_MODE_WIDTH + data2width;
                for(i, j = 0; j < data2width; i++, j++)
                    data_2 |= (data[i] & 0x1) << j;
                get_index_ddr(index_2, data_2,ddr_data);

                for(i, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                    index_3 |= (data[i] & 0x1) << j;
                if(!index_3)
                    goto END;
                data3width = get_param_width(index_3);
                allbits += A1_EFUSE_INDEX_MODE_WIDTH + data3width;
                for(i, j = 0; j < data3width; i++, j++)
                    data_3 |= (data[i] & 0x1) << j;
                get_index_ddr(index_3, data_3,ddr_data);

                for(i, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                    index_4 |= (data[i] & 0x1) << j;
                if(!index_4)
                    goto END;
                data4width = get_param_width(index_4);

                for(i, j = 0; j < data4width; i++, j++)
                    data_4 |= (data[i] & 0x1) << j;
                get_index_ddr(index_4, data_4,ddr_data);

                allbits += A1_EFUSE_INDEX_MODE_WIDTH + data4width;
                if(allbits > (A1_EFUSE_PARAM_HAMM_NUBBIT_32BIT - 4))
                    {
#ifdef DEBUG_DDR_EFUSE_PARAM
                        printf("4 parms,allbits:%d,> %d\n",allbits,(A1_EFUSE_PARAM_HAMM_NUBBIT_32BIT - 4));
#endif
                        return;
                }

                for(i, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                    index_5 |= (data[i] & 0x1) << j;
                printf("index_5:%d\n",index_5);

                if(!index_5 )
                    return;

                data5width = get_param_width(index_5);
                allbits += A1_EFUSE_INDEX_MODE_WIDTH + data5width;
                for(i, j = 0; j < data5width; i++, j++)
                    data_5 |= (data[i] & 0x1) << j;
                get_index_ddr(index_5, data_5,ddr_data);


         }

END:
    ;
#ifdef DEBUG_DDR_EFUSE_PARAM
     printf("odt_pd %x\nodt_pu %x\ndrvcmd_pd %x\ndrvcmd_pu %x\ndrvcmdck_pd %x\ndrvcmdck_pu %x\n",
             ddr_data->params[odt_pd], ddr_data->params[odt_pu], ddr_data->params[drvcmd_pd], \
             ddr_data->params[drvcmd_pu], ddr_data->params[drvcmdck_pd], ddr_data->params[drvcmdck_pu]
             );

     printf("drvalah_pd %x\ndrvalah_pu %x\ndqA %x\nvref %x\nkgd_odt %x\nkgd_ds %x\ndrvblbh_pd %x\ndrvblbh_pu %x\ndqB %x\ndqsA %x\n\n",
             ddr_data->params[drvalah_pd],ddr_data->params[drvalah_pu],ddr_data->params[dqA],\
             ddr_data->params[vref],ddr_data->params[kgd_odt],ddr_data->params[kgd_ds],\
             ddr_data->params[drvblbh_pd],ddr_data->params[drvblbh_pu],ddr_data->params[dqB],\
             ddr_data->params[dqsA]
             );
#endif

    }
    else{// normal mode
        memset(ddr_data->params, 0 ,sizeof(unsigned char)*(kgd_ds + 1)); //参数清0，dqs除外

#ifdef DEBUG_DDR_EFUSE_PARAM
         printf("odt_pd %x\nodt_pu %x\ndrvcmd_pd %x\ndrvcmd_pu %x\ndrvcmdck_pd %x\ndrvcmdck_pu %x\n",
                 ddr_data->params[odt_pd], ddr_data->params[odt_pu], ddr_data->params[drvcmd_pd], \
                 ddr_data->params[drvcmd_pu], ddr_data->params[drvcmdck_pd], ddr_data->params[drvcmdck_pu]
                 );

         printf("drvalah_pd %x\ndrvalah_pu %x\ndqA %x\nvref %x\nkgd_odt %x\nkgd_ds %x\ndrvblbh_pd %x\ndrvblbh_pu %x\ndqB %x\ndqsA %x\n\n",
                 ddr_data->params[drvalah_pd],ddr_data->params[drvalah_pu],ddr_data->params[dqA],\
                 ddr_data->params[vref],ddr_data->params[kgd_odt],ddr_data->params[kgd_ds],\
                 ddr_data->params[drvblbh_pd],ddr_data->params[drvblbh_pu],ddr_data->params[dqB],\
                 ddr_data->params[dqsA]
                 );

#endif


        for(i = 1, j = 0; j < 4; j++, i++)
            ddr_data->params[odt_pd] |= (data[i] & 0x1) << j;
        for(i, j = 0; j < 4; j++, i++)
            ddr_data->params[odt_pu] |= (data[i] & 0x1) << j;
        for(i, j = 0; j < 4; j++, i++)
            ddr_data->params[drvcmd_pd] |= (data[i] & 0x1) << j;
        for(i, j = 0; j < 4; j++, i++)
            ddr_data->params[drvcmd_pu]  |= (data[i] & 0x1) << j;
        for(i, j = 0; j < 4; j++, i++)
            ddr_data->params[drvcmdck_pd]  |= (data[i] & 0x1) << j;
        for(i, j = 0; j < 4; j++, i++)
            ddr_data->params[drvcmdck_pu]  |= (data[i] & 0x1) << j;
        for(i, j = 0; j < 4; j++, i++)
            ddr_data->params[drvalah_pd]  |= (data[i] & 0x1) << j;
        for(i, j = 0; j < 4; j++, i++)
            ddr_data->params[drvalah_pu]  |= (data[i] & 0x1) << j;

        if(is16bitdw){
            ddr_data->params[drvblbh_pd] = 12;
            ddr_data->params[drvblbh_pu] = 12;
        }else{
            for(i, j = 0; j < 4; j++, i++)
                ddr_data->params[drvblbh_pd]  |= (data[i] & 0x1) << j;
            for(i, j = 0; j < 4; j++, i++)
                ddr_data->params[drvblbh_pu]  |= (data[i] & 0x1) << j;
        }

        for(i, j = 0; j < 4; j++, i++)
            ddr_data->params[dqA]  |= (data[i] & 0x1) << j;

        if(is16bitdw){
            ddr_data->params[dqB] = 1;
        }else{
            for(i, j = 0; j < 4; j++, i++)
                ddr_data->params[dqB]  |= (data[i] & 0x1) << j;
        }
        for(i, j = 0; j < 3; j++, i++)
            ddr_data->params[vref]  |= (data[i] & 0x1) << j;
        for(i, j = 0; j < 3; j++, i++)
            ddr_data->params[kgd_odt] |= (data[i] & 0x1) << j;
        for(i, j = 0; j < 2; j++, i++)
            ddr_data->params[kgd_ds] |= (data[i] & 0x1) << j;

#ifdef DEBUG_DDR_EFUSE_PARAM
         printf("odt_pd %x\nodt_pu %x\ndrvcmd_pd %x\ndrvcmd_pu %x\ndrvcmdck_pd %x\ndrvcmdck_pu %x\n",
                 ddr_data->params[odt_pd], ddr_data->params[odt_pu], ddr_data->params[drvcmd_pd], \
                 ddr_data->params[drvcmd_pu], ddr_data->params[drvcmdck_pd], ddr_data->params[drvcmdck_pu]
                 );

         printf("drvalah_pd %x\ndrvalah_pu %x\ndqA %x\nvref %x\nkgd_odt %x\nkgd_ds %x\ndrvblbh_pd %x\ndrvblbh_pu %x\ndqB %x\ndqsA %x\n\n",
                 ddr_data->params[drvalah_pd],ddr_data->params[drvalah_pu],ddr_data->params[dqA],\
                 ddr_data->params[vref],ddr_data->params[kgd_odt],ddr_data->params[kgd_ds],\
                 ddr_data->params[drvblbh_pd],ddr_data->params[drvblbh_pu],ddr_data->params[dqB],\
                 ddr_data->params[dqsA]
                 );

#endif

        ddr_data->params[odt_pd] = A1_DDR_ODT_REMAP[ddr_data->params[odt_pd]];
        ddr_data->params[odt_pu] = A1_DDR_ODT_REMAP[ddr_data->params[odt_pu]];
        ddr_data->params[drvcmd_pd] = A1_DDR_DRVCMD_REMAP[ddr_data->params[drvcmd_pd]];
        ddr_data->params[drvcmd_pu] = A1_DDR_DRVCMD_REMAP[ddr_data->params[drvcmd_pu]];
        ddr_data->params[drvcmdck_pd] = A1_DDR_DRVCMD_REMAP[ddr_data->params[drvcmdck_pd]];
        ddr_data->params[drvcmdck_pu] = A1_DDR_DRVCMD_REMAP[ddr_data->params[drvcmdck_pu]];
        ddr_data->params[drvalah_pd] = A1_DDR_DRVDATA_REMAP[ddr_data->params[drvalah_pd]];
        ddr_data->params[drvalah_pu] = A1_DDR_DRVDATA_REMAP[ddr_data->params[drvalah_pu]];
        ddr_data->params[drvblbh_pd] = A1_DDR_DRVDATA_REMAP[ddr_data->params[drvblbh_pd]];
        ddr_data->params[drvblbh_pu] = A1_DDR_DRVDATA_REMAP[ddr_data->params[drvblbh_pu]];
        ddr_data->params[dqA] = A1_DDR_DQ_REMAP[ddr_data->params[dqA]];
        ddr_data->params[dqB] = A1_DDR_DQ_REMAP[ddr_data->params[dqB]];
        ddr_data->params[vref] = A1_DDR_VREF_REMAP[ddr_data->params[vref]];

#ifdef DEBUG_DDR_EFUSE_PARAM
        printf("odt_pd %x\nodt_pu %x\ndrvcmd_pd %x\ndrvcmd_pu %x\ndrvcmdck_pd %x\ndrvcmdck_pu %x\n",
                ddr_data->params[odt_pd], ddr_data->params[odt_pu], ddr_data->params[drvcmd_pd], \
                ddr_data->params[drvcmd_pu], ddr_data->params[drvcmdck_pd], ddr_data->params[drvcmdck_pu]
                );

        printf("drvalah_pd %x\ndrvalah_pu %x\ndqA %x\nvref %x\nkgd_odt %x\nkgd_ds %x\ndrvblbh_pd %x\ndrvblbh_pu %x\ndqB %x\ndqsA %x\n\n",
                ddr_data->params[drvalah_pd],ddr_data->params[drvalah_pu],ddr_data->params[dqA],\
                ddr_data->params[vref],ddr_data->params[kgd_odt],ddr_data->params[kgd_ds],\
                ddr_data->params[drvblbh_pd],ddr_data->params[drvblbh_pu],ddr_data->params[dqB],\
                ddr_data->params[dqsA]
                );

#endif
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
	get_ddr_data(dataBits, numBits,&a1n_ddr3_param,1);
	return decodedData;
}

int get_param_width(int index)
{
    int width = -1;

	switch(index)
	{
		case odt_pd:
		case odt_pu:
		case drvcmd_pd:
		case drvcmd_pu:
		case drvcmdck_pd:
		case drvcmdck_pu:
		case drvalah_pd:
		case drvalah_pu:
        case drvblbh_pd:
        case drvblbh_pu:
            width = 5;
            break;
		case dqA:
		case dqB:
            width = 6;
			break;
        case vref:
            width = 8;
            break;
        case kgd_odt:
            width = 3;
            break;
        case kgd_ds:
            width = 2;
            break;

        default:
		    printf("param not support!\n");
		break;
	}
#ifdef DEBUG_DDR_EFUSE_PARAM
        printf("index:%d, width:%d\n",index,width);
#endif

    return width;
}

int set_ddr_data(int data[])
{
	int i = 0;
	unsigned int j = 0;

	data[0] = 1;
	for(i = 1, j = 0; j < 4; j++, i++)
		data[i] = (ddr3_data.index0 >> j) & 0x1;
	for(i, j = 0; j < get_param_width(ddr3_data.index0); j++, i++)
		data[i] = (ddr3_data.data0 >> j) & 0x1;

    if(ddr3_data.index1 == 0)
        return 0;
	for(i, j = 0; j < 4; j++, i++)
		data[i] = (ddr3_data.index1 >> j) & 0x1;
	for(i, j = 0; j < get_param_width(ddr3_data.index1); j++, i++)
		data[i] = (ddr3_data.data1 >> j) & 0x1;

    if(ddr3_data.index2 == 0)
        return 0;
	for(i, j = 0; j < 4; j++, i++)
		data[i] = (ddr3_data.index2 >> j) & 0x1;
	for(i, j = 0; j < get_param_width(ddr3_data.index2); j++, i++)
		data[i] = (ddr3_data.data2 >> j) & 0x1;

    if(ddr3_data.index3 == 0)
        return 0;
    for(i, j = 0; j < 4; j++, i++)
		data[i] = (ddr3_data.index3 >> j) & 0x1;
	for(i, j = 0; j < get_param_width(ddr3_data.index3); j++, i++)
		data[i] = (ddr3_data.data3 >> j) & 0x1;

	return 0;
}


int main(int argc, char* argv[])
{
	unsigned int data_hex = 0;
	int dataBits = A1_EFUSE_PARAM_DATA_NUBBIT_16BIT + 1;
	int numParityBits = check_ParityBits(dataBits);
	int hammingCode[dataBits + numParityBits + 1];

	int decodedData = 0;
	int i = 0, ret = 0;
	int data[sizeof(hanming_ddr_data) / 4] = {};

    memset(data, 0, sizeof(hanming_ddr_data) / 4);

	ret = set_ddr_data(data);
	if(ret)
	{
		printf("set_ddr_data err!\n");
		return 0;
	}
	printf("16bit index data %d is: ",dataBits);
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

#if 0
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
#endif
	return 0;
}
