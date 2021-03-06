# Wii U Linux Launcher
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

ifeq ($(strip $(DEVKITPPC)),)
PREFIX := powerpc-linux-gnu-
else
PATH := $(DEVKITPPC)/bin:$(PATH)
PREFIX := powerpc-eabi-
endif

CC := $(PREFIX)gcc
AS := $(PREFIX)as
LD := $(PREFIX)ld

CFLAGS := -ffreestanding -I dynamic_libs -I include -O2 -mcpu=750 -meabi -Wall
LDFLAGS := --as-needed -T link.ld --nmagic
ASFLAGS := -mregnames

OBJS=\
	crt0.o \
	dynamic_libs/fs_functions.o \
	dynamic_libs/os_functions.o \
	dynamic_libs/sys_functions.o \
	dynamic_libs/vpad_functions.o \
	fs.o \
	hax.o \
	keyboard.o \
	main.o \
	purgatory.o \
	settings.o \
	string.o \
	version.o \

all: linux.elf meta/meta.xml

linux.elf: $(OBJS) link.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

main.o: arm/arm.xxd

arm/arm.xxd:
	$(MAKE) -C arm arm.xxd

meta/meta.xml: meta/meta.xml.sh
	$< > $@

version.c: version.c.sh
	./version.c.sh > $@

clean:
	rm -f linux.elf meta/meta.xml *.o version.c dynamic_libs/*.o
	$(MAKE) -C arm clean

.PHONY: version.c meta/meta.xml arm/arm.xxd clean
