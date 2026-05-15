/*
 * =====================================================================================
 *
 *       Filename:  LPDDR2_W97AH6N1B_1J.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020年09月21日 17时55分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */

#ifndef __LPDDR2_W97AH6N1B_1J_H__
#define __LPDDR2_W97AH6N1B_1J_H__

static inline void LPDDR2_W97AH6N1B_1J_init(void *data)
{
    struct ddr_chip_info *c = (struct ddr_chip_info *)data;

    c->DDR_ROW          = 13,
    c->DDR_COL          = 10,
    c->DDR_BANK8        = 1,
    c->DDR_BL           = 8,

    c->DDR_RL           = -1,
    c->DDR_WL           = -1,

    c->DDR_tMRW         = DDR__tck(5);
    c->DDR_tDQSCK       = DDR__ps(2500);
    c->DDR_tDQSCKMAX    = DDR__ps(5500);
    c->DDR_tRAS         = DDR_SELECT_MAX__tCK_ps(3, 70 * 1000);
    c->DDR_tRTP         = DDR_SELECT_MAX__tCK_ps(2, 7500);
    c->DDR_tRP          = DDR_SELECT_MAX__tCK_ps(3, 18750);
    c->DDR_tRCD         = DDR_SELECT_MAX__tCK_ps(3, 18750);
    c->DDR_tRC          = DDR_SELECT_MAX__tCK_ps(5, 52500);
    c->DDR_tRRD         = DDR_SELECT_MAX__tCK_ps(2, 10000);
    c->DDR_tWR          = DDR_SELECT_MAX__tCK_ps(3, 15000);
    c->DDR_tWTR         = DDR_SELECT_MAX__tCK_ps(2, 7500);
    c->DDR_tCCD         = DDR__tck(2);
    c->DDR_tFAW         = DDR_SELECT_MAX__tCK_ps(8, 50000);

    c->DDR_tRFC         = DDR__ns(110);
    c->DDR_tREFI        = DDR__ns(7800);

    c->DDR_tCKE         = DDR__tck(3);
    c->DDR_tCKESR       = DDR_SELECT_MAX__tCK_ps(3, DDR__ns(15));
    c->DDR_tXSR         = DDR_SELECT_MAX__tCK_ps(2, c->DDR_tRFC + DDR__ns(10));
    c->DDR_tXP          = DDR_SELECT_MAX__tCK_ps(2, 7500);

}


#ifndef CONFIG_LPDDR2_W97AH6N1B_1J_MEM_FREQ
#define CONFIG_LPDDR2_W97AH6N1B_1J_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#define LPDDR2_W97AH6N1B_1J {                    \
    .name = "W97AH6N1B_1J",                    \
    .id   = DDR_CHIP_ID(VENDOR_WINBOND, TYPE_LPDDR2, MEM_128M),    \
    .type = LPDDR2,                        \
    .freq = CONFIG_LPDDR2_W97AH6N1B_1J_MEM_FREQ,            \
    .size = 128,                        \
    .init = LPDDR2_W97AH6N1B_1J_init,                \
}

#endif
