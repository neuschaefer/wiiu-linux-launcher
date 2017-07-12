/*
 * Wii U Linux Launcher -- Settings
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

#include <stddef.h>
#include <string.h>
#include <os_functions.h>
#include "main.h"
#include "fs.h"

char kernel_path[256];
char dtb_path[256];
char initrd_path[256];
char cmdline[256];

static void load_default_settings(void)
{
	snprintf(kernel_path, sizeof kernel_path,
			"%s/wiiu/linux/zImage", sdcard_path);
	snprintf(dtb_path, sizeof dtb_path,
			"%s/wiiu/linux/wiiu.dtb", sdcard_path);
	snprintf(initrd_path, sizeof initrd_path, "", 0);
	snprintf(cmdline, sizeof cmdline, "", 0);
}

static void get_config_path(char *buf, size_t size)
{
	snprintf(buf, size, "%s/wiiu/apps/linux/config.txt", sdcard_path);
}

/* Find the first newline or return NULL */
static char *find_newline(char *str)
{
	char *p;

	for (p = str; *p; p++)
		if (*p == '\n' || *p == '\r')
			return p;

	return NULL;
}

/* Does a start with b? */
static int starts_with(const char *string, const char *pattern)
{
	size_t i;

	for (i = 0;; i++) {
		if (pattern[i] == '\0')
			return 1;
		if (string[i] != pattern[i])
			return 0;
	}
}

static void parse_settings(char *string)
{
	char *line, *next = string;
	char *newline;

	while (next) {
		line = next;

		newline = find_newline(line);
		if (newline) {
			*newline = '\0';
			next = newline + 1;
		} else {
			next = NULL;
		}

		if (starts_with(line, "kernel="))
			snprintf(kernel_path, sizeof kernel_path, "%s", line + 7);
		else if (starts_with(line, "dtb="))
			snprintf(dtb_path, sizeof dtb_path, "%s", line + 4);
		else if (starts_with(line, "initrd="))
			snprintf(initrd_path, sizeof initrd_path, "%s", line + 7);
		else if (starts_with(line, "cmdline="))
			snprintf(cmdline, sizeof cmdline, "%s", line + 8);

		/* ignore everything else */
	}
}

static int try_load_settings(void)
{
	int res;
	char path[256], buf[1024];

	get_config_path(path, sizeof path);
	res = get_file_size(path, NULL);
	if (res == 0)
		return -1;

	res = read_file_into_buffer(path, (u8 *)buf, sizeof(buf) - 1, NULL);
	if (res < 0)
		return res;
	buf[res] = '\0';

	parse_settings(buf);

	return 0;
}

void load_settings(void)
{
	int res = try_load_settings();

	if (res < 0)
		load_default_settings();
}

void save_settings(void)
{
	char buf[1024], path[256];
	int res;

	get_config_path(path, sizeof buf);
	res = snprintf(buf, sizeof buf,
			"kernel=%s\ndtb=%s\ninitrd=%s\ncmdline=%s\n",
			kernel_path, dtb_path, initrd_path, cmdline);

	write_buffer_into_file(path, (u8 *)buf, res);
}
