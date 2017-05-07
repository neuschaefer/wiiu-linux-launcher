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

#ifndef _SETTINGS_H
#define _SETTINGS_H

/* Paths of the files to be loaded */
extern char kernel_path[256];
extern char dtb_path[256];
extern char initrd_path[256];
extern char cmdline[256];

extern void load_settings(void);
extern void save_settings(void);

#endif
