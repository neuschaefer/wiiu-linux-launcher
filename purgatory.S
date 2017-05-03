/*
 * Wii U Linux Launcher
 * purgatory.S: The code that runs after CafeOS and before the new kernel.
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

.globl purgatory
.globl purgatory_end
purgatory:
	/* keep this part in sync with struct purgatory in main.c */
	b body
size:	.long 0
dtb:	.long 0
kern:	.long 0

body:
	b body

purgatory_end:
