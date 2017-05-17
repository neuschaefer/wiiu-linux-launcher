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

#include <os_functions.h>
#include "fs.h"
#include "hax.h"
#include "main.h"

/* Open /dev/iosuhax and return an FD or an error (negative) */
int iosuhax_open(void)
{
	int ret;

	ret = IOS_Open("/dev/iosuhax", 0);
	if (ret < 0)
		warnf("/dev/iosuhax: %s (%d). Please start Mocha",
				FS_strerror(ret), ret);

	return ret;
}

/* Try to close /dev/iosuhax again */
void iosuhax_close(int fd)
{
	IOS_Close(fd);
}

/* IOCTLs that /dev/iosuhax responds to */
#define IOCTL_MEM_WRITE             0x00
#define IOCTL_MEM_READ              0x01
#define IOCTL_SVC                   0x02
#define IOCTL_KILL_SERVER           0x03
#define IOCTL_MEMCPY                0x04
#define IOCTL_REPEATED_WRITE        0x05
#define IOCTL_KERN_READ32           0x06
#define IOCTL_KERN_WRITE32          0x07

/* Perform syscall 0x81, the iosuhax kernel backdoor */
static int iosuhax_svc81(int fd, uint32_t command, int arg1, int arg2, int arg3)
{
	int req_buf[5] = {
		0x81,
		command,
		arg1,
		arg2,
		arg3
	};

	int resp_buf[1];

	int ret;

	ret = IOS_Ioctl(fd, IOCTL_SVC, req_buf, sizeof req_buf, resp_buf,
			sizeof resp_buf);
	if (ret < 0)
		return ret;

	return resp_buf[0];
}

/* Commands that the kernel hook at svc 0x81 responds to */
#define KERNEL_READ32           1
#define KERNEL_WRITE32          2
#define KERNEL_MEMCPY           3
#define KERNEL_GET_CFW_CONFIG   4

uint32_t iosuhax_kern_read32(int fd, uint32_t address)
{
	return iosuhax_svc81(fd, KERNEL_READ32, address, 0, 0);
}

int iosuhax_kern_write32(int fd, uint32_t address, uint32_t value)
{
	return iosuhax_svc81(fd, KERNEL_WRITE32, address, value, 0);
}

void iosuhax_kern_write_buf(int fd, uint32_t dst, const void *src, size_t size)
{
	size_t i;
	uint32_t s = (uint32_t) src;

	for (i = 0; i < size; i += 4) {
		uint32_t value = *(uint32_t *)(s + i);
		iosuhax_kern_write32(fd, dst + i, value);
	}
}

void iosuhax_svc_0x53(int fd, uint32_t addr)
{
	int req_buf[2] = {
		0x53,
		addr
	};

	int resp_buf[1];

	IOS_Ioctl(fd, IOCTL_SVC, req_buf, sizeof req_buf, resp_buf,
			sizeof resp_buf);
}
