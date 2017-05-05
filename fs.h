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

#ifndef _FS_H
#define _FS_H

#include <stddef.h>
#include <fs_defs.h>

extern void fs_init(void);
extern void fs_deinit(void);

extern const char *FS_strerror(int error);
extern void mount_sdcard(void);
extern void unmount_sdcard(void);
extern size_t get_file_size(const char *filename, const char *what);
extern int read_file_into_buffer(const char *filename, u8 *buffer, size_t size,
		const char *what);

#endif
