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
#include "keyboard.h"

enum file_state {
	STATE_NONE,	/* No file provided */
	STATE_VALID,	/* The file exists and can be loaded */
	STATE_INVALID	/* The file can't be loaded for some reason */
};

/* File validity states */
static enum file_state kernel_state, dtb_state, initrd_state;


/* A physically contiguous memory buffer that contains a small header, the
 * kernel, the dtb, and the initrd. Allocated with OSAllocFromSystem. */
static void *contiguous_buffer;

/* Paths of the files to be loaded */
static char kernel_path[256];
static char dtb_path[256];
static char initrd_path[256];
static char cmdline[256];
static char *current_text = NULL;

/* A warning or error message */
static char warning[1024];

static int selection = 0;
static struct keyboard keyboard;
static int keyboard_shown = 0;

static void enter_keyboard(char *buffer)
{
	current_text = buffer;
	keyboard_shown = 1;
}

static void exit_keyboard(void)
{
	current_text = NULL;
	keyboard_shown = 0;
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

#define snprintf(buf, size, fmt, ...) \
	__os_snprintf(buf, size, fmt, __VA_ARGS__)

#define OSScreenPrintf(x, y, buf, fmt, ...) \
	snprintf(buf, sizeof(buf), fmt, __VA_ARGS__); \
	OSScreenPutFontBoth(x, y, buf)

#define warnf(fmt, ...) \
	snprintf(warning, sizeof(warning), fmt, __VA_ARGS__)

/*
 * Wii U Linux Launcher
 *
 * kernel  : sd:/linux/image
 * dtb     : sd:/linux/wiiu.dtb
 * initrd  : sd:/linux/initrd
 * cmdline : root=/dev/mmcblk0p1
 *
 * ERROR: kernel not found
 *
 *          [ (A) LOAD ]  [ (X) BOOT ]
 */
void draw_gui(void)
{
	char line[128];
	int y, i;

	OSScreenClearBufferBoth(0x488cd100); /* A nice blue background */

	OSScreenPutFontBoth(21, 0, "Wii U Linux Launcher");

	y = 2;
	OSScreenPrintf(2, y++, line, "kernel  : %s", kernel_path);
	OSScreenPrintf(2, y++, line, "dtb     : %s", dtb_path);
	OSScreenPrintf(2, y++, line, "initrd  : %s", initrd_path);
	OSScreenPrintf(2, y++, line, "cmdline : %s", cmdline);
	OSScreenPutFontBoth(2, y++, "load it!");

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

	/* Draw the firmware version in the lower right corner */
	OSScreenPrintf(49, 17, line, "OS_FIRMWARE: %d", OS_FIRMWARE);

	OSScreenFlipBuffersBoth();
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
			warnf("Loading not yet implemented", 0);
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
	/*
	if (vpad->tpdata.touched && vpad->tpdata.invalid == 0) {
		warnf("touched: (%d, %d)", vpad->tpdata.x, vpad->tpdata.y);
	}
	*/

	if (keyboard_shown) {
		/* Inform the keyboard about the input event, but ignore it otherwise */
		keyboard_handle_vpad(&keyboard, vpad, keyboard_cb);
		return;
	}

	if (vpad->btns_d & VPAD_BUTTON_DOWN)
		selection++;
	if (vpad->btns_d & VPAD_BUTTON_UP)
		selection--;
	if (vpad->btns_d & VPAD_BUTTON_A)
		action(selection);

	/* some normalization... */
	if (selection < 0) selection = 0;
	if (selection > 4) selection = 4;
}


struct purgatory {
	uint32_t jmp;	/* A jump instruction, to skip the header */
	uint32_t purgatory_size;
};

static size_t get_file_size(const char *filename)
{
	return 0x1000;
}

/* FIXME! */
void *purgatory_template = NULL;
size_t purgatory_size = 0;

/* https://www.kernel.org/doc/Documentation/devicetree/booting-without-of.txt */
static int load_stuff(void)
{
	size_t purgatory_size = 0x1000 /*TODO*/;
	size_t kernel_size    = get_file_size(kernel_path);
	size_t dtb_size       = get_file_size(dtb_path);
	size_t initrd_size    = get_file_size(initrd_path);
	size_t total_size     = purgatory_size + kernel_size + dtb_size + initrd_size;

	if (contiguous_buffer) {
		OSFreeToSystem((int)contiguous_buffer);
		contiguous_buffer = NULL;
	}

	contiguous_buffer = (void *)OSAllocFromSystem(total_size, 16);
	if (!contiguous_buffer)
		return -1;

	struct purgatory *purgatory = contiguous_buffer;
	memcpy(purgatory, purgatory_template, purgatory_size);
	purgatory->purgatory_size = purgatory_size;

	/* TODO: memcpy the other stuff */
}

static void init_screens(void)
{
	OSScreenInit();

	OSScreenSetBufferEx(0, (void *)0xF4000000);
	OSScreenSetBufferEx(1, (void *)(0xF4000000 + OSScreenGetBufferSizeEx(0)));

	OSScreenEnableEx(0, 1);
	OSScreenEnableEx(1, 1);

	OSScreenClearBufferBoth(0x00000000);
	OSScreenFlipBuffersBoth();
}

int main(void)
{
	InitOSFunctionPointers();
	InitVPadFunctionPointers();

	init_screens();
	keyboard_init(&keyboard, 0, 10);

	uint32_t color = 0, i;
	for (i = 0; i < 8; i++) {
		OSScreenClearBufferBoth(color);
		color += 0x11223344;

		OSScreenFlipBuffersBoth();

		usleep(100000);
	}

	for (;;) {
		VPADData vpad;
		int err;

		/* Read input events from the gamepad */
		VPADRead(0, &vpad, 1, &err);

		if (vpad.btns_h & VPAD_BUTTON_HOME)
			break;

		handle_vpad(&vpad);

		draw_gui();

		usleep(1000000 / 50);
	}

	return 0;
}
