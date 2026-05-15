#include <common.h>
#include <command.h>
#include "hdec_hal.h"
#include "hdec_reg.h"

extern unsigned int ImgWidth, ImgHeight;
unsigned char *jpeg = NULL;
unsigned char *vdma = NULL;
unsigned char *vobuf = NULL;
unsigned int jpeg_size = 0;
unsigned int vdma_size = 0x5000;

int load_jpeg(void)
{
	char *s = NULL;
	unsigned int u = 0;

	s = getenv("jpeg_addr");
	if (s != NULL)
	{
		u = simple_strtol(s, NULL, 0);
		if (u == 0 || (u & 0x7f) != 0)
		{
			printf("Invalid jpeg addr 0x%08x\n", u);
			return -1;
		}
		jpeg = (unsigned char *)u;
		if (*(jpeg) != 0xFF || *(jpeg+1) != 0xD8)
		{
			printf("Jpeg File must begain with 0xFF D8!\n");
			return -1;
		}
		//printf("jpeg_addr ====> 0x%08x\n", u);
	}
	else
	{
		printf("you should set jpeg picture's address\n");
		return -1;
	}

	s = getenv("jpeg_size");
	if (s != NULL)
	{
		jpeg_size = simple_strtol(s, NULL, 0);
		//printf("jpeg_size ====> 0x%x\n", jpeg_size);
	}
	else
	{
		printf("you should set jpeg picture's size\n");
		return -1;
	}
	if (jpeg_size == 0)
	{
		printf("Invalid jpeg size 0x%08x\n", jpeg_size);
		return -1;
	}

	vdma = (jpeg + ALIGNUP(jpeg_size, 0x100));
	//printf("vdma ====> 0x%08x\n", (unsigned int)vdma);

	s = getenv("vobuf");
	if (s != NULL)
	{
		u = simple_strtol(s, NULL, 0);
		vobuf = (unsigned char *)u;
		//printf("vobuf ====> 0x%08x\n", u);
		if (u == 0 || (u & 0x7f) != 0 || u < ((unsigned int)jpeg+ALIGNUP(jpeg_size, 0x100)+vdma_size))
		{
			printf("Invalid vo buffer 0x%08x, vo buffer must greater or equal 0x%08x\n", u, ((unsigned int)jpeg+ALIGNUP(jpeg_size, 0x100)+vdma_size));
			return -1;
		}
	}
	else
	{
		printf("you should set vobuf's address\n");
		return -1;
	}

	return 0;
}

int jpeg_decode(void)
{
	int ret = 0;

	ret = LoadJpegFile();
	if (ret != 0)
	{
		return -1;
	}

	return 0;
}

void get_jpeg_info(unsigned int *w, unsigned int *h, unsigned int *y_addr, unsigned int *uv_addr)
{
	*w = ALIGNDOWN(ImgWidth, 16);
	*h = ALIGNDOWN(ImgHeight, 16);
	*y_addr = (unsigned int)vobuf-0x80000000;
	*uv_addr = (unsigned int)(vobuf+ALIGNUP(ImgWidth, 16)*ALIGNUP(ImgHeight, 16))-0x80000000;
}
