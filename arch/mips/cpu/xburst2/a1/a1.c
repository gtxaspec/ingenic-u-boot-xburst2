/*
 * A1 common routines
 *
 * Copyright (c) 2015 Ingenic Semiconductor Co.,Ltd
 * Author: Elvis <huan.wang@ingenic.com>
 * Based on: arch/mips/cpu/xburst/jz4775/jz4775.c
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

/* #define DEBUG */
#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/clk.h>
#include <asm/arch/cpm.h>
#include <asm/arch/tcu.h>
#include <spl.h>
#include <asm/cacheops.h>
#include <asm/dma-default.h>
#ifdef CONFIG_A1ALL
#include <generated/ddr_reg_values_a1all.h>
#include <linux/string.h>
#endif

#ifdef CONFIG_BIG_SPL
#include "testcode.c"
#endif

//#define DDR_MEM_TEST

#ifdef CONFIG_SPL_BUILD

/* Pointer to as well as the global data structure for SPL */
DECLARE_GLOBAL_DATA_PTR;
gd_t gdata __attribute__ ((section(".data")));

#ifndef CONFIG_BURNER
struct global_info ginfo __attribute__ ((section(".data"))) = {
	.extal		= CONFIG_SYS_EXTAL,
#ifndef CONFIG_A1ALL
	.cpufreq	= CONFIG_SYS_CPU_FREQ,
	.ddrfreq	= CONFIG_SYS_MEM_FREQ,
#endif
	.uart_idx	= CONFIG_SYS_UART_INDEX,
	.baud_rate	= CONFIG_BAUDRATE,
};
#endif

extern void pll_init(void);
extern void sdram_init(void);
extern void validate_cache(void);

#ifdef DDR_MEM_TEST
static void ddr_mem_pdma_test(void)
{
#if 0
	{
#define PHY_BASE						(0xb3011000)
#define REG_INNOPHY_REG46				(PHY_BASE + 0x118)
#define REG_INNOPHY_REG56				(PHY_BASE + 0x158)
		printf("ddr dma training start\n");
		int m = 0;
		int i = 0;
		int test_ok = 1;
		int test_result_old = 0;
		int win_start = 1;
		int win_end = 1;
		volatile unsigned int *uncacheaddr = (0xa0000000);
		volatile unsigned int *tmpaddr;
		unsigned int data = 0;
		unsigned int rdata = 0;
		//open pdma clock gate
		unsigned int gate0 = *(volatile unsigned int *)0xb0000020;
		gate0 &= ~0x400000;
		*(volatile unsigned int *)0xb0000020 = gate0;
		for (m = 0; m < 128; m++) {

			test_ok = 1;

			//*(volatile unsigned int *)REG_INNOPHY_REG46 = m;
			//*(volatile unsigned int *)REG_INNOPHY_REG56 = m;

			tmpaddr = uncacheaddr;
			for (i = 0; i < 1024*1024; i++) {
				data = (i%2)? 0x5a5a5a00:0xa5a5a500;
				data |= m;
				*((volatile unsigned int *)(tmpaddr)) = data;
				tmpaddr++;
			}

			pdma_test_transfer(0, 0x0, 0x400000, 0x1000);

			tmpaddr = uncacheaddr+0x400000/4;
			for (i = 0; i < 1024*1024; i++) {
				data = (i%2)? 0x5a5a5a00:0xa5a5a500;
				data |= m;
				rdata = *((volatile unsigned int *)(tmpaddr));
				if (data != rdata) {
					printf("error  : addr = 0x%x ,data = 0x%x, want = 0x%x\n", tmpaddr, rdata, data);
					test_ok = 0;
					if (1 == test_result_old) {
						test_result_old = 0;
						win_end = m;
					}
					break;
				} else {
					;//printf("correct: addr = 0x%x ,data = 0x%x\n", tmpaddr, data);
				}
				tmpaddr++;
			}
			if ((0 == test_result_old)&&(1 == test_ok)) {
				test_result_old = 1;
				win_start = m;
			}
			//printf("CONFIG: REG46 = 0x%x ,REG56 = 0x%x, %s\n",
			//		*(volatile unsigned int *)REG_INNOPHY_REG46, *(volatile unsigned int *)REG_INNOPHY_REG56,
			//		test_ok?"pass":"failed");
		}
		//printf("CONFIG: win_start = 0x%x, win_end = 0x%x, dqsmask = 0x%x\n", win_start, win_end, (win_start+win_end)/2);

		//*(volatile unsigned int *)REG_INNOPHY_REG46 = (win_start+win_end)/2;
		//*(volatile unsigned int *)REG_INNOPHY_REG56 = (win_start+win_end)/2;
	}
#endif

}

static void ddr_mem_test1(void)
{
	int i = 0;
	int q;
	unsigned int ddbuf[8] = {0};
	unsigned int addr = 0xa0000000;
	unsigned int value = 0xa55a1234;
	printf("mem test start!\n");
	for(i = 0; i < 64 * 1024 * 1024; i += 8) {
		*(volatile unsigned int *)(addr + (i + 0) * 4) = value;
		*(volatile unsigned int *)(addr + (i + 1) * 4) = value;
		*(volatile unsigned int *)(addr + (i + 2) * 4) = value;
		*(volatile unsigned int *)(addr + (i + 3) * 4) = value;
		*(volatile unsigned int *)(addr + (i + 4) * 4) = value;
		*(volatile unsigned int *)(addr + (i + 5) * 4) = value;
		*(volatile unsigned int *)(addr + (i + 6) * 4) = value;
		*(volatile unsigned int *)(addr + (i + 7) * 4) = value;
	}

	printf("mem test start check\n");
	for(i = 0; i < 64 * 1024 * 1024; i += 8) {
		int error = 0;
		ddbuf[0] = *(volatile unsigned int *)(addr + (i + 0) * 4);
		ddbuf[1] = *(volatile unsigned int *)(addr + (i + 1) * 4);
		ddbuf[2] = *(volatile unsigned int *)(addr + (i + 2) * 4);
		ddbuf[3] = *(volatile unsigned int *)(addr + (i + 3) * 4);
		ddbuf[4] = *(volatile unsigned int *)(addr + (i + 4) * 4);
		ddbuf[5] = *(volatile unsigned int *)(addr + (i + 5) * 4);
		ddbuf[6] = *(volatile unsigned int *)(addr + (i + 6) * 4);
		ddbuf[7] = *(volatile unsigned int *)(addr + (i + 7) * 4);
		for(q = 0; q < 8; q++) {
			if ((ddbuf[q]) != (value)) {
				printf("mem test addr 0x%x  error want 0x%x get 0x%x\n",(addr + (i + q) * 4), value, ddbuf[q]);
#if 0
				u32 retrycnt;
				for (retrycnt = 0; retrycnt < 100; retrycnt++) {
					ddbuf[q] = *(volatile unsigned int *)(addr + (i + q) * 4);
					if ((ddbuf[q]) != (value)) {
						printf("retry %d mem test addr 0x%x  error want 0x%x get 0x%x\n",retrycnt, (addr + (i + q) * 4), value, ddbuf[q]);
					}
				}
#endif
				error++;
			}
		}
#if 0
		if (error) {
			u32 retrycnt;
			for (retrycnt = 0; retrycnt < 100; retrycnt++) {
				printf("retry %d mem test start check\n", retrycnt);
				for(i = 0; i < 16 * 1024 * 1024; i += 8) {
					int error = 0;
					ddbuf[0] = *(volatile unsigned int *)(addr + (i + 0) * 4);
					ddbuf[1] = *(volatile unsigned int *)(addr + (i + 1) * 4);
					ddbuf[2] = *(volatile unsigned int *)(addr + (i + 2) * 4);
					ddbuf[3] = *(volatile unsigned int *)(addr + (i + 3) * 4);
					ddbuf[4] = *(volatile unsigned int *)(addr + (i + 4) * 4);
					ddbuf[5] = *(volatile unsigned int *)(addr + (i + 5) * 4);
					ddbuf[6] = *(volatile unsigned int *)(addr + (i + 6) * 4);
					ddbuf[7] = *(volatile unsigned int *)(addr + (i + 7) * 4);
					for(q = 0; q < 8; q++) {
						if ((ddbuf[q]&0xffff) != (value&0xffff)) {
							printf("retry %d mem test addr 0x%x  error want 0x%x get 0x%x\n", retrycnt, (addr + (i + q) * 4), value, ddbuf[q]);
							error++;
						}
					}
				}
			}
		}
#endif
	}
	printf("mem test end\n");
}

static void ddr_mem_test2(void)
{
	volatile u32 tmp = 0,tmp1 = 0,tmp2 = 0;
	u32 data = 0,data1=0;
	u32 reg41,reg42;
	printf("mem test2 start!\n");
	for (tmp = 0xa0000000; tmp < 0xa0000000 + 0xf000000; tmp+=4) {
		u32 i = 0;
		u32 j = 0;
		u32 td = 0x5678f0f0;
		for (i = 0; i < 1; i++) {
			*(u32*)tmp = td;
			u32 data_tmp = 0;
			u32 reg;
			data = *(u32*)tmp;
			if ((data&0xffff) != (td&0xffff)) {
				printf("\n##### add(%d) = %p, want = %x, get = %x, error = %x\n", i, tmp, td, data, (td^data));
				data_tmp = data;
				for (j = 0; j < 1000; j++) {
					data = *(u32*)tmp;
					if ((data&0xffff) == (td&0xffff)) {
						printf("\n retry read correct idx = %d\n", j);
					} else {
						if ((data&0xffff) != (data_tmp&0xffff)) {
							printf("\n retry read other error idx = %d, get = %x, error = %x\n", j, data, (td^data));
							data_tmp = data;
						}
					}
				}
			}
			td = ((td << 4) | (td >> (32 -4)));
		}
		//printf("#");
	}
	printf("mem test2 end!\n");
}

static void ddr_mem_test3(void)
{
	int i = 0;
	int q;
	unsigned int addr = 0xa0000000;
	unsigned int value = 0xf0f01234;
	unsigned int value_get;
	printf("mem test3 start1!\n");
	for(i = 0; i < 128 * 1024 * 1024; i += 64) {
		*(volatile unsigned int *)(addr + i) = value;
		value_get = *(volatile unsigned int *)(addr + i);
		if ((value&0xffff) != (value_get&0xffff)) {
			printf("mem test3-1 addr 0x%x  error want 0x%x get 0x%x\n", addr+i, value, value_get);
		}

	}
	printf("mem test3 start2!\n");
	u32 seed = 3;
	for(i = 1; i < 64*1000000; i++) {
		seed = seed*7;
		addr = ((seed&(~3))&0x7ffffff)|0xa0000000;
		*(volatile unsigned int *)(addr) = value;
		value_get = *(volatile unsigned int *)(addr);
		if ((value&0xffff) != (value_get&0xffff)) {
			printf("mem test3-2 addr 0x%x  error want 0x%x get 0x%x\n", addr, value, value_get);
		}
	}
	printf("mem test3 end!\n");
}

static void ddr_mem_test6(void)
{
	int i = 0;
	int q;
	unsigned int addr = 0xa1000000;
	unsigned int value = 0x5a5aa5a5;
	unsigned int value_get;
	printf("mem test6 start!\n");
	for(i = 0; i < 16 * 1024 * 1024; i += 4) {
		value = addr+i;
		*(volatile unsigned int *)(addr + i) = value;
		value_get = *(volatile unsigned int *)(addr + i);
		if ((value) != (value_get)) {
			printf("mem test6-1 addr 0x%x  want 0x%x get 0x%x error 0x%x \n", addr+i, value, value_get, value^value_get);
		}

	}
	printf("mem test6 end!\n");
}

static void ddr_mem_test7(void)
{
	int i = 0;
	int j = 0;
#define DATA_0 0x0
	u32 data[16] = {0x1aabcdc, 0x47192832, 0xdabadc3, 0x38237801, 0x51098376, 0x42345635, 0xad1424a4, 0x53683462,0x23678433, 0x13562453, 0x12425123, 0x45631434, 0x17894432, 0x98098532, 0x16873582, 0x28375923};
	unsigned int addr = 0x81000000;
	unsigned int value_get;
	printf("mem test7 start!\n");
	for(i = 0; i < 10000; i++) {
#if 0
		for (j = 0; j < 16; j++) {
			data[j] = 0xa5a5*j*i;
		}
#endif
		addr = 0x81000000;
		for (j = 0; j < 16; j++) {
			*(volatile unsigned int *)(addr+(j*4)) = data[j];
		}
		//flush cache
		flush_dcache_range(addr, addr+(16*4));
		flush_l2cache_range(addr, addr+(16*4));
#if 1
		addr = 0xa1000000;
		for (j = 0; j < 16; j++) {
			value_get = *(volatile unsigned int *)(addr+(j*4));
			if (value_get != data[j]) {
				printf("mem test7-1 test idx %d, addr 0x%x  error want 0x%x get 0x%x\n", i, addr+(j*4), data[j], value_get);
			}
		}
#endif
	}
	printf("mem test7 end!\n");
}
static void ddr_mem_test8(void)
{
	int i = 0;
	int j = 0;
#define DATA_0 0x0
	u32 data[16] = {0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff,0x00000000, 0xffffffff, 0xffffffff, DATA_0, DATA_0, 0xffffffff, DATA_0, 0xffffffff};
	unsigned int addr = 0x81000000;
	unsigned int value_get;
	printf("mem test8 start!\n");
	for(i = 0; i < 100000000; i++) {
#if 0
		for (j = 0; j < 16; j++) {
			data[j] = 0xa5a5*j*i;
		}
#endif
		//for (j = 0; j < 16; j++) {
		*(volatile unsigned int *)(0xc+(16*4)) = data[j];
		//}
		//flush cache
		//flush_dcache_range(addr, 0xc);
		//flush_l2cache_range(addr, 0xc);
#if 1
		//for (j = 0; j < 16; j++) {
		value_get = *(volatile unsigned int *)(0xc+(16*4));
		if (value_get != data[j]) {
			printf("mem test8-1 test idx %d, addr 0x%x  error want 0x%x get 0x%x\n", i, 0xc+(16*4), data[j], value_get);
		}
		//}
#endif
	}
	printf("mem test7 end!\n");
}
static void ddr_mem_test5(void)
{
	int i = 0;
	int j = 0;
	int q;
	unsigned int addr = 0xa0001104;
	unsigned int value = 0xa55a1234;
	unsigned int value_get;
	printf("mem test5 start!\n");
	for(i = 0; i < 100000000; i++) {
		*(volatile unsigned int *)(addr) = value;
		value_get = *(volatile unsigned int *)(addr);
		if ((value&0xffff) != (value_get&0xffff)) {
			u32 data_tmp = value_get;
			printf("mem test5 addr 0x%x  error want 0x%x get 0x%x, error = %x\n", addr, value, value_get, value^value_get);
#if 0
			for (j = 0; j < 10; j++) {
				u32 data;
				data = *(u32*)(addr);
				if ((data&0xffff) == (value&0xffff)) {
					printf("test5 retry read correct idx = %d\n", j);
				} else {
					if ((data&0xffff) != (data_tmp&0xffff)) {
						printf("test5 retry read other error idx = %d, get = %x, error = %x\n", j, data, (value^data));
						data_tmp = data;
					}
				}
			}
#endif
		}
	}
	printf("mem test5 end!\n");
}

static void ddr_mem_test4(void)
{
	unsigned int  addr;
	unsigned int  Wdata;
	unsigned int  Rdata;
	unsigned int  wrong_addr;
	unsigned int  i,j,n;
	unsigned int  fail_flag;

	addr = 	0x0;
	fail_flag = 0;
	wrong_addr = 0;
	n = 0;
	printf("mem test4 start1!\n");
	////////////////write and read////////////////
	printf("Start write and read!!\n ");
	for(i=0;i<1;i++){
		j = 0 ;
		for(addr=0xa0000000;addr<0xaffffff0;addr = addr+4){
			Wdata = ~addr+(addr<<2)+i*50+addr*0x51255616;
			*(volatile unsigned int*)addr = Wdata;
			Rdata =  *(volatile unsigned int*)(addr);

			//printf("DDR test result:Addr=%x  Wdata = %x Rdata = %x ; fail_flag = %d wrong_addr=%x\n",addr,Wdata,Rdata,fail_flag,wrong_addr);
			//printf("DDR test result:Addr=%x  Wdata = %x Rdata = %x ; \n",addr,Wdata,Rdata);
			if(Rdata != Wdata){
				j++;
				fail_flag  = 1 ;
				wrong_addr = addr;
				printf("DDR \"Write&read\" fail result: fail_Addr=%x  exp = %x act = %x ; \n",addr,Wdata,Rdata);
			}
			n =j+n;
		}

		printf("DDR \"Write&read\" test %d : fail %d times\n",i,n);

	}
	////////////////write////////////////
	n = 0;
	printf("Start read after Write all!!\n ");
	for(i=0;i<1;i++){
		for(addr=0xa0000000;addr<0xaffffff0;addr = addr+4){

			Wdata = ~addr+(addr<<2)+i*50+addr*0x51255616;
			*(volatile unsigned int*)addr = Wdata;
		}
		/////////////////read////////////////
		for(addr=0xa0000000;addr<0xaffffff0;addr = addr+4){
			j=0;
			Wdata = ~addr+(addr<<2)+i*50+addr*0x51255616;
			Rdata =  *(volatile unsigned int*)(addr);
			//printf("DDR test result:Addr=%x  Wdata = %x Rdata = %x ; fail_flag = %d wrong_addr=%x\n",addr,Wdata,Rdata,fail_flag,wrong_addr);
			if(Rdata != Wdata){
				j++;
				fail_flag  = 1 ;
				wrong_addr = addr;
				printf("DDR \"read after Write all\" result: fail_Addr=%x  exp = %x act = %x ; \n",addr,Wdata,Rdata);
			}
			n =j+n;
		}
		printf("DDR \"read after Write all\" %d : fail %d times\n",i,n);
	}

	printf("Finish DDR test!!!\n",i,n);
	if(fail_flag == 1)
		printf("DDR test fail!! \n");

	else
		printf("DDR test pass!! \n");
	printf("mem test3 end!\n");

}

static void ddr_mem_test9(void)
{
	int i = 0;
	int j = 0;
	unsigned int addr = 0x81000000;
	unsigned int value_get;
	unsigned int data;
	unsigned int test_size =  64;
	unsigned int test_data =  0xffffffff;
	printf("mem test9 start!\n");
	for(i = 0; i < 1; i++) {
		addr = 0x81000000;
		for (j = 0; j < test_size; j+=4) {
			data = (j/4)%2?test_data:~test_data;
			*(volatile unsigned int *)(addr+j) = data;
		}
		//flush cache
		flush_dcache_range(addr, addr+test_size);
		flush_l2cache_range(addr, addr+test_size);
		addr = 0xa1000000;
		for (j = 0; j < test_size; j+=4) {
			data = (j/4)%2?test_data:~test_data;
			value_get = *(volatile unsigned int *)(addr+j);
			if (value_get != data) {
				printf("mem test9 idx %d, addr 0x%x  want 0x%x get 0x%x error 0x%x\n",
						i, addr+j, data, value_get, data^value_get);
			}
		}
	}
	printf("mem test9 end!\n");
}

static void ddr_mem_test10(void)
{
	int i = 0;
	int j = 0;
	unsigned int addr;
	unsigned int value_get;
	unsigned int data;
	unsigned int test_size =  64;
	unsigned int test_data =  0xffffffff;
	printf("mem test10 start!\n");
	for(i = 0; i < 1; i++) {
		addr = 0x81000000;
		for (j = 0; j < test_size; j+=4) {
			value_get = *(volatile unsigned int *)(addr+j);
		}

		addr = 0xa1000000;
		for (j = 0; j < test_size; j+=4) {
			data = (j/4)%2?test_data:~test_data;
			*(volatile unsigned int *)(addr+j) = data;
		}
		//flush cache
		addr = 0x81000000;
		invalid_dcache_range(addr, addr+test_size);
		invalid_l2cache_range(addr, addr+test_size);
		addr = 0x81000000;
		for (j = 0; j < test_size; j+=4) {
			data = (j/4)%2?test_data:~test_data;
			value_get = *(volatile unsigned int *)(addr+j);
			if (value_get != data) {
				printf("mem test10 idx %d, addr 0x%x  want 0x%x get 0x%x error 0x%x\n",
						i, addr+j, data, value_get, data^value_get);
			}
		}
	}
	printf("mem test10 end!\n");
}

static void ddr_mem_test11(void)
{
	int i = 0;
	int j = 0;
	unsigned int addr;
	unsigned int value_get;
	unsigned int data;
	unsigned int test_size =  64*1024*1024;
	unsigned int test_data =  0xffffffff;

	printf("mem test11 start!\n");
	for(i = 0; i < 1; i++) {
		addr = 0xa1000000;
		for (j = 0; j < test_size; j+=4) {
			data = (j/4)%2?test_data:0;
			*(volatile unsigned int *)(addr+j) = data;
		}
		addr = 0xa1000000;
		for (j = 0; j < test_size; j+=4) {
			data = (j/4)%2?test_data:0;
			//data = (j/4)%2?0x5a5a5a5a:~0xa5a5a5a5;
			value_get = *(volatile unsigned int *)(addr+j);
			if (value_get != data) {
				printf("mem test11 idx %d, addr 0x%x  want 0x%x get 0x%x error 0x%x\n",
						i, addr+j, data, value_get, data^value_get);
				int k = 0;
				int data_tmp;
				data_tmp = value_get;
				for (k = 0; k < 10; k++) {
					value_get = *(volatile unsigned int *)(addr+j);
					if (value_get == data) {
						printf("### %d, mem test11 right idx %d, addr 0x%x  want 0x%x get 0x%x error 0x%x\n",
								k, i, addr+j, data, value_get, data^value_get);
						break;
					} else if (value_get == data_tmp) {
						;
					} else {
						printf("### %d, mem test11 error idx %d, addr 0x%x  want 0x%x get 0x%x error 0x%x\n",
								k, i, addr+j, data, value_get, data^value_get);
						data_tmp = value_get;
					}
				}
			}
		}
	}
	printf("mem test11 end!\n");
}

#endif

static void ddr_mem_test12(void)
{
	int i = 0;
	int j = 0;
	unsigned int addr;
	unsigned int value_get;
	unsigned int data;
	unsigned int test_size =  64;
	unsigned int test_data =  0xffffffff;
	//printf("mem test12 start!\n");
	for(i = 0; i < 1; i++) {


		addr = 0x81000000;
		for (j = 0; j < test_size; j+=4) {
			data = (j/4)%2?test_data:0;
			*(volatile unsigned int *)(addr+j) = data;
		}

		//flush cache
		flush_dcache_range(addr, addr+test_size);
		flush_l2cache_range(addr, addr+test_size);

		//read cached
		addr = 0x81000000;
		for (j = 0; j < test_size; j+=4) {
			value_get = *(volatile unsigned int *)(addr+j);
		}
#if 0
		//write uncached
		addr = 0xa1000000;
		for (j = 0; j < test_size; j+=4) {
			data = (j/4)%2?test_data:0;
			*(volatile unsigned int *)(addr+j) = data;
		}
#endif
		//invalid cache
		addr = 0x81000000;
		invalid_dcache_range(addr, addr+test_size);
		invalid_l2cache_range(addr, addr+test_size);
		//read cached
		addr = 0x81000000;
		printf("mem data:\n");
		for (j = 0; j < test_size; j+=4) {
			value_get = *(volatile unsigned int *)(addr+j);
			if (0 == j/4%4)
				printf("%x: ", addr+j);
			printf("  %x", value_get);
			if (3 == j/4%4)
				printf("\n");
		}
	}
	//printf("mem test10 end!\n");
}

#ifdef CONFIG_SIMULATION
volatile noinline void hello_word(void)
{
	while(1)
		asm volatile ("wait");
}
#endif

#define cache_op(op, addr)		\
	__asm__ __volatile__(		\
		".set	push\n"		\
		".set	noreorder\n"	\
		".set	mips3\n"	\
		"cache	%0, %1\n"	\
		".set	pop\n"		\
		:			\
		: "i" (op), "R" (*(unsigned char *)(addr)))


static void flush_dcache_range1(u32 addr, u32 size)
{
	u32 start = addr;
	for (; start < addr+size; start += CONFIG_SYS_CACHELINE_SIZE) {
		cache_op(INDEX_WRITEBACK_INV_D, start);
	}
	__asm__ __volatile__("sync");
}

static void init_bootuptime_timer(void)
{
       writel(TCU_TCSR_PRESCALE16, TCU_TCSR(3));
       writel(0xffff, TCU_TDFR(3));
       writel(0, TCU_TCNT(3));
       writel(TCU_TER_TCEN3, TCU_TESR);
}

void board_init_f(ulong dummy)
{
	unsigned int ccu_value=0;
	/*init bootup time timer as early as possible. */
	init_bootuptime_timer();

	/* Set global data pointer */
	gd = &gdata;

	/* close watchdog */
	*((volatile unsigned int *)(0xb0002004)) = 0;
    /* close ccu IFU simple Loop */
	ccu_value = *((volatile unsigned int *)(0xb2200fe0));
	*((volatile unsigned int *)(0xb2200fe0)) = (ccu_value | 0x00000078);

	/* Setup global info */
#ifndef CONFIG_CMD_BURN
	gd->arch.gi = &ginfo;
#else
	gd->arch.gi = (struct global_info *)CONFIG_SPL_GINFO_BASE;
#endif

#ifdef CONFIG_BURNER
#ifdef CONFIG_A1ALL
	gd->arch.gi->ddr_div = ((pddr_params->config_sys_apll_freq % pddr_params->config_sys_mpll_freq) == 0)
		               ? (pddr_params->config_sys_apll_freq / pddr_params->config_sys_mpll_freq)
		               : (pddr_params->config_sys_apll_freq / pddr_params->config_sys_mpll_freq + 1);
#else

	gd->arch.gi->ddr_div = ((gd->arch.gi->cpufreq % gd->arch.gi->ddrfreq) == 0)
		               ? (gd->arch.gi->cpufreq / gd->arch.gi->ddrfreq)
		               : (gd->arch.gi->cpufreq / gd->arch.gi->ddrfreq + 1);
#endif  /*end of CONFIG_A1ALL*/
#endif
	gpio_init();

#ifndef CONFIG_FPGA
	/* Init uart first */
	enable_uart_clk();
#endif

#ifdef CONFIG_SPL_SERIAL_SUPPORT
	preloader_console_init();
#endif

#ifdef CONFIG_A1ALL
	get_a1chip_ddr_config();
#endif

#ifndef CONFIG_FPGA
	debug("Timer init\n");
	timer_init();

#ifdef CONFIG_SPL_REGULATOR_SUPPORT
	debug("regulator set\n");
	spl_regulator_set();
#endif
#ifdef CONFIG_SIMULATION
	debug("CLK stop\n");
#endif
	clk_prepare();
#ifdef CONFIG_SIMULATION
	debug("PLL init\n");
#endif
#ifndef CONFIG_FPGA
	pll_init();
#endif
#ifdef CONFIG_SIMULATION
	debug("CLK init\n");
#endif
	clk_init();
#endif
#ifdef CONFIG_SIMULATION
	debug("SDRAM init\n");
#endif
	sdram_init();
#ifdef CONFIG_SIMULATION
	printf("SDRAM init ok\n");
#endif

#ifdef CONFIG_SIMULATION
	{
		hello_word();
	}
#endif

#ifdef CONFIG_DDR_TEST
	ddr_basic_tests();
#endif

#ifdef DDR_MEM_TEST
	//ddr_mem_test1();
	//ddr_mem_test2();
	//ddr_mem_test3();
	//ddr_mem_test5();
	//ddr_mem_test6();
	//ddr_mem_test7();
	//ddr_mem_test9();
	//ddr_mem_test10();
	//ddr_mem_test11();
	//ddr_mem_test4();
	//ddr_mem_test8();
#endif
	ddr_mem_test12();

#ifndef CONFIG_BURNER
	/* Clear the BSS */
	memset(__bss_start, 0, (char *)&__bss_end - __bss_start);
#ifdef CONFIG_PALLADIUM
	{
		debug("Going to palladium kernel......\n");
		void (*kernel)(void);
		kernel = (void (*)(void))0x80010000;
		(*kernel)();
	}
#endif
	debug("board_init_r\n");
	board_init_r(NULL, 0);
#endif
}

extern void flush_cache_all(void);

void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image)
{
	typedef void __noreturn (*image_entry_noargs_t)(void);

	image_entry_noargs_t image_entry =
		(image_entry_noargs_t) spl_image->entry_point;
	flush_cache_all();
	/* allcate L2 cache size */
	/***********************************
	  L2 cache size
	  reg addr: 0x12200060
	  bit   12 11 10
			0   0  0   L2C=0KB
			0   0  1   L2C=128KB
			0   1  0   L2C=256KB
			0   1  1   L2C=512KB
			1   0  0   L2C=1024KB
	 ***********************************/
	/*twait l2cache alloc ok */
	__asm__ volatile(
			".set push     \n\t"
			".set mips32r2 \n\t"
			"sync          \n\t"
			"lw $0,0(%0)   \n\t"
			".set pop      \n\t"
			::"r" (0xa0000000));
	*((volatile unsigned int *)(0xb2200060)) = (*(volatile unsigned int *)(0xb2200060) & ~(0x7<<10)) | (0x1<<10);
	__asm__ volatile(
			".set push     \n\t"
			".set mips32r2 \n\t"
			"sync          \n\t"
			"lw $0,0(%0)   \n\t"
			".set pop      \n\t"
			::"r" (0xa0000000));
	//printf("image entry point: 0x%X\n", spl_image->entry_point);
	image_entry();
}

#endif /* CONFIG_SPL_BUILD */

/*
 * U-Boot common functions
 */

void enable_interrupts(void)
{
}

int disable_interrupts(void)
{
	return 0;
}
