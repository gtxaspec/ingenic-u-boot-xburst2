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

extern int load_jpeg(void);
extern int jpeg_decode(void);
extern void get_jpeg_info(unsigned int *w, unsigned int *h, unsigned int *y_addr, unsigned int *uv_addr);

static int do_jpegd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	unsigned int w, h, y, uv;

	ret = load_jpeg();
	if (ret != 0)
	{
		printf("load jpeg err\n");
		return -1;
	}
	ret = jpeg_decode();
	if (ret != 0)
	{
		printf("jpeg decode err\n");
		return -1;
	}
	get_jpeg_info(&w, &h, &y, &uv);
	//printf("w = %d, h = %d, Yaddr = 0x%08x, UVaddr = 0x%08x\n", w, h, y, uv);
	return 0;
}

U_BOOT_CMD(
	decjpg,1,1,do_jpegd,
	"jpgd   - decode jpeg picture.\n",
	"\t- setenv jpeg_addr 0x--------\n"
	"\t- setenv jpeg_size 0x--------\n"
	"\t- setenv vobuf     0x--------\n"
	);

