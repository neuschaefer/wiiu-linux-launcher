/*
 * Wii U Linux Launcher -- Latte register definitions
 *
 * Copyright (C) 2017  Jonathan Neuschäfer <j.neuschaefer@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program, in the file LICENSE.GPLv2.
 */

#ifndef LATTE_H
#define LATTE_H

#define LT_TIMER		0x0d800010
#define LT_MEMIRR		0x0d800060
#define LT_AHBPROT		0x0d800064
#define LT_EXICTRL		0x0d800070
#define LT_RESETS_COMPAT	0x0d800194
#define LT_IOPOWER		0x0d8001dc
#define LT_COMPAT_MEMCTRL_STATE	0x0d8005b0
#define LT_RESETS		0x0d8005e0
#define LT_60XE_CFG		0x0d800640

#define SHA_CTRL		0x0d030000
#define   SHA_CTRL_EXEC		0x80000000
#define   SHA_CTRL_IRQ		0x40000000
#define   SHA_CTRL_ERR		0x20000000
#define SHA_SRC			0x0d030004
#define SHA_H(x)		(0x0d030008 + 4*x)

#endif
