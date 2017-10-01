/*
 * Filesystem functions
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

#include <os_functions.h>
#include <fs_functions.h>
#include <string.h>
#include "main.h"
#include "fs.h"

#define FS_BUFFER_SIZE 4096
static FSClient *fs_client;
static FSCmdBlock *fs_cmdblock;
static uint8_t *fs_buffer;
char sdcard_path[FS_MAX_MOUNTPATH_SIZE];

void fs_init(void)
{
	FSInit();
	fs_client = xmalloc(sizeof(*fs_client), 0x20);
	fs_cmdblock = xmalloc(sizeof(*fs_cmdblock), 0x20);
	fs_buffer = xmalloc(FS_BUFFER_SIZE, FS_IO_BUFFER_ALIGN);
	FSAddClient(fs_client, 0xffffffff);
	mount_sdcard();
}

void fs_deinit(void)
{
	unmount_sdcard();
	FSDelClient(fs_client);
	xfree(fs_buffer);
	xfree(fs_client);
	xfree(fs_cmdblock);
	FSShutdown();
}

const char *FS_strerror(int error)
{
	switch (error) {
		case  0: return "success";
		case -6: return "file not found";
		case -7: return "not a file";
		default: return "unknown";
	}
}

/* Mount the SD card and store its mount point path in sdcard_path */
void mount_sdcard(void)
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
		return;
	}

	if (strcmp(sdcard_path, "/vol/external01") != 0)
		warnf("SD card mounted at %s", sdcard_path);
}

/* (Try to) unmount the SD card again */
void unmount_sdcard(void)
{
	FSUnmount(fs_client, fs_cmdblock, sdcard_path, -1);
}

size_t get_file_size(const char *filename, const char *what)
{
	s32 res;
	FSStat stat;

	if (filename[0] == '\0')
		return 0;

	FSInitCmdBlock(fs_cmdblock);
	res = FSGetStat(fs_client, fs_cmdblock, filename, &stat, -1);
	if (res < 0) {
		if (what)
			warnf("Failed to stat %s: %s (%d)", what,
					FS_strerror(res), res);
		return 0;
	}

	return stat.size;
}

#define MIN(a, b) (((a) < (b))? (a) : (b))
int read_file_into_buffer(const char *filename, u8 *buffer, size_t size,
		const char *what)
{
	s32 res, handle;
	size_t bytes_read = 0, chunk_size;

	if (filename[0] == '\0')
		return 0;

	if (what) {
		warnf("Loading %s...", what);
		draw_gui();
	}

	memset(buffer, 0, size);

	res = FSOpenFile(fs_client, fs_cmdblock, filename, "r", &handle, -1);
	if (res < 0) {
		if (what)
			warnf("Opening %s failed: %s (%d)", what,
					FS_strerror(res), res);
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
			if (what)
				warnf("Reading from %s failed: %s (%d)",
						FS_strerror(res), res);
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
	if (what)
		warn("");

	return bytes_read;
}

int write_buffer_into_file(const char *filename, u8 *buffer, size_t size)
{
	s32 res, handle;
	size_t bytes_written = 0;

	res = FSOpenFile(fs_client, fs_cmdblock, filename, "w", &handle, -1);
	if (res < 0)
		return res;

	while (bytes_written < size) {
		size_t chunk_size = MIN(FS_BUFFER_SIZE, size - bytes_written);

		memcpy(fs_buffer, buffer + bytes_written, chunk_size);
		res = FSWriteFile(fs_client, fs_cmdblock, fs_buffer,
				1, size - bytes_written, handle, 0, -1);
		if (res < 0)
			return res;
		else if (res == 0)
			break;
		else
			bytes_written += res;
	}

	FSCloseFile(fs_client, fs_cmdblock, handle, -1);

	return bytes_written;
}
