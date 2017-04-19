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

#include "keyboard.h"

void keyboard_init(struct keyboard *keyb, int x, int y)
{
	keyb->x = x;
	keyb->y = y;
	keyb->flags = KEYB_SCREEN_DRC;
	keyb->keymap = &keyboard_map_us;
}

static void keyboard_putstr(struct keyboard *keyb,
		uint32_t xoff, uint32_t yoff, const char *str)
{
	int x = keyb->x + xoff;
	int y = keyb->y + yoff;

	if (keyb->flags & KEYB_SCREEN_TV)
		OSScreenPutFontEx(0, x, y, str);
	if (keyb->flags & KEYB_SCREEN_DRC)
		OSScreenPutFontEx(1, x, y, str);
}

static const char chars_us[] = {
	'`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	0,   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
	0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','\\',
	0,   'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   0
};

static const char chars_us_shifted[] = {
	'~', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
	0,   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
	0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '|',
	0,   'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   0
};

const struct keyboard_map keyboard_map_us = {
	.width = 13,
	.height = 4,
	.chars = chars_us,
	.chars_shifted = chars_us_shifted
};

int touched_x, touched_y;
void keyboard_draw(struct keyboard *keyb)
{
	int x, y;
	const char *chars;

	if (keyb->flags & KEYB_SHIFT)
		chars = keyb->keymap->chars_shifted;
	else
		chars = keyb->keymap->chars;

	for (x = 0; x < keyb->keymap->width; x++) {
		for (y = 0; y < keyb->keymap->height; y++) {
			char str[2] = {
				chars[x + keyb->keymap->width*y],
				'\0'
			};
			keyboard_putstr(keyb, 5*x + y, 2*y, str);
		}
	}

	//keyboard_putstr(keyb, 0, 2, "-->"); (tab)
	keyboard_putstr(keyb, 0, 4, (keyb->flags & KEYB_SHIFT)? "SHIFT" : "shift");
	keyboard_putstr(keyb, 0, 6, "ctrl");

	//OSScreenPutFontEx(1, touched_x, touched_y, "X");
}

static void translate_touch_coords(int touchx, int touchy, int *charx, int *chary)
{
	/* char( 0,  0) = touch( 330, 3500)
	 * char(49, 17) = touch(3000,  380) */
	*charx = (touchx - 330) * 0.018148;
	*chary = (touchy - 3500) * -0.005448717;
}

static char keymap_lookup(const struct keyboard_map *keymap, int shifted, int x, int y)
{
	if (x < 0 || y < 0 || x >= keymap->width || y >= keymap->height)
		return 0;

	const char *chars = shifted? keymap->chars_shifted : keymap->chars;
	return chars[x + y*keymap->width];
}

void keyboard_handle_vpad(struct keyboard *keyb,
		const VPADData *vpad, keyboard_callback_t cb)
{
	/* L/R: shift */
	if (vpad->btns_h & VPAD_BUTTON_L || vpad->btns_h & VPAD_BUTTON_R)
		keyb->flags |= KEYB_SHIFT;
	else
		keyb->flags &= ~KEYB_SHIFT;

	/* A: enter */
	if (vpad->btns_d & VPAD_BUTTON_A)
		cb(keyb, '\n');

	/* B: backspace */
	if (vpad->btns_d & VPAD_BUTTON_B)
		cb(keyb, KEYB_BACKSPACE);

	/* ZL: space */
	if (vpad->btns_d & VPAD_BUTTON_ZL)
		cb(keyb, ' ');

	/* handle touch input */
	if (vpad->tpdata.touched && vpad->tpdata.invalid == 0) {
		if (!(keyb->flags & KEYB_TOUCHED)) {
			int x, y, ch;
			translate_touch_coords(vpad->tpdata.x, vpad->tpdata.y,
					&x, &y);

			touched_x = x;
			touched_y = y;

			x -= keyb->x;
			y -= keyb->y;

			y = (y + 1) / 2;
			x = (x - y + 2) / 5;

			ch = keymap_lookup(keyb->keymap,
					keyb->flags & KEYB_SHIFT, x, y);
			if (ch)
				cb(keyb, ch);

			keyb->flags |= KEYB_TOUCHED;
		}
	} else {
		keyb->flags &= ~KEYB_TOUCHED;
	}
}
