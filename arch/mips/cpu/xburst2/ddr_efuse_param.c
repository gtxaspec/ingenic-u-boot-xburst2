/*
 * DDR driver for A1 .
 * Used by A1 ddr param set.
 *
 * Copyright (C) 2023 Ingenic Semiconductor Co.,Ltd
 * Author: Edwin <cwen@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*#define DEBUG*/
/*#define DEBUG_READ_WRITE */
//#define CONFIG_DDR_DEBUG 1

#include <config.h>
#include <common.h>
#include <linux/string.h>
#include <malloc.h>
#include <ddr/ddr_common.h>
#ifdef CONFIG_A1ALL
#include <generated/ddr_reg_values_a1all.h>
struct a1_ddr_params *pddr_params;
#else
#include <generated/ddr_reg_values.h>
#endif
#include <asm/cacheops.h>
#include <asm/dma-default.h>

#include <asm/io.h>
#include <asm/arch/clk.h>
#include "ddr_debug.h"
#define ddr_hang() do{						\
		printf("%s %d\n",__FUNCTION__,__LINE__);	\
		hang();						\
	}while(0)



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

int check_ParityBits(int dataBits) {
    int m = 1;
    while ((1 << m) < (dataBits + m + 1)) {
        m++;
    }
    return m;
}

void get_index_ddr(int index, int data,ddr3_param_t *ddr_data)
{

	switch(index)
	{
		case odt_pd:
            ddr_data->params[odt_pd] = data;
			printf("get ddr3_data.odt_pd is 0x%x\n", data);
			break;
		case odt_pu:
			ddr_data->params[odt_pu]  = data;
			printf("get ddr3_data.odt_pu is 0x%x\n", data);
			break;
		case drvcmd_pd:
			ddr_data->params[drvcmd_pd] = data;
			printf("get ddr3_data.drvcmd_pd is 0x%x\n",data);
			break;
		case drvcmd_pu:
			ddr_data->params[drvcmd_pu]  = data;
			printf("get ddr3_data.drvcmd_pu is 0x%x\n", data);
			break;
		case drvcmdck_pd:
			ddr_data->params[drvcmdck_pd] = data;
			printf("get ddr3_data.drvcmdck_pd is 0x%x\n", data);
			break;
		case drvcmdck_pu:
			ddr_data->params[drvcmdck_pu] = data;
			printf("get ddr3_data.drvcmdck_pu is 0x%x\n", data);
			break;
		case drvalah_pd:
			ddr_data->params[drvalah_pd] = data;
			printf("get ddr3_data.drvalah_pd is 0x%x\n", data);
			break;
		case drvalah_pu:
			ddr_data->params[drvalah_pu] = data;
			printf("get ddr3_data.drvah_pu is 0x%x\n", data);
			break;
        case drvblbh_pd:
            ddr_data->params[drvblbh_pd] = data;
            printf("get ddr3_data.drvblbh_pd is 0x%x\n", data);
            break;
        case drvblbh_pu:
            ddr_data->params[drvblbh_pu] = data;
            printf("get ddr3_data.drvblbh_pu is 0x%x\n", data);
            break;
		case dqA:
			ddr_data->params[dqA] = data;
			printf("get ddr3_data.dqA is 0x%x\n", data);
			break;
		case dqB:
			ddr_data->params[dqB] = data;
			printf("get ddr3_data.dqB is 0x%x\n", data);
			break;
        case vref:
            ddr_data->params[vref] = data;
			printf("get ddr3_data.vref is 0x%x\n", data);
            break;
        case kgd_odt:
            ddr_data->params[kgd_odt] = data;
            printf("get ddr3_data.kgd_odt is 0x%x\n", data);
            break;
        case kgd_ds:
            ddr_data->params[kgd_ds] = data;
            printf("get ddr3_data.kgd_ds is 0x%x\n", data);
            break;

        default:
		printf("index not support!\n");
		break;
	}
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

void get_ddr_data(int data[], int numBits ,ddr3_param_t *ddr_data,bool is16bitdw)
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

        printf("index mode\n");
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
            if(allbits > (A1_EFUSE_PARAM_HAMM_NUBBIT_32BIT - 4)){
#ifdef DEBUG_DDR_EFUSE_PARAM
                    printf("4 parms,allbits:%d,> %d\n",allbits,(A1_EFUSE_PARAM_HAMM_NUBBIT_32BIT - 4));
#endif
                    return;
            }

            for(i, j = 0; j < A1_EFUSE_INDEX_MODE_WIDTH; i++, j++)
                index_5 |= (data[i] & 0x1) << j;

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
    else{// fixed mode
        printf("fixed mode\n");

        memset(ddr_data->params, 0 ,sizeof(uint8_t)*(kgd_ds + 1)); //参数清0，dqs除外

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


int check_HammingCode(uint8_t hammingCode[], int numBits ,ddr3_param_t *ddr_data,bool is16bitdw) {
	int numParityBits = check_ParityBits(numBits);
	unsigned int zero_parity = 0, pp = 0;
	int position = 0;
	int bitMask = 0;
	int parity = 0;
	int i, j;
	int dataBits[numBits];
	int dataIndex = 0;

    if(!ddr_data || !hammingCode)
        return -1;

	/*计算整体异或值，判断是否2bit错误*/
	for(i = 0; i < numBits + numParityBits; i++)
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
	/*
		if(pp == 0)
		{
			printf("Err 2 bit\n");
			return -1;
		}else{
			printf("Err at bit position: %d\n", zero_parity);
			hammingCode[zero_parity - 1] ^= 1;
		}
	*/
        printf("ddr hammingcode error,use default param\n");
        return -1;
	}else{
		printf("ddr hammingcode right\n");
	}
#ifdef DEBUG_DDR_EFUSE_PARAM
        printf("right data is\n");
        for(i = 0; i < numBits + numParityBits; i++)
            printf("%d", hammingCode[i]);
        printf("\n");
#endif

	/*从汉明码中提取数据位*/
	for (i = 0, j = 0; i < numBits + numParityBits; i++) {
		if ((i + 1) == (1 << j)) {
			j++;
		}
		else {
			dataBits[dataIndex++] = hammingCode[i];
		}
	}

	get_ddr_data(dataBits, numBits,ddr_data,is16bitdw);
	return 0;
}

int set_ddr_data(int reg1,int reg2,uint8_t data[],int numBits,bool is16bitdw)
{
	int i = 0;
	unsigned int j = 0;
    int allbit = 0;

    if(!data)
        return -1;

#ifdef DEBUG_DDR_EFUSE_PARAM
    printf("hamming data:\n");
#endif

    for(i=0,j=32; i < 32; i++){
            data[i]=(reg1 >> --j) & 0x1;
#ifdef DEBUG_DDR_EFUSE_PARAM
            printf("%d",data[i]);
#endif
    }

    if(true == is16bitdw)
        allbit = A1_EFUSE_PARAM_HAMM_NUBBIT_16BIT;
    else
        allbit = A1_EFUSE_PARAM_HAMM_NUBBIT_32BIT;

    for(i,j=32; i < allbit; i++){
        data[i] = (reg2 >> --j) & 0x1;
#ifdef DEBUG_DDR_EFUSE_PARAM
        printf("%d",data[i]);
#endif

    }
#ifdef DEBUG_DDR_EFUSE_PARAM
    printf("\n");
#endif

    return 0;
}

int ddr_param_efuse_init(ddr3_param_t *ddr3_param_data, bool is16bitdw)
{
    int reg1 = 0,reg2 = 0,ret = 0;
	uint8_t data[A1_EFUSE_PARAM_HAMM_NUBBIT_32BIT] = {0};

    if(!ddr3_param_data)
        return -1;

    printf("ddr_param_efuse_init\n");

    reg1=___swab32(readl(A1_EFUSE_PARAM_REG0));
    reg2=___swab32(readl(A1_EFUSE_PARAM_REG1));

#ifdef DEBUG_DDR_EFUSE_PARAM
    printf("ddr_param_efuse_init reg1:%x\n",___swab32(readl(A1_EFUSE_PARAM_REG0)));
    printf("ddr_param_efuse_init reg2:%x\n",___swab32(readl(A1_EFUSE_PARAM_REG1)));
#endif

    if(reg1 == 0 && reg2 == 0){
        printf("use default param\n");
        return -1;
    }

    if(true == is16bitdw){//16bit
        set_ddr_data(reg1,reg2,data,A1_EFUSE_PARAM_HAMM_NUBBIT_16BIT,is16bitdw);
        ret=check_HammingCode(data,A1_EFUSE_PARAM_DATA_NUBBIT_16BIT+A1_EFUSE_PARAM_INDEX_MODE_BIT,ddr3_param_data,is16bitdw);
    }else{//32bit
        set_ddr_data(reg1,reg2,data,A1_EFUSE_PARAM_HAMM_NUBBIT_32BIT,is16bitdw);
        ret=check_HammingCode(data,A1_EFUSE_PARAM_DATA_NUBBIT_32BIT+A1_EFUSE_PARAM_INDEX_MODE_BIT,ddr3_param_data,is16bitdw);
    }

    if(ret){
        printf("use default param\n");
        return -2;
    }

    return 0;
}

