#ifndef __CONFIG_ISVP_T41_H__
#define __CONFIG_ISVP_T41_H__

#include "isvp_common.h"

/**
 * Boot arguments definitions.
 */
#define BOOTARGS_COMMON "mem=\\${osmem} rmem=\\${rmem}"
#if defined(CONFIG_DDR_128M) || defined(CONFIG_DDR_256M)
#define CONFIG_EXTRA_SETTINGS \
"osmem=99M@0x0\0" \
"rmem=29M@0x6300000\0"
#else
#define CONFIG_EXTRA_SETTINGS \
"osmem=42M@0x0\0" \
"rmem=22M@0x2a00000\0"
#endif

/*
	Platform Default GPIOs
	These shall be specific to the SoC model
*/

#define CONFIG_GPIO_SETTINGS \
"gpio_default=\0" \
"gpio_default_net=\0"



#if 0
#ifdef CONFIG_FAST_BOOT
#ifdef CONFIG_SPL_MMC_SUPPORT
	#define CONFIG_BOOTARGS BOOTARGS_COMMON " init=/linuxrc root=/dev/mmcblk0p2 rw rootdelay=1"
#elif defined(CONFIG_SFC_NOR)
#ifdef CONFIG_OF_LIBFDT
	#define CONFIG_BOOTARGS BOOTARGS_COMMON " init=/linuxrc rootfstype=squashfs root=/dev/mtdblock2 rw mtdparts=sfc0_nor:256k(boot),2560k(kernel),2048k(root),64k(dtb),-(appfs) lpj=11968512 quiet"
#else
	#define CONFIG_BOOTARGS BOOTARGS_COMMON " init=/linuxrc rootfstype=squashfs root=/dev/mtdblock2 rw mtdparts=sfc0_nor:256k(boot),2560k(kernel),2048k(root),-(appfs) lpj=11968512 quiet"
#endif
#elif defined(CONFIG_SFC_NAND)
	#define CONFIG_BOOTARGS BOOTARGS_COMMON " init=/linuxrc ubi.mtd=2 root=ubi0:rootfs rootfstype=ubifs rw mtdparts=sfc0_nand:1M(uboot),3M(kernel),20M(root),-(appfs) lpj=11968512 quiet"
#endif
#else
#ifdef CONFIG_SPL_MMC_SUPPORT
	#define CONFIG_BOOTARGS BOOTARGS_COMMON " init=/linuxrc root=/dev/mmcblk0p2 rw rootdelay=1"
#elif defined(CONFIG_SFC_NOR)
#ifdef CONFIG_OF_LIBFDT
	#define CONFIG_BOOTARGS BOOTARGS_COMMON " init=/linuxrc rootfstype=squashfs root=/dev/mtdblock2 rw mtdparts=sfc0_nor:256k(boot),2560k(kernel),2048k(root),64k(dtb),-(appfs) lpj=11968512"
#else
	#define CONFIG_BOOTARGS BOOTARGS_COMMON " init=/linuxrc rootfstype=squashfs root=/dev/mtdblock2 rw mtdparts=sfc0_nor:256k(boot),2560k(kernel),2048k(root),-(appfs) lpj=11968512"
#endif
#elif defined(CONFIG_SFC_NAND)
	#define CONFIG_BOOTARGS BOOTARGS_COMMON " init=/linuxrc ubi.mtd=2 root=ubi0:rootfs rootfstype=ubifs rw mtdparts=sfc0_nand:1M(uboot),3M(kernel),20M(root),-(appfs) lpj=11968512"
#endif
#endif

///////////////
/**
 * Boot command definitions.
 */
 #define CONFIG_BOOTDELAY 1

 #ifdef CONFIG_SPL_MMC_SUPPORT
 #define CONFIG_BOOTCOMMAND "mmc read 0x80600000 0x1800 0x3000; bootm 0x80600000"
 #endif  /* CONFIG_SPL_MMC_SUPPORT */
 
 #ifdef CONFIG_SFC_NOR
 #ifdef CONFIG_OF_LIBFDT
	 #define CONFIG_BOOTCOMMAND "sf0 probe;sf0 read 0x80600000 0x40000 0x280000;sf0 read 0x83000000 0x540000 0x10000;bootm 0x80600000 - 0x83000000"
 #else
	 #define CONFIG_BOOTCOMMAND "sf0 probe;sf0 read 0x80600000 0x40000 0x280000;bootm 0x80600000"
 #endif
 #endif /* CONFIG_SFC_NOR */
 
 #ifdef CONFIG_SFC_NAND
	 #define CONFIG_BOOTCOMMAND "nand read 0x80600000 0x100000 0x300000;bootm 0x80600000"
 #endif /* CONFIG_SFC_NAND */

 
 //////////////////////

 /**
 * Boot arguments definitions.
 */
#if defined(CONFIG_T41N) || defined(CONFIG_T41NQ)
#define BOOTARGS_COMMON "console=ttyS1,115200n8 mem=48M@0x0 rmem=64M@0x3000000 nmem=16M@0x7000000"
#elif defined(CONFIG_T41L) || defined(CONFIG_T41LQ)
#define BOOTARGS_COMMON "console=ttyS1,115200n8 mem=32M@0x0 rmem=32M@0x2000000"
#elif defined(CONFIG_T41A) || defined(CONFIG_T41ZX) || defined(CONFIG_T41XQ)
#define BOOTARGS_COMMON "console=ttyS1,115200n8 mem=100M@0x0 rmem=128M@0x6400000 nmem=28M@0xE400000"
#else
#define BOOTARGS_COMMON "console=ttyS1,115200n8 mem=64M@0x0 rmem=64M@0x4000000"
#endif


#endif


#endif /* __CONFIG_ISVP_T41_H__ */