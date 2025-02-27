#ifndef __HDEC_HAL_H__
#define __HDEC_HAL_H__

#define M_SOF0  0xc0
#define M_DHT   0xc4
#define M_EOI   0xd9
#define M_SOS   0xda
#define M_DQT   0xdb
#define M_DRI   0xdd
#define M_APP0  0xe0

#define W1  2841
#define W2  2676
#define W3  2408
#define W5  1609
#define W6  1108
#define W7  565

#define GETLEN(a, b) ((unsigned short)((unsigned char)(a))|((unsigned short)((unsigned char)(b)) << 8))
#define ALIGNUP(a, b)	((a + (b - 1)) & ~(b - 1))
#define ALIGNDOWN(a, b)	(a & ~(b - 1))

int LoadJpegFile(void);
int Init_Table(void);
int Init_Header(void);
int Jpeg_HWDecode_Uboot(void);

void InitTable(void);
int InitHeader(void);
int Decode(void);
void GetYUV(int flag);
void StoreBuffer(void);
int DecodeMCUBlock(void);
int HufBlock(unsigned char dchufindex, unsigned char achufindex);
int DecodeElement(void);
void IQtIZzMCUComponent(int flag);
void IQtIZzBlock(int *s,int *d,int flag);
void Fast_IDCT(int * block);
unsigned char ReadByte(void);
void Initialize_Fast_IDCT(void);
void idctrow(int * blk);
void idctcol(int * blk);

#endif
