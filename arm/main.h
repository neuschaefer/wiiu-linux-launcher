/*
 * Wii U Linux Launcher -- Header file for main.c
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

#include <stdint.h>

#ifndef MAIN_H
#define MAIN_H

extern void udelay(uint32_t usec);

#define write32(addr, value)					\
	do { *(volatile uint32_t *)(addr) = (value); } while(0)

#define read32(addr)						\
	(*(volatile uint32_t *)(addr))

#define clear32(addr, mask)					\
	do { *(volatile uint32_t *)(addr) &= ~(mask); } while(0)

#define set32(addr, mask)					\
	do { *(volatile uint32_t *)(addr) |= (mask); } while(0)

#define mask32(addr, clear, set)				\
	do {							\
		uint32_t addr_ = (addr);			\
		uint32_t value = read32(addr_);			\
		write32(addr_, (value & ~clear) | set);		\
	} while(0);						\

#endif
