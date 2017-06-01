/*
 * Wii U Linux Launcher -- Font declaration
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

#define FONT_EVERYTHING 0

#if FONT_EVERYTHING
#define FONT_OFFSET	0
#define FONT_END	0x100
#else
#define FONT_OFFSET	0x20
#define FONT_END	0x80
#endif

#define FONT_GLYPHS	(FONT_END - FONT_OFFSET)

extern const unsigned char font[FONT_GLYPHS * 8];
