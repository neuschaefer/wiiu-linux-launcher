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
#include "main.h"
#include "latte.h"
#include "memory.h"
#include "ppc.h"

/* Clear n 32-bit words at p with a given value */
static void memset32(uint32_t *p, uint32_t value, unsigned long n)
{
	unsigned long i;

	for (i = 0; i < n; i++)
		p[i] = value;
}

static void memcpy32(uint32_t *d, const uint32_t *s, unsigned long n)
{
	unsigned long i;

	for (i = 0; i < n; i++)
		d[i] = s[i];
}

void memset(void *ptr, uint8_t value, unsigned long n)
{
	unsigned long i;
	uint8_t *p = ptr;

	for (i = 0; i < n; i++)
		p[i] = value;
}

int memcmp(const void *s1, const void *s2, uint32_t n)
{
	const char *p1 = s1;
	const char *p2 = s2;
	uint32_t i;

	for (i = 0; i < n; i++)
		if (p1[i] != p2[i])
			return p1[i] - p2[i];

	return 0;
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


static uint32_t gettime(void)
{
	return read32(LT_TIMER);
}

/*
 * The ARM runs at 243MHz. LT_TIMER is incremented at 1/128th of that.
 * 243000000Hz / 128 * 1µS = 1.8984375. Let's just make that 2 ;)
 */
void udelay(uint32_t usec)
{
	uint32_t ticks = 2 * usec;
	uint32_t start = gettime();

	while (gettime() - start < ticks);
}


static uint32_t *const fb_drc = (void *)0x00708000;
static const uint32_t stride_drc = 896;

static void put_glyph(uint32_t *fb, unsigned int stride, const uint8_t *glyphs)
{
	int row, col;

	for (row = 0; row < 8; row++) {
		uint8_t glyph = glyphs[row];
		for (col = 0; col < 8; col++) {
			if (glyph & 0x80)
				fb[stride * row + col] = 0x00000000;
			else
				fb[stride * row + col] = 0xffff00ff;
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

static void put_char_xy_drc(int x, int y, char c)
{
	put_char_xy(fb_drc, stride_drc, x, y, c);
}

static void put_str_xy(uint32_t *fb, unsigned stride, int x, int y, const char *str)
{
	int i;

	for (i = 0; str[i]; i++)
		put_char_xy(fb, stride, x+i, y, str[i]);
}

static void put_str_xy_drc(int x, int y, const char *str)
{
	put_str_xy(fb_drc, stride_drc, x, y, str);
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

static void put_hex_xy_drc(int x, int y, uint32_t value)
{
	put_hex_xy(fb_drc, stride_drc, x, y, value);
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

static void fail_with_hex(const char *reason, uint32_t value)
{
	put_str_xy_drc(0x18,                  0x10, reason);
	put_hex_xy_drc(0x18 + strlen(reason), 0x10, value);
}

static void *const WIIU_ANCAST_BASE	= (void *)0x08000000;	/* aka. MEM0-A */
static void *const VWII_ANCAST_BASE	= (void *)0x01330000;

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

	if (dest == WIIU_ANCAST_BASE && dev != 0x11) {
		fail_with_hex("not a Wii U ancast image: ", dev);
		return -1;
	}

	/*
	if (dest == VWII_ANCAST_BASE && dev != 0x13) {
		fail_with_hex("not a vWii ancast image: ", dev);
		return -1;
	}
	*/

	if (size + 0x100 > (2 << 20)) {
		fail_with_hex("ancast image too big: ", size);
		return -1;
	}

	/* Clear the other header area */
	if (dest == WIIU_ANCAST_BASE) {
		memset(VWII_ANCAST_BASE, 0, 0x100);
		dc_flushrange(VWII_ANCAST_BASE, 0x100);
	} else {
		memset(WIIU_ANCAST_BASE, 0, 0x100);
		dc_flushrange(WIIU_ANCAST_BASE, 0x100);
	}

	/* Explicitly use the slightly faster memcpy32 here */
	memcpy32(dest, base, (size + 0x100) / 4);

	/* Make sure that the copied image has reached the RAM. */
	dc_flushrange(dest, size + 0x100);

	return 0;
}

void *const MEM0_MEMCONS = (void *)0x08200000;
void display_memconsole(void)
{
	char *memcons = MEM0_MEMCONS;
	unsigned pos = 0;
	unsigned x = 0x10, y = 0x10;

	while(1) {
		dc_invalidaterange(memcons, 4);
		uint32_t new_pos = *(volatile uint32_t *)memcons;

		if (pos < new_pos)
			/* invalidate the cache before reading the new chars */
			dc_invalidaterange(memcons + pos, new_pos - pos);

		while (pos < new_pos) {
			char c = memcons[4 + pos++];

			switch(c) {
			case '\n':
				y++;
				x = 0x10;
				break;
			case '\0':
				return;
			default:
				put_char_xy_drc(x++, y, c);
				break;
			}
		}
	}
}

/* Overwrite the first instructions with a jump to entry */
void ppc_patch_entry(void *ancast, uint32_t entry)
{
	uint32_t *i = (void *)((uint32_t)ancast + 0x100);

	/* Load the payload's entry point address into the saved PC register */
	i[0] = 0x3c600000 | (entry >> 16);		/* lis r3,     entry@hi	*/
	i[1] = 0x60630000 | (entry & 0xffff);		/* ori r3, r3, entry@lo	*/
	i[2] = 0x7c7a03a6;				/* mtsrr0 r3		*/
	i[3] = 0x38600000;				/* li r3, 0		*/
	i[4] = 0x7c7b03a6;				/* mtsrr1 r3		*/
	i[5] = 0x4c000064;				/* rfi			*/

	dc_flushrange(i, 6 * sizeof(uint32_t));
}

/*
 * Flip the switch to turn on the PPC, then wait for the bootrom to decrypt the
 * first instruction of the provided ancast image, and overwrite it with a jump
 * into our payload.
 */
int ppc_start_and_race(void *ancast, uint32_t entry)
{
	volatile uint32_t *first_insn = (void *)((uint32_t)ancast + 0x100);
	uint32_t old;
	uint32_t start_time = gettime();

	old = *first_insn;

	/* Take the PPC out of its Hard Reset state */
	ppc_reset();

	/* Clear the romstate. Otherwise the PPC bootrom will not function correctly. */
	uint32_t *rom_state = (void *)0x016fffe0, status;
	memset(rom_state, 0, 0x20);
	dc_flushrange(rom_state, 0x20);

	while (1) {
		dc_invalidaterange((uint32_t *)first_insn, 4);

		if (*first_insn != old)
			break;

		/* Did the bootrom report an error? */
		dc_invalidaterange(&rom_state[7], 4);
		status = read32((uint32_t)&rom_state[7]);
		if (status >> 24 != 0)
			return status;

		if (gettime() - start_time > 100000000)
			return -1;
	}

	//ppc_patch_entry(ancast, entry);
	return 0;
}

static void hexdump(void *base, uint32_t length)
{
	uint32_t i, startx = 0x10, x = startx, y = 0x24;
	uint32_t *p = base;

	for (i = 0; i < length / sizeof(uint32_t); i++) {
		put_hex_xy_drc(x, y, p[i]);
		x += 10;
		if (x >= startx + 80) {
			x = startx;
			y++;
		}
	}
}


/* SHA-1 functions (for debugging purposes) */

static void sha1_init(void)
{
	static const uint32_t initdata[5] = {
		0x67452301,
		0xEFCDAB89,
		0x98BADCFE,
		0x10325476,
		0xC3D2E1F0
	};

	/* reset the SHA-1 engine */
	write32(SHA_CTRL, 0);
	udelay(100);

	/* load the magic constants */
	memcpy32((void *)SHA_H(0), initdata, 5);
	udelay(100);
}

static uint32_t sha1_wait(void)
{
	uint32_t status;

	do {
		status = read32(SHA_CTRL);
	} while(status & SHA_CTRL_EXEC);

	return status & SHA_CTRL_ERR;
}

#define MIN(x, y)	(((x) < (y))? (x):(y))
static int sha1_update(char *p, uint32_t size)
{
	uint32_t offset = 0;

	while (size) {
		uint32_t chunk = MIN(0x10000, size);
		uint32_t blocks = (chunk / 64) - 1;
		write32(SHA_SRC, (uint32_t)p + offset);
		write32(SHA_CTRL, blocks | SHA_CTRL_EXEC);
		udelay(100);
		if (sha1_wait())
			return -1;

		size -= chunk;
		offset += chunk;
	}

	return 0;
}

static int sha1_finish(uint32_t size, char *tmp_buf)
{
	//uint8_t block[64];
	char *block = tmp_buf;

	memset(block, 0, 64);

	block[0] = 0x80;

	*(uint32_t *)&block[60] = 8 * size;		/* size in bits */

	return sha1_update(block, 64);
}

static int sha1_doit(char *p, uint32_t size, char *tmp_buf)
{
	int ret;

	sha1_init();
	ret = sha1_update(p, size);
	if (ret < 0)
		return ret;

	return sha1_finish(size, tmp_buf);
}

static void test_mem_range(int y, char *p, char *q)
{
	int i;
	uint32_t size = q - p;

	put_hex_xy_drc(0x10, y, (uint32_t)p);
	put_hex_xy_drc(0x19, y, (uint32_t)q);

	if (sha1_doit(p, size, WIIU_ANCAST_BASE + 0x1fff00) < 0) {
		put_str_xy_drc(0x20, y, "sha1 failed :-(");
		return;
	}

	for (i = 0; i < 5; i++)
		put_hex_xy_drc(0x20 + 10*i, y, read32(SHA_H(i)));
}


static void log_str(int y, const char *str)
{
	put_str_xy_drc(0x10, y, "[....]");
	put_str_xy_drc(0x18, y, str);
}

static void log_done(int y)
{
	put_str_xy_drc(0x11, y, "done");
}

extern uint32_t svc_0x53_arguments[];
int main(void)
{
	int logline = 0x0a;

	memset32(fb_drc, 0xffff00ff, 896 * 504);		/* yellow */
	font_test(fb_drc, stride_drc);

	/* Make sure the PPC stops running before we load the ancast image */
	log_str(logline, "Halting PPC");
	ppc_hang();
	log_done(logline++);

	log_str(logline, "Copying ancast image");
	char *const ancast_dest = WIIU_ANCAST_BASE;
	if (copy_ancast_image((void *)svc_0x53_arguments[1], ancast_dest) < 0)
		return 0;
	log_done(logline++);

	/* write a dummy string into the memconsole */
	char *memcons = MEM0_MEMCONS;
	*(uint32_t *)memcons = 15;
	memcpy(memcons + 4, "Hello world\nfoo", 15);
	dc_flushrange(memcons, 20);

	log_str(logline, "Racing the PPC bootrom");
	int ret = ppc_start_and_race(ancast_dest, svc_0x53_arguments[2]);
	if (ret != 0) {
		fail_with_hex("ppc_start_and_race failed: ", ret);
		return 0;
	}
	log_done(logline++);

	display_memconsole();

	/*
	 * A small memory map:
	 *
	 * region	user		address		size
	 * MEM0		GX		08000000	002e0000 (00300000?)
	 * MEM1		game		00000000	02000000
	 * MEM2		everyone	10000000	80000000
	 * SRAM1	IOSU k. heap	fff00000	00008000
	 * SRAM0	IOSU kernel	ffff0000	00010000
	 */

	return 0;
}
