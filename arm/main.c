/*
 * Wii U Linux Launcher -- Code that runs on the ARM
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
#include "font.h"

/* Clear n 32-bit words at p with a given value */
static void memset32(uint32_t *p, uint32_t value, unsigned long n)
{
	unsigned long i;

	for (i = 0; i < n; i++)
		p[i] = value;
}

static void put_glyph(uint32_t *fb, unsigned int stride, const uint8_t *glyphs)
{
	int row, col;

	for (row = 0; row < 8; row++) {
		uint8_t glyph = glyphs[row];
		for (col = 0; col < 8; col++) {
			if (glyph & 0x80)
				fb[stride * row + col] = 0x00000000;
			glyph <<= 1;
		}
	}
}

static void put_char(uint32_t *fb, unsigned stride, char c)
{
	uint32_t index = (uint8_t)c;

	if (index >= FONT_OFFSET && index < FONT_END)
		put_glyph(fb, stride, &font[(index - FONT_OFFSET) * 8]);
}

static void put_char_xy(uint32_t *fb, unsigned stride, int x, int y, char c)
{
	put_char(fb + 8*y*stride + 8*x, stride, c);
}

static void put_str_xy(uint32_t *fb, unsigned stride, int x, int y, const char *str)
{
	int i;

	for (i = 0; str[i]; i++)
		put_char_xy(fb, stride, x+i, y, str[i]);
}

static void put_hex_xy(uint32_t *fb, unsigned stride, int x, int y, uint32_t value)
{
	int i;
	char buf[9];
	const char *hex = "0123456789abcdef";

	for (i = 0; i < 8; i++) {
		buf[7-i] = hex[value & 0xf];
		value >>= 4;
	}
	buf[8] = '\0';

	put_str_xy(fb, stride, x, y, buf);
}

void font_test(uint32_t *fb, unsigned int stride)
{
	int x, y;

	for (x = 0; x < 0x6b; x += 16)
	for (y = 0; y < 0x3c; y++) {
		char buf[] = "hello XX,YY";
		const char *hex = "0123456789abcdef";

		if (x >= 0x10 && x < 0x60 && y > 8 && y < 0x34)
			continue;

		buf[6] = hex[(x >> 4) & 0xf];
		buf[7] = hex[(x >> 0) & 0xf];

		buf[9] = hex[(y >> 4) & 0xf];
		buf[10]= hex[(y >> 0) & 0xf];

		put_str_xy(fb, stride, x, y, buf);
	}
}

static unsigned strlen(const char *s)
{
	unsigned i = 0;

	while(s[i])
		i++;

	return i;
}

void *memcpy(void *dest, const void *src, unsigned n)
{
	char *d = dest;
	const char *s = src;
	unsigned i;

	for (i = 0; i < n; i++)
		d[i] = s[i];

	return dest;
}

static void fail_with_hex(const char *reason, uint32_t value)
{
	uint32_t *fb = (void *)0x00708000;
	put_str_xy(fb, 896, 0x18,                  0x10, reason);
	put_hex_xy(fb, 896, 0x18 + strlen(reason), 0x10, value);
}

static int copy_ancast_image(void *base, void *dest)
{
	uint32_t magic = *(uint32_t *)base;
	uint32_t type  = *(uint32_t *)(base + 0x20);
	uint32_t dev   = *(uint32_t *)(base + 0xa4);
	uint32_t size  = *(uint32_t *)(base + 0xac);

	if (magic != 0xefa282d9) {
		fail_with_hex("invalid ancast magic: ", magic);
		return -1;
	}

	if (type != 1) {
		fail_with_hex("not a PPC ancast image: ", type);
		return -1;
	}

	if (dest == (void *)0x08000000 && dev != 0x11) {
		fail_with_hex("not a Wii U ancast image: ", dev);
		return -1;
	}

	if (dest == (void *)0x01330000 && dev != 0x13) {
		fail_with_hex("not a vWii ancast image: ", dev);
		return -1;
	}

	if (size + 0x100 > (2 << 20)) {
		fail_with_hex("ancast image too big: ", size);
		return -1;
	}

	memcpy(dest, base, size + 0x100);

	return 0;
}

void display_memconsole(void)
{
	char *MEM0_B = (void *)0x08100000;
	unsigned pos = 0;
	unsigned x = 0x10, y = 0x10;

	while(1) {
		//dc_flushentry(MEM0_B);
		uint32_t new_pos = *(volatile uint32_t *)MEM0_B;

		while (pos < new_pos) {
			char c = MEM0_B[4 + pos++];

			switch(c) {
			case '\n':
				y++;
				x = 0x10;
				break;
			case '\0':
				return;
			default:
				put_char_xy((void *)0x00708000, 896, x++, y, c);
				break;
			}
		}
	}
}

extern uint32_t svc_0x53_arguments[];
int main(void)
{
	uint32_t *fb = (uint32_t *)0x00708000;

	memset32(fb, 0xffff00ff, 896 * 504);	/* yellow */
	font_test(fb, 896);

	char *MEM0_B = (void *)0x08100000;
	*(uint32_t *)MEM0_B = 16;
	memcpy(MEM0_B + 4, "Hello world\nfoo\0", 16);
	display_memconsole();

	/*
	 * region	user		address		size
	 * MEM0		GX		08000000	002e0000 (00300000?)
	 * MEM1		game		00000000	02000000
	 * MEM2		everyone	10000000	80000000
	 * SRAM1	IOSU k. heap	fff00000	00008000
	 * SRAM0	IOSU kernel	ffff0000	00010000
	 */

	return 0;
}
