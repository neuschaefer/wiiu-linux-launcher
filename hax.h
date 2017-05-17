/*
 * Wrapper code for kernel/service patches
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

#ifndef _HAX_H
#define _HAX_H

#include <stdint.h>

extern int iosuhax_open(void);
extern void iosuhax_close(int fd);

extern uint32_t iosuhax_kern_read32(int fd, uint32_t address);
extern int iosuhax_kern_write32(int fd, uint32_t address, uint32_t value);
extern void iosuhax_kern_write_buf(int fd, uint32_t dst, const void *src, size_t size);
extern void iosuhax_svc_0x53(int fd, uint32_t addr);

#endif
