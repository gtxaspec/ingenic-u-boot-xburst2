/*
 * (C) Copyright 2005-2022
 * Ingenic inc, sadick.wjxu@ingenic.com
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include "a1_vo.h"

extern int start_vo(unsigned int type,unsigned int sync);
extern int stop_vo(void);
extern int start_videolayer(unsigned int layer,unsigned int yaddr,unsigned int uvaddr,
		unsigned int stride,unsigned int x,unsigned int y,unsigned int w,unsigned int h);
extern int stop_videolayer(unsigned int layer);
extern int set_vo_background_color(unsigned int color);
extern int start_graphicslayer(unsigned int addr,unsigned int stride,unsigned int pixfmt,unsigned int w,unsigned int h);
extern int stop_graphicslayer(void);

static int atoi_checkargv(char *string)
{
	int length;
	int retval = 0;
	int i;
	int sign = 1;

	length = strlen(string);
	for (i = 0; i < length; i++) {
		if (0 == i && string[0] == '-') {
			sign = -1;
			continue;
		}
		if (string[i] > '9' || string[i] < '0')
			break;

		retval *= 10;
		retval += string[i] - '0';
	}
	retval *= sign;
	return retval;
}

static int do_start_vo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int intf_type,sync;

	if(argc < 3){
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}
	if(atoi_checkargv(argv[1]) < 0){
		printf("Invalid parameter!interface type:%d\n", atoi_checkargv(argv[2]));
		return -1;
	}
	if(atoi_checkargv(argv[2]) < 0){
		printf("Invalid parameter!display sync:%d\n", atoi_checkargv(argv[3]));
		return -1;
	}
	intf_type = (unsigned int)simple_strtoul(argv[1], NULL, 10);
	sync = (unsigned int)simple_strtoul(argv[2], NULL, 10);

	return start_vo(intf_type,sync);
}

static int do_stop_vo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return stop_vo();
}

static int do_startvl(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int layer, yaddr, uvaddr,stride,x,y,w,h;
	if(argc < 9){
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}

	layer	= (unsigned int)simple_strtoul(argv[1], NULL, 10);
	yaddr	= (unsigned int)simple_strtoul(argv[2], NULL, 16);
	uvaddr	= (unsigned int)simple_strtoul(argv[3], NULL, 16);
	stride	= (unsigned int)simple_strtoul(argv[4], NULL, 10);
	x		= (unsigned int)simple_strtoul(argv[5], NULL, 10);
	y		= (unsigned int)simple_strtoul(argv[6], NULL, 10);
	w		= (unsigned int)simple_strtoul(argv[7], NULL, 10);
	h		= (unsigned int)simple_strtoul(argv[8], NULL, 10);

	start_videolayer(layer,yaddr,uvaddr,stride,x,y,w,h);
	return 0;
}

static int do_stopvl(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int layer;

	if(argc < 2){
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}
	if(atoi_checkargv(argv[1]) < 0){
		printf("Invalid parameter!layer:%d\n", atoi_checkargv(argv[1]));
		return -1;
	}
	layer = (unsigned int)simple_strtoul(argv[1], NULL, 10);
	return stop_videolayer(layer);
}

static int do_setvobg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int rgb;
	if(argc < 2){
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}
	if(atoi_checkargv(argv[1]) < 0){
		printf("Invalid parameter!backgroung color:%d\n", atoi_checkargv(argv[1]));
		return -1;
	}
	rgb = (unsigned int)simple_strtoul(argv[1], NULL, 10);
	return set_vo_background_color(rgb);
}

static int do_startgx(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int addr,stride,w,h,pixfmt;
	if(argc < 6){
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}

	addr	= (unsigned int)simple_strtoul(argv[1], NULL, 10);
	stride	= (unsigned int)simple_strtoul(argv[2], NULL, 10);
	pixfmt	= (unsigned int)simple_strtoul(argv[3], NULL, 10);
	w		= (unsigned int)simple_strtoul(argv[4], NULL, 10);
	h		= (unsigned int)simple_strtoul(argv[5], NULL, 10);
	return start_graphicslayer(addr,stride,pixfmt,w,h);
}

static int do_stopgx(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return stop_graphicslayer();
}

U_BOOT_CMD(
	startvo,CFG_MAXARGS,1,do_start_vo,
	"startvo   - open interface of vo device.\n"
	"\t- startvo [type sync]",
	"\nargs: [type, sync]\n"
	"\t-<type>: 1(VGA),2(HDMI),4(TFT)\n"
	"\t\tsupport multi type eg: 3(HDMI|VGA)\n"
	"\t-<sync>:\n"
	"\t\t0(1080P24),1(1080P25),2(1080P30),3(720P50),\n"
	"\t\t4(720P60),5(1080P50),6(1080P60),7(576P50),\n"
	"\t\t8(480P60),9(640x480),10(800x600),11(1024x768)\n"
	"\t\t12(1280x1024),13(1366x768),14(1440x900),15(1280x800)\n"
	);

U_BOOT_CMD(
	stopvo,CFG_MAXARGS,1,do_stop_vo,
	"stopvo   - close interface of vo device.\n"
	"\t- stopvo [none]",
	"\nargs: [none]\n"
	);

U_BOOT_CMD(
	startvl,CFG_MAXARGS,1,do_startvl,
	"startvl   - open video layer.\n"
	"\t- startvl [layer yaddr uvaddr stride x y w h]\n",
	"\nargs: [layer, yaddr, uvaddr, stride, x, y, w, h]\n"
	"\t-<layer>   : 0(V0), 1(V1)\n"
	"\t-<yaddr>   : picture Y component address\n"
	"\t-<uvaddr>  : picture UV component address\n"
	"\t-<stride>  : picture stride\n"
	"\t-<x,y,w,h> : display area\n"
	);

U_BOOT_CMD(
	stopvl,CFG_MAXARGS,1,do_stopvl,
	"stopvl   - close video layer.\n"
	"\t- stopvl [layer]",
	"\nargs: [layer]\n"
	"\t-<layer> : 0(V0), 1(V1)\n"
	);

U_BOOT_CMD(
	startgx,CFG_MAXARGS,1,do_startgx,
	"startgx   - open graphics layer.\n"
	"\t- startgx [addr stride pixfmt w h]\n",
	"\nargs: [addr,stride,pixfmt,w,h]\n"
	"\t-<addr>    : picture address\n"
	"\t-<stride>  : picture stride\n"
	"\t-<pixfmt>  : picture pixel format\n"
	"\t-<w,h>     : display area\n"
	"\t-<pixfmt>:\n"
	"\t\t0(ARGB8888),1(ARBG8888),2(AGBR8888),3(AGRB888),\n"
	"\t\t4(ABGR8888),5(ABRG8888),6(RGBA8888),7(RBGA8888),\n"
	"\t\t8(GBRA8888),9(GRBA8888),10(BGRA8888),11(BRGA8888)\n"
	"\t\t12(ARGB1555),13(ARBG1555),14(AGBR1555),15(AGRB1555)\n"
	"\t\t16(ABGR1555),17(ABRG1555),18(RGBA5551),19(RBGA5551)\n"
	"\t\t20(GBRA5551),21(GRBA5551),22(BGRA5551),23(BRGA5551)\n"
	);

U_BOOT_CMD(
	stopgx,CFG_MAXARGS,1,do_stopgx,
	"stopgx   - close graphics layer.\n"
	"\t- stopgx [none]",
	"\nargs: [none]\n"
	);

U_BOOT_CMD(
	setvobg,CFG_MAXARGS,1,do_setvobg,
	"setvobg  - set background color.\n"
	"\t- setvobg [color]",
	"\nargs: [color]\n"
	"\t-<color> : rgb color space\n"
	);
