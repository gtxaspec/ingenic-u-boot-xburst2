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

#ifndef __A1_VO_H__
#define __A1_VO_H__

#define CFG_MAXARGS 10
#define PIC_MAX_WIDTH 3840
#define PIC_MAX_HEIGHT 2160
#define PIC_MIN_LENGTH 32
#define VO_LAYER_DEVICE_0           0
#define VO_LAYER_DEVICE_1           1
#define NR_MAX_VO_LAYER_DEVS        2

typedef enum{
	VO_INTF_VGA = 0x01,
	VO_INTF_HDMI = 0x02,
	VO_INTF_TFT = 0x04,
}VO_INTF_TYPE_E;

typedef enum{
	VO_OUTPUT_1080P24 = 0,
	VO_OUTPUT_1080P25,
	VO_OUTPUT_1080P30,

	VO_OUTPUT_720P50,
	VO_OUTPUT_720P60,
	VO_OUTPUT_1080P50,
	VO_OUTPUT_1080P60,

	VO_OUTPUT_576P50,
	VO_OUTPUT_480P60,

	VO_OUTPUT_640x480_60,            /* VESA 640 x 480 at 60 Hz (non-interlaced) CVT */
	VO_OUTPUT_800x600_60,            /* VESA 800 x 600 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1024x768_60,           /* VESA 1024 x 768 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1280x1024_60,          /* VESA 1280 x 1024 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1366x768_60,           /* VESA 1366 x 768 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1440x900_60,           /* VESA 1440 x 900 at 60 Hz (non-interlaced) CVT Compliant */
	VO_OUTPUT_1280x800_60,           /* 1280*800@60Hz VGA@60Hz*/
	VO_OUTPUT_BUTT
}VO_INTF_SYNC_E;

#endif
