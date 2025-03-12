#ifndef __CONFIG_ISVP_A1_H__
#define __CONFIG_ISVP_A1_H__

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



#endif /* __CONFIG_ISVP_A1_H__ */