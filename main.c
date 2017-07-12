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

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <common/common.h>
#include <os_functions.h>
#include <vpad_functions.h>
#include <fs_functions.h>
#include "keyboard.h"
#include "main.h"
#include "fs.h"
#include "settings.h"
#include "version.h"
#include "hax.h"

/* A physically contiguous memory buffer that contains a small header, the
 * kernel, the dtb, and the initrd. Allocated from the end of MEM1. */
static void *contiguous_buffer = NULL;

static char *current_text = NULL;

/* A warning or error message */
char warning[1024];

static int selection = 0;
static struct keyboard keyboard;
static int keyboard_shown = 0;

static int iosuhax = -1;

/* Pointers to the raw framebuffers. [0] is TV, [1] is DRC. */
uint32_t *framebuffers[2];

void *xmalloc(size_t size, size_t alignment)
{
	void *(* MEMAllocFromDefaultHeapEx)(int size, int alignment) =
		(void *) *pMEMAllocFromDefaultHeapEx;
	void *ptr = MEMAllocFromDefaultHeapEx(size, alignment);

	if (!ptr)
		OSFatal("MEMAllocFromDefaultHeapEx failed");

	return ptr;
}

void xfree(void *ptr)
{
	void (* MEMFreeToDefaultHeap)(void *addr) = (void *) *pMEMFreeToDefaultHeap;

	if (ptr)
		MEMFreeToDefaultHeap(ptr);
}

static void enter_keyboard(char *buffer)
{
	current_text = buffer;
	keyboard_shown = 1;
}

static void exit_keyboard(void)
{
	if (keyboard_shown) {
		current_text = NULL;
		save_settings();
		keyboard_shown = 0;
	}
}

/* Print some text to both screens */
static void OSScreenPutFontBoth(uint32_t posX, uint32_t posY, const char *str) {
	OSScreenPutFontEx(0, posX, posY, str);
	OSScreenPutFontEx(1, posX, posY, str);
}

static void OSScreenFlipBuffersBoth(void) {
	OSScreenFlipBuffersEx(0);
	OSScreenFlipBuffersEx(1);
}

static void OSScreenClearBufferBoth(uint32_t color) {
	OSScreenClearBufferEx(0, color);
	OSScreenClearBufferEx(1, color);
}

/* Draw the last line of the screen. NOTE: It's at different line number on the
 * gamepad and on the TV. */
static void draw_status_line(void)
{
	char buf[32];

	/* Draw the program version (git commit) in the lower left corner */
	snprintf(buf, sizeof buf, "Git: %s", program_version);
	OSScreenPutFontEx(0, 0, 27, buf);
	OSScreenPutFontEx(1, 0, 17, buf);

	/* Draw the firmware version in the lower right corner */
	snprintf(buf, sizeof buf, "OS_FIRMWARE: %d", OS_FIRMWARE);
	OSScreenPutFontEx(0, 85, 27, buf);
	OSScreenPutFontEx(1, 49, 17, buf);
}

/*
 *                   Wii U Linux Launcher
 *
 * kernel  : /vol/external01/linux/image
 * dtb     : /vol/external01/linux/wiiu.dtb
 * initrd  : /vol/external01/linux/initrd
 * cmdline : root=/dev/mmcblk0p1
 * load it!
 *
 *
 * Opening kernel failed: file not found (-6)
 *
 *
 * Git: abcd12345678                      OS_FIRMWARE: 550
 */
void draw_gui(void)
{
	char line[128];
	int y;

	OSScreenClearBufferBoth(0x488cd100); /* A nice blue background */

	OSScreenPutFontEx(0, 39, 0, "Wii U Linux Launcher");
	OSScreenPutFontEx(1, 21, 0, "Wii U Linux Launcher");

	y = 2;
	OSScreenPrintf(2, y++, line, "kernel  : %s", kernel_path);
	OSScreenPrintf(2, y++, line, "dtb     : %s", dtb_path);
	OSScreenPrintf(2, y++, line, "initrd  : %s", initrd_path);
	OSScreenPrintf(2, y++, line, "cmdline : %s", cmdline);
	OSScreenPutFontBoth(2, y++, (contiguous_buffer == NULL)?
			"load it!" : "load it! (press start to boot)");

	/* What's currently selected for editing? */
	OSScreenPutFontBoth(0, 2 + selection, "> ");

	/* Show a little cursor, if we're editing a line*/
	if (keyboard_shown) {
		int len = strlen(current_text);
		OSScreenPutFontBoth(12 + len, 2 + selection, "_");
	}

	OSScreenPutFontBoth(0, 9, warning);

	if (keyboard_shown) {
		keyboard_draw(&keyboard);
	}

	draw_status_line();

	OSScreenFlipBuffersBoth();
}

extern uint8_t purgatory[];
extern uint8_t purgatory_end[];

struct purgatory_header {
	uint32_t jmp;		/* A jump instruction, to skip the header */
	uint32_t size;		/* The size of the whole thing */
	uint32_t dtb_phys;	/* physical address of the devicetree blob */
	uint32_t kern_phys;	/* physical address of the kernel */
};

/* Get a chunk of MEM1 */
static void *get_mem1_chunk(size_t size)
{
	/* Perform alignment (I *think* this correct) */
	size = -((-size) & (~0xfff));

	/* Skip the framebuffers */
	if (size > 0x02000000 - OSScreenGetBufferSizeEx(0) - OSScreenGetBufferSizeEx(1)) {
		warnf("ERROR: Can't allocate %#x bytes from MEM1", size);
		return NULL;
	}

	return (void *) (0xf4000000 + 0x02000000 - size);
}

/* https://www.kernel.org/doc/Documentation/devicetree/booting-without-of.txt */
static int load_stuff(void)
{
	size_t purgatory_size = purgatory_end - purgatory;
	size_t total_size;
	uint8_t *buffer;
	int res;

	contiguous_buffer = NULL;

	if (kernel_path[0] == '\0') {
		warn("You need to specify a kernel!");
		return 0;
	}

	size_t kernel_size = get_file_size(kernel_path, "kernel");


	/* TODO: determine the size of initrd and dtb */

	total_size = purgatory_size + kernel_size;
	buffer = get_mem1_chunk(total_size);
	if (!buffer)
		return -1;

	struct purgatory_header *header = (void *)buffer;
	memcpy(header, purgatory, purgatory_size);

	/* TODO: fill the header with all necessary information */

	size_t kernel_offset = purgatory_size;
	res = read_file_into_buffer(kernel_path,
			(u8 *)buffer + kernel_offset, kernel_size, "kernel");
	if (res < 0)
		return res;

	/* TODO: patch initrd and cmdline into dtb */

	/* Let other functions see that we've loaded stuff */
	contiguous_buffer = buffer;

	return 0;
}

/* ARM code \o/ */
#include "arm/arm.xxd"

/* Make sure that the given framebuffer is using the first half as foreground. */
static void set_framebuffer_foreground(int fb)
{
	OSScreenPutPixelEx(fb, 0, 0, 0xa0a0a0a0);
	OSScreenFlipBuffersEx(fb);
	OSScreenPutPixelEx(fb, 0, 0, 0xb0b0b0b0);

	switch(*framebuffers[fb]) {
	case 0xa0a0a0a0:
		/* do nothing */
		break;
	case 0xb0b0b0b0:
		OSScreenFlipBuffersEx(fb);
		break;
	default:
		OSFatal("set_framebuffer_foreground read a wrong color");
	}
}

static void boot(void)
{
	const uint32_t arm_code = 0xfffff000;

	if (!contiguous_buffer)
		return;

	iosuhax = iosuhax_open();
	if (iosuhax < 0)
		return;

	if (arm_bin_len > -arm_code) {
		warnf("Error: The ARM binary is too big (%#x)", arm_bin_len);
		return;
	}

	void *ancast_addr = (void *)0xf5000000;
	void *ppcboot_addr = (void *)0xf4000000;
	memcpy(ppcboot_addr, purgatory, 0x38);

	/* TODO: load the ancast image directly from the NAND filesystem */
	int ret = read_file_into_buffer(
			"/vol/external01/wiiu/apps/linux/ancast.img",
			ancast_addr, 2 << 20, "ancast image");

	if (ret < 0)
		return;

	/* Flush (part of) the cache to ensure that the ancast image hits RAM
	 * before we shutdown the PPC (using svc 0x53) */
	DCFlushRange(ancast_addr, ret);

	warn("loading ARM code into MEM1...");
	draw_gui();
	iosuhax_kern_write_buf(iosuhax, arm_code, arm_bin, arm_bin_len);
	iosuhax_kern_write32(iosuhax, arm_code + 4,
			(uint32_t)OSEffectiveToPhysical(ancast_addr));
	iosuhax_kern_write32(iosuhax, arm_code + 8,
			(uint32_t)OSEffectiveToPhysical(ppcboot_addr));

	warn("booting...");
	/* Draw the GUI twice to make sure both the foreground
	   buffer and the background buffer contain the current state */
	draw_gui();
	draw_gui();

	set_framebuffer_foreground(0);
	set_framebuffer_foreground(1);
	iosuhax_svc_0x53(iosuhax, arm_code);
}

static void action(int what)
{
	warning[0] = '\0';

	switch (what) {
		case 0:
			enter_keyboard(kernel_path);
			break;
		case 1:
			enter_keyboard(dtb_path);
			break;
		case 2:
			enter_keyboard(initrd_path);
			break;
		case 3:
			enter_keyboard(cmdline);
			break;
		case 4:
			load_stuff();
			break;
	}
}

static void append_char(char c)
{
	size_t len = strlen(current_text);

	if (len < sizeof(kernel_path) - 1) {
		current_text[len] = c;
		current_text[len + 1] = '\0';
	}
}

static void remove_char()
{
	size_t len = strlen(current_text);

	if (len)
		current_text[len - 1] = '\0';
}

static void keyboard_cb(struct keyboard *keyb, int ch)
{
	switch (ch) {
		case KEYB_BACKSPACE:
			remove_char();
			break;
		case '\n':
			exit_keyboard();
			break;
		default:
			append_char(ch);
			break;
	}
}

static void handle_vpad(const VPADData *vpad)
{
	if (vpad->btns_d & VPAD_BUTTON_DOWN) {
		exit_keyboard();
		selection++;
	}
	if (vpad->btns_d & VPAD_BUTTON_UP) {
		exit_keyboard();
		selection--;
	}

	if (keyboard_shown) {
		/* Inform the keyboard about the input event, but ignore it otherwise */
		keyboard_handle_vpad(&keyboard, vpad, keyboard_cb);
		return;
	}

	if (vpad->btns_d & VPAD_BUTTON_A)
		action(selection);

	if (vpad->btns_d & VPAD_BUTTON_PLUS)
		boot();

	/* some normalization... */
	if (selection < 0) selection = 0;
	if (selection > 4) selection = 4;
}

static void init_screens(void)
{
	OSScreenInit();

	framebuffers[0] = (void *)0xf4000000;
	framebuffers[1] = (void *)(0xf4000000 + OSScreenGetBufferSizeEx(0));
	OSScreenSetBufferEx(0, framebuffers[0]);
	OSScreenSetBufferEx(1, framebuffers[1]);

	OSScreenEnableEx(0, 1);
	OSScreenEnableEx(1, 1);

	OSScreenClearBufferBoth(0x00000000);
	OSScreenFlipBuffersBoth();
}

int main(void)
{
	InitOSFunctionPointers();
	InitVPadFunctionPointers();
	InitFSFunctionPointers();

	init_screens();
	keyboard_init(&keyboard, 0, 10);

	fs_init();

	load_settings();

	uint32_t color = 0, i;
	for (i = 0; i < 8; i++) {
		OSScreenClearBufferBoth(color);
		color += 0x11223344;

		OSScreenFlipBuffersBoth();

		os_usleep(100000);
	}

	for (;;) {
		VPADData vpad;
		s32 err;

		/* Read input events from the gamepad */
		VPADRead(0, &vpad, 1, &err);

		if (vpad.btns_h & VPAD_BUTTON_HOME)
			break;

		handle_vpad(&vpad);

		draw_gui();

		os_usleep(1000000 / 50);
	}

	fs_deinit();

	return 0;
}
