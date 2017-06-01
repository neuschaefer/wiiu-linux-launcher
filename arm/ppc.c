/*
 *  From:
 *      minute - a port of the "mini" IOS replacement for the Wii U.
 *
 *      Copyright (C) 2016          SALT
 *      Copyright (C) 2016          Daz Jones <daz@dazzozo.com>
 *
 *  This code is licensed to you under the terms of the GNU GPL, version 2;
 *  see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 */

#include "main.h"
#include "latte.h"

#include <string.h>

void ppc_hang(void)
{
    clear32(LT_RESETS_COMPAT, 0x230);
    udelay(100);
}

void ppc_reset(void)
{
    ppc_hang();

    mask32(LT_COMPAT_MEMCTRL_STATE, 0xFFE0022F, 0x8100000);

    clear32(LT_60XE_CFG, 8);
    mask32(LT_60XE_CFG, 0x1C0000, 0x20000);
    set32(LT_60XE_CFG, 0xC000);

    set32(LT_RESETS_COMPAT, 0x200);
    clear32(LT_COMPAT_MEMCTRL_STATE, 0x20);

    set32(LT_RESETS_COMPAT, 0x30);
}

/* ppc_race, ppc_jump_stub, ppc_wait_stub, ppc_prepare, ppc_jump omitted. */
