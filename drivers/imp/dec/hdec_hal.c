#include <common.h>
#include <command.h>
#include <asm/arch-a1/cpm.h>
#include <asm/arch/clk.h>
#include <asm/io.h>
#include <asm/types.h>
#include "hdec_hal.h"
#include "hdec_reg.h"

extern unsigned char *jpeg;
extern unsigned char *vdma;
extern unsigned char *vobuf;
unsigned char *JpegBuf = NULL;
unsigned char *bs = NULL;
int huffenc[384] = {0};
int huffmin[64] = {0};
int hea[64] = {0};
int heb[336] = {0};
int qt[4][64] = {0};
int regs[8] = {0};
unsigned int ImgWidth = 0, ImgHeight = 0;

extern void flush_cache_all(void);

int SampRate_Y_H,SampRate_Y_V,SampRate_U_H,SampRate_U_V,SampRate_V_H,SampRate_V_V;
int H_YtoU,V_YtoU,H_YtoV,V_YtoV,Y_in_MCU,U_in_MCU,V_in_MCU;
unsigned char *lp,*lpPtr;
int qt_table[3][64];
int comp_num;
unsigned char comp_index[3];
unsigned char YDcIndex,YAcIndex,UVDcIndex,UVAcIndex,HufTabIndex;
int *YQtTable,*UQtTable,*VQtTable;
unsigned char MyAnd[9]={0,1,3,7,0x0f,0x1f,0x3f,0x7f,0xff};
int code_pos_table[4][16];
int code_len_table[4][16];
unsigned short code_value_table[4][256];
unsigned short huf_max_value[4][16],huf_min_value[4][16];
int BitPos,CurByte,rrun,vvalue;
int MCUBuffer[640],BlockBuffer[64];
int QtZzMCUBuffer[640];
int ycoef,ucoef,vcoef,IntervalFlag,interval = 0;
int Y[256],U[256],V[256];
int sizei,sizej;
int restart;
int iclip[1024];
int LineBytes;
int value_type;

const int Zig_Zag[8][8]=
{
	{0,1,5,6,14,15,27,28},
	{2,4,7,13,16,26,29,42},
	{3,8,12,17,25,30,41,43},
	{9,11,18,24,31,40,44,53},
	{10,19,23,32,39,45,52,54},
	{20,22,33,38,46,51,55,60},
	{21,34,37,47,50,56,59,61},
	{35,36,48,49,57,58,62,63}
};

int LoadJpegFile(void)
{
	int ret = 0;
	JpegBuf = jpeg;
#ifdef CONFIG_HW_DECODE
	ret = Init_Table();
	if (ret != 0)
	{
		printf("Init Table err\n");
		return -1;
	}

	ret = Init_Header();
	if (ret != 0)
	{
		printf("Init Header err\n");
		return -1;
	}

	ret = Jpeg_HWDecode_Uboot();
	if (ret != 0)
	{
		printf("Jpeg HW Decode err\n");
		return -1;
	}
	else
	{
		printf("width=%d, height=%d, stride=%d, yaddr=0x%x, uvaddr=0x%x\n", ImgWidth, ALIGNDOWN(ImgHeight, 16), ImgWidth, (unsigned int)vobuf, (unsigned int)(vobuf+ALIGNUP(ImgWidth, 16)*ALIGNUP(ImgHeight, 16)));
	}

	ret = jpeg_reset();
	if (ret != 0)
	{
		printf("Jpeg HW reset err!\n");
		return -1;
	}
	jpeg_clk_gate_close();
#else
	InitTable();

	ret = InitHeader();
	if (ret != 0)
	{
		printf("Init Header err\n");
		return -1;
	}

	if ((SampRate_Y_H == 0) || (SampRate_Y_V == 0))
		return -1;

	int mod = ImgWidth%8;
	if (mod != 0)
		LineBytes = (ImgWidth + (8-mod)) * 4;
	else
		LineBytes = ImgWidth * 4;
	lpPtr = vobuf;

	ret = Decode();
	if (ret != 0)
	{
		printf("Jpeg SW Decode err\n");
		return -1;
	}
	else
	{
		printf("width=%d, height=%d, stride=%d, addr=0x%x\n", ImgWidth, ImgHeight, LineBytes, (unsigned int)vobuf);
	}
#endif

	return 0;
}

int Init_Table(void)
{
	int i, j;

	for (i = 0; i < 384; i++)
		huffenc[i] = 0xFFF;
	for (i = 168, j = 0xFD0; i < 176; i++, j++)
		huffenc[i] = j;
	for (i = 344, j = 0xFD0; i < 352; i++, j++)
		huffenc[i] = j;

	for (i = 162; i < 174; i++)
		heb[i] = 0;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 64; j++)
			qt[i][j] = 0;

	return 0;
}

int Init_Header(void)
{
	int ret = 0;
	int i,j,n,v,c,code,hid;
	int dc,abase,bbase,hbase,hb;
	int l[16] = {0}, nblk[4] = {0}, qt_sel[4] = {0};
	int ha_sel[4] = {0}, hd_sel[4] = {0}, min[64] = {0};
	char tmp[4] = {0};
	int iPos = 0, ht = 0, nTemp = 0, ncol = 0, nrst = 0, dst_w = 0, dst_h = 0;
	unsigned int min0, min1, min2, min3;

	iPos += 2;
	while (ret == 0)
	{
		c = JpegBuf[iPos++];
		while (c != 0xFF);
		c = JpegBuf[iPos++];
		while (c == 0xFF);
		switch (c)
		{
			case M_SOF0:
				iPos += 3;
				v = JpegBuf[iPos++];
				dst_h = (v << 8) | JpegBuf[iPos++];
				ImgHeight = dst_h;
				v = JpegBuf[iPos++];
				dst_w = (v << 8) | JpegBuf[iPos++];
				if (dst_w % 16)
				{
					printf("Jpeg file width must be align to 16\n");
					return -1;
				}
				ImgWidth = dst_w;
				ncol = n = JpegBuf[iPos++];
				for (i = 0; i < n; i++)
				{
					iPos++;
					nblk[i] = JpegBuf[iPos++];
					qt_sel[i] = JpegBuf[iPos++];
				}
				break;

			case M_DHT:
				v = JpegBuf[iPos++];
				n = (v << 8) | JpegBuf[iPos++];
				n -= 2;
				while (n)
				{
					v = JpegBuf[iPos++];
					n--;
					hid = (v >> 4) ? 2 : 0;
					hid |= (v & 15) ? 1 : 0;
					switch (hid)
					{
						case 1:
							hbase = 368;
							break;
						case 2:
							hbase = 0;
							break;
						case 3:
							hbase = 176;
							break;
						default:
							hbase = 352;
							break;
					}
					if (v >> 4)
						abase = 0;
					else
						abase = 1;
					dc = abase;
					ht = v & 15;
					abase |= ht << 1;
					switch (abase)
					{
						case 1:
						case 3:
							bbase = 162;
							break;
						case 2:
							bbase = 174;
							break;
						default:
							bbase = 0;
							break;
					}
					abase <<= 4;
					for (i = abase; i < abase+16; i++)
						hea[i] = 255;
					for (i = 0; i < 16; i++)
						l[i] = JpegBuf[iPos++];
					n -= 16;
					code = 0;
					for (i = 0; i < 16; i++, abase++)
					{
						min[abase] = code;
						hea[abase] = bbase - code;
						if (l[i])
						{
							for (j = 0; j < l[i]; j++, bbase++)
							{
								v = JpegBuf[iPos++];
								n--;
								if (dc)
								{
									huffenc[hbase+v] = (i << 8) | (code & 0xff);
									v &= 15;
									if (ht)
										v <<= 4;
									heb[bbase] |= v;
								}
								else
								{
									if (v == 0)
										hb = 160;
									else if (v == 0xf0)
										hb = 161;
									else
										hb = (v >> 4) * 10 + (v & 0xf) - 1;
									huffenc[hbase+hb] = (i << 8) | (code & 0xff);
									heb[bbase] = v;
								}
								code++;
							}
						}
						code <<= 1;
					}
				}
				break;

			case M_SOS:
				tmp[0] = JpegBuf[iPos++];
				tmp[1] = JpegBuf[iPos++];
				nTemp = (tmp[0] << 8) | (tmp[1]);
				n = JpegBuf[iPos++];
				if ((nTemp != (n*2 + 6)) || (n < 1) || (n > 4))
				{
					printf("bad scan length Ls = %d vs. (%d * 2 + 6)\n", nTemp, n);
					return -1;
				}
				for (i = 0; i < n; i++)
				{
					iPos++;
					ha_sel[i] = hd_sel[i] = JpegBuf[iPos++];
					ha_sel[i] = (ha_sel[i] & 0x30) >> 4;
					hd_sel[i] = hd_sel[i] & 0x3;
				}
				iPos += 3;
				bs = JpegBuf + iPos;
				ret = 1;
				break;

			case M_DQT:
				v = JpegBuf[iPos++];
				v = (v << 8) | JpegBuf[iPos++];
				int len = v - 2;
				while (len > 0)
				{
					int prec;
					v = JpegBuf[iPos++];
					prec = v >> 4;
					n = v & 15;
					if (n > 3)
					{
						printf("error QT\n");
						return -1;
					}
					for (i = 0; i < 64; i++)
					{
						if (prec)
						{
							v = JpegBuf[iPos++];
							qt[n][i] = (v << 8) | JpegBuf[iPos++];
						}
						else
							qt[n][i] = JpegBuf[iPos++];
					}
					len -= 64 + 1;
					if (prec)
						len -= 64;
				}
				break;

			case M_DRI:
				v = JpegBuf[iPos++];
				v = (v << 8) | JpegBuf[iPos++];
				v = JpegBuf[iPos++];
				nrst = (v << 8) | JpegBuf[iPos++];
				break;

			case M_APP0:
				v = JpegBuf[iPos++];
				v = (v << 8) | JpegBuf[iPos++];
				v -= 2;
				for (i = 0; i < v; i++)
					iPos++;
				break;

			case M_EOI:
				printf("please check jpeg file, it's end with 0xFF D9\n");
				return -1;
				break;

			default:
				if ((c & 0xf0) != 0xd0)
				{
					tmp[2] = JpegBuf[iPos++];
					tmp[3] = JpegBuf[iPos++];
					iPos += ((u16)(unsigned char)tmp[3] | (((u16)(unsigned char)tmp[2]) << 8)) - 2;
				}
				else
					iPos++;
				break;
		}
	}
	for (i = 0; i < 64;)
	{
		v = min[i++] & 1;
		v <<= 2;
		v |= min[i++] & 3;
		v <<= 3;
		v |= min[i++] & 7;
		min3 = (v >> 2) & 0xF;
		v <<= 4;
		v |= min[i++] & 15;
		v <<= 5;
		v |= min[i++] & 31;
		v <<= 6;
		v |= min[i++] & 63;
		v <<= 7;
		v |= min[i++] & 127;
		v <<= 8;
		v |= min[i++] & 255;
		min2 = v & 0xFFFFFFFF;
		v = min[i++] & 255;
		v <<= 8;
		v |= min[i++] & 255;
		v <<= 8;
		v |= min[i++] & 255;
		v <<= 8;
		v |= min[i++] & 255;
		min1 = v & 0xFFFFFFFF;
		v = min[i++] & 255;
		v <<= 8;
		v |= min[i++] & 255;
		v <<= 8;
		v |= min[i++] & 255;
		v <<= 8;
		v |= min[i++] & 255;
		min0 = v & 0xFFFFFFFF;

		huffmin[i / 4 - 4] = min0;
		huffmin[i / 4 - 3] = min1;
		huffmin[i / 4 - 2] = min2;
		huffmin[i / 4 - 1] = min3;
	}

	for (i = 0; i < 64; i++)
		hea[i] = hea[i] & 0x1FF;
	for (i = 0; i < 336; i++)
		heb[i] = heb[i] & 0xFF;
	int reg_val;
	reg_val = ((ncol-1) & 0x3);
	if (nrst != 0)
		reg_val |= 0x4;
	regs[0] = reg_val;

	reg_val = ((dst_w+15) >> 4) * ((dst_h+15) >> 4) -1;
	regs[1] = reg_val;

	if (nrst != 0)
		regs[2] = nrst - 1;
	else
		regs[2] = 0;

	for (i = 0; i < 4; i++)
	{
		reg_val = (nblk[i] & 0x3) * ((nblk[i] & 0x30) >> 4) - 1;
		reg_val = (reg_val & 0xF) << 4;
		reg_val |= qt_sel[i] << 2;
		reg_val |= (ha_sel[i] & 0x1) << 1;
		reg_val |= (hd_sel[i] & 0x1) << 0;
		regs[i + 3] = reg_val;
	}

	regs[7] = 0x8000 | (((dst_w+15)&(~15)) / 16 - 1);

	return 0;
}

int Jpeg_HWDecode_Uboot(void)
{
	int i = 0, ret = 0, flag = 30;

	unsigned int *chn = (unsigned int *)vdma;

	GEN_VDMA_ACFG(chn, TCSM_FLUSH, 0, 0x0);
	GEN_VDMA_ACFG(chn, REG_JPGC_GLBI, 0, JPGC_CORE_OPEN);
	for (i = 0; i < HUFFMIN_LEN; i++)
		GEN_VDMA_ACFG(chn, REG_JPGC_HUFM+i*4, 0, huffmin[i]);
	for (i = 0; i < HUFFBASE_LEN; i++)
		GEN_VDMA_ACFG(chn, REG_JPGC_HUFB+i*4, 0, hea[i]);
	for (i = 0; i < HUFFSYM_LEN; i++)
		GEN_VDMA_ACFG(chn, REG_JPGC_HUFS+i*4, 0, heb[i]);
	for (i = 0; i < QMEM_LEN; i++)
		GEN_VDMA_ACFG(chn, REG_JPGC_QMEM+i*4, 0, qt[i / 64][i % 64]);
	GEN_VDMA_ACFG(chn, REG_JPGC_STAT, 0, 0x0);
	GEN_VDMA_ACFG(chn, REG_JPGC_BSA, 0, (unsigned int)bs - 0x80000000);
	GEN_VDMA_ACFG(chn, REG_JPGC_P0A, 0, (unsigned int)vobuf - 0x80000000);
	GEN_VDMA_ACFG(chn, REG_JPGC_P1A, 0, (unsigned int)(vobuf+ALIGNUP(ImgWidth, 16)*ALIGNUP(ImgHeight, 16)) - 0x80000000);
	GEN_VDMA_ACFG(chn, REG_JPGC_NMCU, 0, regs[1]);
	GEN_VDMA_ACFG(chn, REG_JPGC_NRSM, 0, regs[2]);
	GEN_VDMA_ACFG(chn, REG_JPGC_P0C, 0, regs[3]);
	GEN_VDMA_ACFG(chn, REG_JPGC_P1C, 0, regs[4]);
	GEN_VDMA_ACFG(chn, REG_JPGC_P2C, 0, regs[5]);
	GEN_VDMA_ACFG(chn, REG_JPGC_P3C, 0, regs[6]);
	GEN_VDMA_ACFG(chn, REG_JPGC_WIDTH, 0, regs[7]);
	GEN_VDMA_ACFG(chn, REG_JPGC_GLBI, 0, (YUV420PVH | ((regs[0] & 0x3) << 4) | JPEG_DEC | (regs[0] & 0x4) | JPGC_CORE_OPEN));
	GEN_VDMA_ACFG(chn, REG_JPGC_TRIG, VDMA_ACFG_TERM, JPGC_BS_TRIG | JPGC_PP_TRIG | JPGC_CORE_OPEN);

	flush_cache_all();

	jpegClearBits(CPM_BASE, CPM_CLKGR1, BIT(13));
	jpegClearBits(CPM_BASE, CPM_CLKGR1, BIT(3));
	jpegSetBits(CPM_BASE, CPM_SRBC0, BIT(21));
	while (jpegCheckBits(CPM_BASE, CPM_SRBC0, BIT(20)) == 0)
		udelay(1000);
	jpegSetBits(CPM_BASE, CPM_SRBC0, BIT(22));
	jpegWriteReg(CPM_BASE, CPM_SRBC0, 0x0);
	jpegWriteReg(JPEG_BASE, REG_SCH_GLBC, SCH_GLBC_HIAXI | SCH_INTE_RESERR | SCH_INTE_ACFGERR | SCH_INTE_BSERR | SCH_INTE_ENDF);
	jpegWriteReg(JPEG_BASE, REG_VDMA_TASKRG, VDMA_ACFG_DHA((unsigned int)vdma - 0x80000000) | JPGC_CORE_OPEN);
	while (1)
	{
		ret = jpegCheckBits(JPEG_BASE, REG_SCH_STAT, BIT(0));
		if (ret == 1 || flag == 0)
			break;
		udelay(1000);
		flag--;
	}
	if (ret != 0)
		return 0;
	else
		return -1;
}

void InitTable(void)
{
	int i,j;
	sizei = 0;
	sizej = 0;
	ImgWidth = 0;
	ImgHeight = 0;
	rrun = 0;
	vvalue = 0;
	BitPos = 0;
	CurByte = 0;
	IntervalFlag = -1;
	restart = 0;
	for (i = 0;i < 3;i++)
	{
		for (j = 0;j < 64;j++)
			qt_table[i][j] = 0;
	}
	comp_num = 0;
	HufTabIndex = 0;
	for (i = 0;i < 3;i++)
		comp_index[i] = 0;
	for (i = 0;i < 4;i++)
	{
		for (j = 0;j < 16;j++)
		{
			code_len_table[i][j] = 0;
			code_pos_table[i][j] = 0;
			huf_max_value[i][j] = 0;
			huf_min_value[i][j] = 0;
		}
	}
	for (i = 0;i < 4;i++)
	{
		for (j = 0;j < 256;j++)
			code_value_table[i][j] = 0;
	}
	for (i = 0;i< 640;i++)
	{
		MCUBuffer[i] = 0;
		QtZzMCUBuffer[i] = 0;
	}
	for (i = 0;i < 64;i++)
	{
		Y[i] = 0;
		U[i] = 0;
		V[i] = 0;
		BlockBuffer[i] = 0;
	}
	ycoef = 0;
	ucoef = 0;
	vcoef = 0;
}

int InitHeader(void)
{
	unsigned int finish = 0;
	unsigned char id;
	int llength,i,j,k,huftab1,huftab2,huftabindex,ccount;
	unsigned char hf_table_index,qt_table_index,comnum;
	unsigned char *lptemp;
	lp = JpegBuf + 2;
	while (finish == 0)
	{
		id = *(lp + 1);
		lp += 2;
		switch (id)
		{
			case M_APP0:
				llength = GETLEN(*(lp + 1),*lp);
				lp += llength;
				break;
			case M_DQT:
				llength = GETLEN(*(lp + 1), *lp);

				if (llength > 256)
				{
					printf("M_DQT len %d is out of range!\n", llength);
					return -1;
				}

				qt_table_index = *(lp + 2) & 0x0f;

				if (qt_table_index >= 3)
				{
					printf("qt_table_index err %d\n", qt_table_index);
					return -1;
				}

				lptemp = lp + 3;
				if (llength < 80)
				{
					for (i = 0;i < 64;i++)
					{
						qt_table[qt_table_index][i] = (int)(*(lptemp++));
						//printf("%d  %d,", *(unzig_zag+i), qt_table[qt_table_index][*(unzig_zag+i)]);
					}
				}
				else
				{
					for (i = 0;i < 64;i++)
						qt_table[qt_table_index][i] = (int)(*(lptemp++));
					qt_table_index = (*(lptemp++)) & 0x0f;
					for (i = 0;i < 64;i++)
						qt_table[qt_table_index][i] = (int)(*(lptemp++));
				}
				lp += llength;
				break;
			case M_SOF0:
				llength = GETLEN(*(lp + 1),*lp);
				ImgHeight = GETLEN(*(lp + 4),*(lp + 3));
				ImgWidth = GETLEN(*(lp + 6),*(lp + 5));
				comp_num = *(lp + 7);
				if ((comp_num != 1) && (comp_num != 3))
					return -1;
				if (comp_num == 3)
				{
					comp_index[0] = *(lp + 8);
					SampRate_Y_H = (*(lp + 9)) >> 4;
					SampRate_Y_V = (*(lp + 9)) & 0x0f;
					YQtTable = (int *)qt_table[*(lp + 10)];

					comp_index[1] = *(lp  + 11);
					SampRate_U_H = (*(lp + 12)) >> 4;
					SampRate_U_V = (*(lp + 12)) & 0x0f;
					UQtTable = (int *)qt_table[*(lp + 13)];

					comp_index[2] = *(lp + 14);
					SampRate_V_H = (*(lp  + 15)) >> 4;
					SampRate_V_V = (*(lp + 15)) & 0x0f;
					VQtTable = (int *)qt_table[*(lp + 16)];
				}
				else
				{
					comp_index[0] = *(lp + 8);
					SampRate_Y_H = (*(lp + 9)) >> 4;
					SampRate_Y_V = (*(lp + 9)) & 0x0f;
					YQtTable = (int *)qt_table[*(lp + 10)];

					comp_index[1] = *(lp + 8);
					SampRate_U_H = 1;
					SampRate_U_V = 1;
					UQtTable = (int *)qt_table[*(lp + 10)];

					comp_index[2] = *(lp + 8);
					SampRate_V_H = 1;
					SampRate_V_V = 1;
					VQtTable = (int *)qt_table[*(lp + 10)];
				}

				if (comp_num == 1)
				{
					if (SampRate_Y_H==SampRate_Y_V)
					{
						value_type = 0;
						SampRate_Y_H = 1;
						SampRate_Y_V = 1;
						SampRate_U_H = 0;
						SampRate_U_V = 0;
						SampRate_V_H = 0;
						SampRate_V_V = 0;
					}
				}
				else if ((comp_num == 3) && (SampRate_U_H==SampRate_V_H) && (SampRate_U_V==SampRate_V_V))
				{
					if (SampRate_Y_H==(SampRate_U_H<<1))
					{
						if (SampRate_Y_V==(SampRate_U_V<<1))
						{
							value_type= 3;
						}
						else if (SampRate_Y_V==SampRate_U_V)
						{
							value_type = 4;
						}
					}
					else if (SampRate_Y_H==SampRate_U_H)
					{
						if (SampRate_Y_V==(SampRate_U_V<<1))
						{
							value_type = 5;
						}
						else if (SampRate_Y_V==SampRate_U_V)
						{
							value_type = 6;
						}
					}
				}
				lp += llength;
				break;
			case M_DHT:
				llength = GETLEN(*(lp + 1),*lp);

				hf_table_index = *(lp + 2);
				lp += 2;
				while (hf_table_index != 0xff)
				{
					huftab1 = (int)hf_table_index >> 4;
					huftab2 = (int)hf_table_index & 0x0f;
					huftabindex = huftab1 * 2 + huftab2;
					lptemp = lp;
					lptemp++;
					ccount = 0;
					for (i = 0;i < 16;i++)
					{
						code_len_table[huftabindex][i] = (int)(*(lptemp++));
						ccount = ccount + code_len_table[huftabindex][i];
					}
					ccount = ccount + 17;
					j = 0;
					for (i = 0;i < 16;i++)
					{
						if (code_len_table[huftabindex][i] != 0)
						{
							k = 0;
							while (k < code_len_table[huftabindex][i])
							{
								code_value_table[huftabindex][k + j] = (int)(*(lptemp++));
								k++;
							}
							j = j + k;
						}
					}
					i = 0;
					while (code_len_table[huftabindex][i] == 0)
						i++;
					for (j = 0;j < i;j++)
					{
						huf_min_value[huftabindex][j] = 0;
						huf_max_value[huftabindex][j] = 0;
					}
					huf_min_value[huftabindex][i] = 0;
					huf_max_value[huftabindex][i] = code_len_table[huftabindex][i] - 1;
					for (j = i + 1;j < 16;j++)
					{
						huf_min_value[huftabindex][j] = (huf_max_value[huftabindex][j - 1] + 1) << 1;
						huf_max_value[huftabindex][j] = huf_min_value[huftabindex][j] + code_len_table[huftabindex][j] - 1;
					}
					code_pos_table[huftabindex][0] = 0;
					for (j = 1;j < 16;j++)
						code_pos_table[huftabindex][j] = code_len_table[huftabindex][j - 1] + code_pos_table[huftabindex][j - 1];
					lp += ccount;
					hf_table_index = *lp;
				}
				break;
			case M_DRI:
				llength = GETLEN(*(lp + 1),*lp);
				restart = GETLEN(*(lp + 3),*(lp + 2));
				lp += llength;
				break;
			case M_SOS:
				llength = GETLEN(*(lp + 1),*lp);
				comnum = *(lp + 2);
				if (comnum != comp_num)
					return -1;
				lptemp = lp + 3;
				for (i = 0;i < comp_num;i++)
				{
					if (*lptemp == comp_index[0])
					{
						YDcIndex = (*(lptemp + 1)) >> 4;
						YAcIndex = ((*(lptemp + 1)) & 0x0f) + 2;
					}
					else
					{
						UVDcIndex = (*(lptemp + 1)) >> 4;
						UVAcIndex = ((*(lptemp + 1)) & 0x0f) + 2;
					}
					lptemp += 2;
				}
				lp += llength;
				finish = 1;
				break;
			case M_EOI:
				return -1;
				break;
			default:
				if ((id & 0xf0) != 0xd0)
				{
					llength = GETLEN(*(lp + 1),*lp);
					lp += llength;
				}
				else
					lp += 2;
				break;
		}
	}
	return 0;
}

int Decode(void)
{
	int ret;

	Y_in_MCU = SampRate_Y_H * SampRate_Y_V;
	U_in_MCU = SampRate_U_H * SampRate_U_V;
	V_in_MCU = SampRate_V_H * SampRate_V_V;
	H_YtoU = SampRate_Y_H / SampRate_U_H;
	V_YtoU = SampRate_Y_V / SampRate_U_V;
	H_YtoV = SampRate_Y_H / SampRate_V_H;
	V_YtoV = SampRate_Y_V / SampRate_V_V;
	Initialize_Fast_IDCT();
	ret = DecodeMCUBlock();
	while (ret == 0)
	{
		interval++;
		if ((restart != 0) && ((interval % restart) == 0))
			IntervalFlag = 0;
		else
			IntervalFlag = -1;
		IQtIZzMCUComponent(0);
		IQtIZzMCUComponent(1);
		IQtIZzMCUComponent(2);
		GetYUV(0);
		GetYUV(1);
		GetYUV(2);

		StoreBuffer();
		flush_cache_all();

		sizej = sizej + (unsigned int)(SampRate_Y_H << 3);
		if (sizej >= ImgWidth)
		{
			sizej = 0;
			sizei = sizei + (unsigned int)(SampRate_Y_V << 3);
		}
		if ((sizej == 0) && (sizei >= ImgHeight))
			break;

		ret = DecodeMCUBlock();
	}

	return ret;
}
void GetYUV(int flag)
{
	int H,VV,i,j,k,hk;
	int temp;
	int *buf,*tempbuf,*pQtZzMCU;
	switch (flag)
	{
		case 0:
			H = SampRate_Y_H;
			VV = SampRate_Y_V;
			buf = Y;
			pQtZzMCU = QtZzMCUBuffer;
			break;
		case 1:
			H = SampRate_U_H;
			VV = SampRate_U_V;
			buf = U;
			pQtZzMCU = QtZzMCUBuffer;
			pQtZzMCU = pQtZzMCU + (Y_in_MCU << 6);
			break;
		case 2:
			H = SampRate_V_H;
			VV = SampRate_V_V;
			buf = V;
			pQtZzMCU = QtZzMCUBuffer;
			pQtZzMCU = pQtZzMCU + ((Y_in_MCU + U_in_MCU) << 6);
			break;
		default:
			H = 0;
			VV = 0;
			buf = NULL;
			pQtZzMCU = NULL;
			break;
	}
	for (i = 0;i < VV;i++)
	{
		for (j = 0;j < H;j++)
		{
			for (k = 0;k < 8;k++)
			{
				for (hk = 0;hk < 8;hk++)
				{
					temp = ((((i << 3) + k) * SampRate_Y_H) << 3) + (j << 3) + hk;
					tempbuf = buf;
					tempbuf = tempbuf + temp;
					*tempbuf = (int)(*(pQtZzMCU++));
				}
			}
		}
	}
}

void StoreBuffer(void)
{
	int i,j;
	unsigned char *lprgb;
	unsigned char R,G,B;
	int yy,uu,vv,rr,gg,bb;
	int TempSamp1,TempSamp2;
	TempSamp1 = SampRate_Y_V << 3;
	TempSamp2 = SampRate_Y_H << 3;
	int *pY;
	int *pU;
	int *pV;

	for (i = 0;i < TempSamp1;i++)
	{
		if ((sizei + i) < ImgHeight)
		{
			//lprgb = lpPtr + (UINT32)((ImgHeight - sizei - (UINT32)(i) - 1) * LineBytes + sizej * 3);
			lprgb = lpPtr + ( (sizei + i) * LineBytes + (sizej * 4) );
			//lprgb = lpPtr + ( (sizei + i) * LineBytes + (sizej * 2) );

			pY=Y+i*TempSamp2;
			pU=U+(i / V_YtoU)*TempSamp2;
			pV=V+(i / V_YtoV)*TempSamp2;

			for (j = 0;j < TempSamp2;j++)
			{
				if ((sizej + j) < ImgWidth)
				{
					yy = *pY++;
					uu = pU[ j/H_YtoU ];
					vv = pV[ j/H_YtoV ];
					rr = ((yy << 8) + 18 * uu + 367 * vv) >> 8;
					gg = ((yy << 8) - 159 * uu - 220 * vv) >> 8;
					bb = ((yy << 8) + 411 * uu - 29 * vv) >> 8;
					R = (unsigned char)(rr);
					G = (unsigned char)(gg);
					B = (unsigned char)(bb);
					if ((rr & 0xffffff00) != 0)
					{
						if (rr > 255)
							R = 255;
						else
						{
							if (rr < 0)
								R = 0;
						}
					}
					if ((gg & 0xffffff00) != 0)
					{
						if (gg > 255)
							G = 255;
						else
						{
							if (gg < 0)
								G = 0;
						}
					}
					if ((bb & 0xffffff00) != 0)
					{
						if (bb > 255)
							B = 255;
						else
						{
							if (bb < 0)
								B = 0;
						}
					}
					//*lprgb++ = (unsigned char)(B >> 3) + (unsigned char)((G << 2) & 0xe0);
					//*lprgb++ = (unsigned char)(G >> 6) + (unsigned char)((R >> 1) & 0x7c);
					// ARGB8888
					*lprgb++ = (unsigned char) B;
					*lprgb++ = (unsigned char) G;
					*lprgb++ = (unsigned char) R;
					*lprgb++ = (unsigned char) 0;
				}
				else
					break;
			}
		}
		else
			break;
	}
}

int DecodeMCUBlock(void)
{
	int *lpMCUBuffer;
	int i,j;
	int ret=0;
	int tempX;
	if (IntervalFlag == 0)
	{
		lp += 2;
		ycoef = 0;
		ucoef = 0;
		vcoef = 0;
		BitPos = 0;
		CurByte = 0;
	}
	switch (comp_num)
	{
		case 3:
			lpMCUBuffer = MCUBuffer;
			tempX = SampRate_Y_H * SampRate_Y_V;
			for (i = 0;i < tempX;i++)
			{
				ret = HufBlock(YDcIndex,YAcIndex);
				if (ret != 0)
				{
					printf("ret1 = %x\n", ret);
					return ret;
				}
				BlockBuffer[0] = BlockBuffer[0] + ycoef;
				ycoef = BlockBuffer[0];
				for (j = 0;j < 64;j++)
					*lpMCUBuffer++ = BlockBuffer[j];
			}
			tempX = SampRate_U_H * SampRate_U_V;
			for (i = 0;i < tempX;i++)
			{
				ret = HufBlock(UVDcIndex,UVAcIndex);
				if (ret != 0)
				{
					printf("ret2 = %x\n", ret);
					return ret;
				}
				BlockBuffer[0] = BlockBuffer[0] + ucoef;
				ucoef = BlockBuffer[0];
				for (j = 0;j < 64;j++)
					*lpMCUBuffer++ = BlockBuffer[j];
			}
			tempX = SampRate_V_H * SampRate_V_V;
			for (i = 0;i < tempX;i++)
			{
				ret = HufBlock(UVDcIndex,UVAcIndex);
				if (ret != 0)
				{
					printf("ret3 = %x\n", ret);
					return ret;
				}
				BlockBuffer[0] = BlockBuffer[0] + vcoef;
				vcoef = BlockBuffer[0];
				for (j = 0;j < 64;j++)
					*lpMCUBuffer++ = BlockBuffer[j];
			}
			break;
		case 1:
			lpMCUBuffer = MCUBuffer;
			ret = HufBlock(YDcIndex,YAcIndex);
			if (ret != 0)
			{
				printf("ret1 = %x\n", ret);
				return ret;
			}
			BlockBuffer[0] = BlockBuffer[0] + ycoef;
			ycoef = BlockBuffer[0];
			for (j = 0;j < 64;j++)
				*lpMCUBuffer++ = BlockBuffer[j];
			for (i = 0;i < 128;i++)
				*lpMCUBuffer++ = 0;
			break;
		default:
			return -1;
	}
	return 0;
}

int HufBlock(unsigned char dchufindex, unsigned char achufindex)
{
	int count,i;
	int ret;

	count = 0;
	HufTabIndex = dchufindex;
	ret = DecodeElement();
	if (ret != 0)
	{
		printf("DC ret1 = %x\n", ret);
		return ret;
	}
	BlockBuffer[count++] = vvalue;

	HufTabIndex = achufindex;
	while (count < 64)
	{
		ret = DecodeElement();
		if (ret != 0)
		{
			printf("AC ret1 = %x\n", ret);
			return ret;
		}
		if ((rrun == 0) && (vvalue == 0))
		{
			for (i = count;i < 64;i++)
				BlockBuffer[i] = 0;
			count = 64;
		}
		else
		{
			for (i = 0;i < rrun;i++)
				BlockBuffer[count++] = 0;
			BlockBuffer[count++] = vvalue;
		}
	}
	return 0;
}

int DecodeElement(void)
{
	int thiscode,tempcode;
	unsigned short temp,valueex;
	int codelen;
	unsigned char hufexbyte,runsize,tempsize,sign;
	unsigned char newbyte,lastbyte;

	if (BitPos >= 1)
	{
		BitPos--;
		thiscode = (unsigned char)CurByte >> BitPos;
		CurByte = CurByte & MyAnd[BitPos];
	}
	else
	{
		lastbyte = ReadByte();
		BitPos--;
		newbyte = CurByte & MyAnd[BitPos];
		thiscode = lastbyte >> 7;
		CurByte = newbyte;
	}
	codelen = 1;

	while ((thiscode < huf_min_value[HufTabIndex][codelen - 1])||(code_len_table[HufTabIndex][codelen - 1] == 0)||(thiscode > huf_max_value[HufTabIndex][codelen - 1]))
	{
		if (BitPos >= 1)
		{
			BitPos--;
			tempcode = (unsigned char)CurByte >> BitPos;
			CurByte = CurByte & MyAnd[BitPos];
		}
		else
		{
			lastbyte = ReadByte();
			BitPos--;
			newbyte = CurByte & MyAnd[BitPos];
			tempcode = (unsigned char)lastbyte >> 7;
			CurByte = newbyte;
		}
		thiscode = (thiscode << 1) + tempcode;
		codelen++;
		if (codelen > 16)
			return -1;
	}
	temp = thiscode - huf_min_value[HufTabIndex][codelen - 1] + code_pos_table[HufTabIndex][codelen - 1];
	hufexbyte = (unsigned char)code_value_table[HufTabIndex][temp];
	rrun = (int)(hufexbyte >> 4);
	runsize = hufexbyte & 0x0f;
	if (runsize == 0)
	{
		vvalue = 0;
		return 0;
	}
	tempsize = runsize;
	if (BitPos >= runsize)
	{
		BitPos = BitPos - runsize;
		valueex = (unsigned char)CurByte >> BitPos;
		CurByte = CurByte & MyAnd[BitPos];
	}
	else
	{
		valueex = CurByte;
		tempsize = tempsize - BitPos;
		while (tempsize > 8)
		{
			lastbyte = ReadByte();
			valueex = (valueex << 8) + (unsigned char)lastbyte;
			tempsize = tempsize - 8;
		}
		lastbyte = ReadByte();
		BitPos = BitPos - tempsize;
		valueex = (valueex << tempsize) + (lastbyte >> BitPos);
		CurByte = lastbyte & MyAnd[BitPos];
	}
	sign = valueex >> (runsize - 1);
	if (sign != 0)
		vvalue = valueex;
	else
	{
		valueex = valueex ^ 0xffff;
		temp = 0xffff << runsize;
		vvalue = -(int)(valueex ^ temp);
	}
	return 0;
}

void IQtIZzMCUComponent(int flag)
{
	int H,VV,i,j;
	int *pQtZzMCUBuffer,*tempbuf1;
	int *pMCUBuffer,*tempbuf2;

	switch (flag)
	{
		case 0:
			H = SampRate_Y_H;
			VV = SampRate_Y_V;
			pMCUBuffer = MCUBuffer;  /* Huffman Decoded */
			pQtZzMCUBuffer = QtZzMCUBuffer;
			break;
		case 1:
			H = SampRate_U_H;
			VV = SampRate_U_V;
			pMCUBuffer = MCUBuffer;
			pMCUBuffer += Y_in_MCU << 6;
			pQtZzMCUBuffer = QtZzMCUBuffer;
			pQtZzMCUBuffer += Y_in_MCU << 6;
			break;
		case 2:
			H = SampRate_V_H;
			VV = SampRate_V_V;
			pMCUBuffer = MCUBuffer;
			pMCUBuffer += (Y_in_MCU + U_in_MCU) << 6;
			pQtZzMCUBuffer = QtZzMCUBuffer;
			pQtZzMCUBuffer += (Y_in_MCU + U_in_MCU) << 6;
			break;
		default:
			H = 0;
			pQtZzMCUBuffer = NULL;
			pMCUBuffer = NULL;
			VV = 0;
			break;
	}
	for (i = 0;i < VV;i++)
	{
		for (j = 0;j < H;j++)
		{
			tempbuf2 = pMCUBuffer;
			tempbuf2 += (i * H + j) << 6;
			tempbuf1 = pQtZzMCUBuffer;
			tempbuf1 += (i * H + j) << 6;
			IQtIZzBlock(tempbuf2,tempbuf1,flag);      /* 8*8DU */
		}
	}
}

void IQtIZzBlock(int *s,int *d,int flag)
{
	int i,j,tag;
	int *pQt,*temp1,*temp3;
	int buffer2[8][8];
	int *temp2;
	int offset;

	switch (flag)
	{
		case 0:
			pQt = YQtTable;
			offset = 128;
			break;
		case 1:
			pQt = UQtTable;
			offset = 0;
			break;
		case 2:
			pQt = VQtTable;
			offset = 0;
			break;
		default:
			pQt = NULL;
			offset = 0;
			break;
	}

	for (i = 0;i < 8;i++)
	{
		for (j = 0;j < 8;j++)
		{
			tag = Zig_Zag[i][j];
			temp1 = s;
			temp1 = temp1 + tag;
			temp3 = pQt;
			temp3 = temp3 + tag;
			buffer2[i][j] = (int)((*temp1) * (*temp3));
		}
	}
	Fast_IDCT(&buffer2[0][0]);
	for (i = 0;i < 8;i++)
	{
		for (j = 0;j < 8;j++)
		{
			temp2 = d;
			temp2 = temp2 + (i * 8 +j);
			*temp2 = buffer2[i][j] + offset;
		}
	}
}

void Fast_IDCT(int *block)
{
	int i;
	for (i = 0;i < 8;i++)
		idctrow(block + 8 * i);
	for (i = 0;i < 8;i++)
		idctcol(block + i);
}

unsigned char ReadByte(void)
{
	unsigned char i;

	i = *lp;
	lp = lp + 1;
	if (i == 0xff)
		lp = lp + 1;
	BitPos = 8;
	CurByte = i;
	return i;
}

void Initialize_Fast_IDCT(void)
{
	int i;

	for (i = -512;i < 512;i++)
	{
		if (i < -256)
			iclip[512 + i] = -256;
		if (i > 255)
			iclip[512 + i] = 255;
		if (( i >= -256) && (i <= 255))
			iclip[512 + i] = i;
	}
}

void idctrow(int *blk)
{
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;

	x1 = blk[4] << 11;
	x2 = blk[6];
	x3 = blk[2];
	x4 = blk[1];
	x5 = blk[7];
	x6 = blk[5];
	x7 = blk[3];
	if ((x1 || x2 || x3 || x4 || x5 || x6 || x7) == 0)
	{
		blk[1] = blk[0] << 3;
		blk[2] = blk[0] << 3;
		blk[3] = blk[0] << 3;
		blk[4] = blk[0] << 3;
		blk[5] = blk[0] << 3;
		blk[6] = blk[0] << 3;
		blk[7] = blk[0] << 3;
		blk[0] = blk[0] << 3;
		return;
	}
	x0 = (blk[0] << 11) + 128;
	x8 = W7 * (x4 + x5);
	x4 = x8 + (W1 - W7) * x4;
	x5 = x8 - (W1 + W7) * x5;
	x8 = W3 * (x6 + x7);
	x6 = x8 - (W3 - W5) * x6;
	x7 = x8 - (W3 + W5) * x7;
	x8 = x0 + x1;
	x0 = x0 - x1;
	x1 = W6 * (x3 + x2);
	x2 = x1 - (W2 + W6) * x2;
	x3 = x1 + (W2 - W6) * x3;
	x1 = x4 + x6;
	x4 = x4 - x6;
	x6 = x5 + x7;
	x5 = x5 - x7;
	x7 = x8 + x3;
	x8 = x8 - x3;
	x3 = x0 + x2;
	x0 = x0 - x2;

	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;

	blk[0] = (x7 + x1) >> 8;
	blk[1] = (x3 + x2) >> 8;
	blk[2] = (x0 + x4) >> 8;
	blk[3] = (x8 + x6) >> 8;
	blk[4] = (x8 - x6) >> 8;
	blk[5] = (x0 - x4) >> 8;
	blk[6] = (x3 - x2) >> 8;
	blk[7] = (x7 - x1) >> 8;
}

void idctcol(int * blk)
{
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;
	int Temp;

	x1 = blk[8 * 4] << 8;
	x2 = blk[8 * 6];
	x3 = blk[8 * 2];
	x4 = blk[8 * 1];
	x5 = blk[8 * 7];
	x6 = blk[8 * 5];
	x7 = blk[8 * 3];
	if ((x1 | x2 | x3 | x4 | x5 | x6 | x7) == 0)
	{
		Temp = (blk[8 * 0] + 32) >> 6;
		blk[8 * 1] = iclip[512 + Temp];
		blk[8 * 2] = iclip[512 + Temp];
		blk[8 * 3] = iclip[512 + Temp];
		blk[8 * 4] = iclip[512 + Temp];
		blk[8 * 5] = iclip[512 + Temp];
		blk[8 * 6] = iclip[512 + Temp];
		blk[8 * 7] = iclip[512 + Temp];
		blk[8 * 0] = iclip[512 + Temp];
		return;
	}
	x0 = (blk[8 * 0] << 8) + 8192;
	x8 = W7 * (x4 + x5) + 4;

	x4 = (x8 + (W1 - W7) * x4) >> 3;
	x5 = (x8 - (W1 + W7) * x5) >> 3;
	x8 = W3 * (x6 + x7) + 4;

	x6 = (x8 -(W3 - W5) * x6) >> 3;
	x7 = (x8 - (W3 + W5) * x7) >> 3;
	x8 = x0 + x1;
	x0 = x0 - x1;
	x1 = W6 * (x3 + x2) + 4;

	x2 = (x1 - (W2 + W6) * x2) >> 3;
	x3 = (x1 + (W2 - W6) * x3) >> 3;

	x1 = x4 + x6;
	x4 = x4 - x6;
	x6 = x5 + x7;
	x5 = x5 - x7;
	x7 = x8 + x3;
	x8 = x8 - x3;
	x3 = x0 + x2;
	x0 = x0 - x2;

	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;
	Temp = (x7 + x1) >> 14;

	blk[0] = iclip[512 + Temp];
	Temp = (x3 + x2) >> 14;

	blk[8] = iclip[512 + Temp];
	Temp = (x0 + x4) >> 14;

	blk[16] = iclip[512 + Temp];
	Temp = (x8 + x6) >> 14;

	blk[24] = iclip[512 + Temp];
	Temp = (x8 - x6) >> 14;

	blk[32] = iclip[512 + Temp];
	Temp = (x0 - x4) >> 14;

	blk[40] = iclip[512 + Temp];
	Temp = (x3 - x2) >> 14;

	blk[48] = iclip[512 + Temp];
	Temp = (x7 - x1) >> 14;

	blk[56] = iclip[512 + Temp];
}
