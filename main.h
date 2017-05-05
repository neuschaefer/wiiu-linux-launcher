/*
 * Wii U Linux Launcher
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

#ifndef _MAIN_H
#define _MAIN_H

extern void *xmalloc(size_t size, size_t alignment);
extern void xfree(void *ptr);

/* A warning or error message */
extern char warning[1024];

#define snprintf(buf, size, fmt, ...) \
	__os_snprintf(buf, size, fmt, __VA_ARGS__)

#define OSScreenPrintf(x, y, buf, fmt, ...) \
	snprintf(buf, sizeof(buf), fmt, __VA_ARGS__); \
	OSScreenPutFontBoth(x, y, buf)

#define warnf(fmt, ...) \
	snprintf(warning, sizeof(warning), fmt, __VA_ARGS__)

#define warn(fmt) \
	warnf(fmt, 0)

extern void draw_gui(void);

#endif
