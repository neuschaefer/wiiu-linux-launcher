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

/* A physically contiguous memory buffer that contains a small header, the
 * kernel, the dtb, and the initrd. Allocated from the end of MEM1. */
static void *contiguous_buffer = NULL;

/* Paths of the files to be loaded */
static char kernel_path[256];
static char dtb_path[256];
static char initrd_path[256];
static char cmdline[256];
static char *current_text = NULL;

/* A warning or error message */
char warning[1024];

static int selection = 0;
static struct keyboard keyboard;
static int keyboard_shown = 0;

static uint8_t *fs_client;
static uint8_t *fs_cmdblock;
static uint8_t *fs_buffer;
#define FS_BUFFER_SIZE 4096
static char sdcard_path[FS_MAX_MOUNTPATH_SIZE];

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

	/* Draw the firmware version in the lower right corner */
	OSScreenPrintf(49, 17, line, "OS_FIRMWARE: %d", OS_FIRMWARE);

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

static const char *FS_strerror(int error)
{
	switch (error) {
		case  0: return "success";
		case -6: return "file not found";
		case -7: return "not a file";
		default: return "unknown";
	}
}

/* Mount the SD card and store its mount point path in sdcard_path */
static void mount_sdcard(void)
{
	char mount_source[FS_MOUNT_SOURCE_SIZE];
	s32 res;

	FSInitCmdBlock(fs_cmdblock);
	res = FSGetMountSource(fs_client, fs_cmdblock, FS_SOURCETYPE_EXTERNAL,
			mount_source, -1);

	if (res < 0) {
		warnf("FSGetMountSource failed: %s (%d)", FS_strerror(res), res);
		sdcard_path[0] = '\0';
		return;
	}

	res = FSMount(fs_client, fs_cmdblock, mount_source, sdcard_path, sizeof
			sdcard_path, -1);
	if (res < 0) {
		warnf("Failed to mount SD card: %s (%d)", FS_strerror(res), res);
		sdcard_path[0] = '\0';
	}

	warnf("SD card mounted at %s", sdcard_path); /* debug */
}

/* (Try to) unmount the SD card again */
static void unmount_sdcard(void)
{
	FSUnmount(fs_client, fs_cmdblock, sdcard_path, -1);
}

static size_t get_file_size(const char *filename, const char *what)
{
	s32 res;
	FSStat stat;

	if (filename[0] == '\0')
		return 0;

	FSInitCmdBlock(fs_cmdblock);
	res = FSGetStat(fs_client, fs_cmdblock, filename, &stat, -1);
	if (res < 0) {
		warnf("Failed to stat %s: %s (%d)", what, FS_strerror(res), res);
		return 0;
	}

	return stat.size;
}

#define MIN(a, b) (((a) < (b))? (a) : (b))
static int read_file_into_buffer(const char *filename, u8 *buffer, size_t size,
		const char *what)
{
	s32 res, handle;
	size_t bytes_read = 0, chunk_size;

	if (filename[0] == '\0')
		return 0;

	warnf("Loading %s...", what);
	draw_gui();

	memset(buffer, 0, size);

	res = FSOpenFile(fs_client, fs_cmdblock, filename, "r", &handle, -1);
	if (res < 0) {
		warnf("Opening %s failed: %s (%d)", what, FS_strerror(res), res);
		return res;
	}

	while (bytes_read < size) {
		/*
		 * NOTE: If the buffer passed to FSReadFile isn't aligned to a
		 * 0x40 byte boundary, FSReadFile will hang! Because of this, I
		 * read the data into an aligned buffer first, and then copy it
		 * into the target buffer.
		 */

		chunk_size = MIN(size - bytes_read, FS_BUFFER_SIZE);
		res = FSReadFile(fs_client, fs_cmdblock, fs_buffer,
				1, chunk_size, handle, 0, -1);

		if (res < 0) {
			warnf("Reading from %s failed: %s (%d)", FS_strerror(res), res);
			FSCloseFile(fs_client, fs_cmdblock, handle, -1);
			return res;
		} else if (res == 0) {
			break;
		} else {
			memcpy(buffer + bytes_read, fs_buffer, res);
			bytes_read += res;
		}
	}

	FSCloseFile(fs_client, fs_cmdblock, handle, -1);
	warn("");

	return bytes_read;
}

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
}

static void boot(void)
{
	if (!contiguous_buffer)
		return;

	warn("booting...");
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
	InitFSFunctionPointers();

	init_screens();
	keyboard_init(&keyboard, 0, 10);

	FSInit();
	fs_client = xmalloc(FS_CLIENT_SIZE, 0x20);
	fs_cmdblock = xmalloc(FS_CMD_BLOCK_SIZE, 0x20);
	fs_buffer = xmalloc(FS_BUFFER_SIZE, 0x40);
	FSAddClient(fs_client, 0xffffffff);
	mount_sdcard();

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

	unmount_sdcard();
	FSDelClient(fs_client);
	xfree(fs_buffer);
	xfree(fs_client);
	xfree(fs_cmdblock);
	FSShutdown();

	return 0;
}
