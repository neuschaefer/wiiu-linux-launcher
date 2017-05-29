# Wii U Linux Launcher -- Makefile code that runs on the ARM
#
# Copyright (C) 2017  Jonathan Neuschäfer <j.neuschaefer@gmx.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program, in the file LICENSE.GPLv2.

ifeq ($(strip $(DEVKITARM)),)
PREFIX := arm-none-eabi-
else
PATH := $(DEVKITARM)/bin:$(PATH)
PREFIX := arm-none-eabi-
endif

CC := $(PREFIX)gcc
AS := $(CC)
LD := $(PREFIX)ld
OBJCOPY := $(PREFIX)objcopy

CFLAGS := -mbig-endian
LDFLAGS := --as-needed -T link.ld -EB -e svc_0x53_arguments
ASFLAGS := -mbig-endian

OBJS=\
	start.o

all: arm.bin

arm.xxd: arm.bin
	xxd -i $< > $@

arm.bin: arm.elf
	$(OBJCOPY) $< -O binary $@

arm.elf: $(OBJS) link.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@
