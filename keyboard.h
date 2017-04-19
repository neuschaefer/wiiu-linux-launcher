/*
 * Another Wii U Soft-Keyboard
 *
 * Copyright (C) 2017  Jonathan Neuschäfer <j.neuschaefer@gmx.net>
 *
 * [3-clause BSD license:]
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *     (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *
 *     (3)The name of the author may not be used to
 *     endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _JN_KEYBOARD_H_
#define _JN_KEYBOARD_H_

#include <os_functions.h>
#include <vpad_functions.h>

struct keyboard_map {
	int width, height;
	const char *chars, *chars_shifted;
};
extern const struct keyboard_map keyboard_map_us;

struct keyboard {
	/* The character position of the upper-left corner of the keyboard */
	int x, y;

	int flags;
#define KEYB_SHIFT	(1 << 0)	/* The shift key has been pressed */
#define KEYB_SCREEN_TV	(1 << 1)	/* Draw on the TV */
#define KEYB_SCREEN_DRC	(1 << 2)	/* Draw on the gamepad */
#define KEYB_TOUCHED	(1 << 3)	/* The current touch event has been processed */

	const struct keyboard_map *keymap;
};

/* Initialize a struct keyboard */
extern void keyboard_init(struct keyboard *keyb, int x, int y);

/* Draw the keyboard */
extern void keyboard_draw(struct keyboard *keyb);

/*
 * A keyboard event callback. A function of this type is called whenever a new
 * character has been typed in.
 *
 * ch: An ascii character or one of the special characters defined below.
 */
typedef void (* keyboard_callback_t)(struct keyboard *keyb, int ch);
#define KEYB_BACKSPACE -1

/*
 * Give the keyboard at character position (x,y) a new event to digest. This
 * may result in cb being called.
 */
extern void
keyboard_handle_vpad(struct keyboard *keyb, const VPADData *vpad, keyboard_callback_t cb);

#endif
