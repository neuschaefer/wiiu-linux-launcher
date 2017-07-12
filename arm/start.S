/*
 * Wii U Linux Launcher -- Code that runs on the ARM
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

#define LT_TIMER	0x0d800010
#define LT_GPIOE_OUT	0x0d8000c0
#define LT_GPIO_OWNER	0x0d8000fc

.globl svc_0x53_arguments
svc_0x53_arguments:
	.long	0x10			@ offset to code
	.long	0			@ unknown, stores pointer to ancast image
	.long	0			@ unknown, stores pointer to ppc payload
	.long	0			@ overwritten by the IOSU kernel

start:
	bl	main

blink_init:
	ldr	r8, =LT_TIMER
	ldr	r9, =LT_GPIOE_OUT
	ldr	r10, =LT_GPIO_OWNER

	ldr	r0, =0x00ffffff
	str	r0, [r10]		@ donate all GPIOs to Espresso

	ldr	r0, [r9]		@ load LT_GPIOE_OUT value once
	and	r0, #~0x100		@ reset sensor bar bit

blink:
	ldr	r1, [r8]		@ read timer
	lsr	r1, #13			@ move 0x100 into the one-second range
	and	r1, #0x100
	orr	r1, r0			@ combine with old LT_GPIOE_OUT state
	str	r1, [r9]
	b	blink			@ repeat!
