/*
 * Basic string functions
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

#include <string.h>

void *memset(void *s, int c, size_t n)
{
	char *p = s;
	size_t i;

	for (i = 0; i < n; i++) {
		p[i] = c;
	}

	return s;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	const char *s = src;
	char *d = dest;
	size_t i;

	for (i = 0; i < n; i++) {
		d[i] = s[i];
	}

	return dest;
}

size_t strlen(const char *s)
{
	size_t res = 0;

	while (*s++)
		res++;

	return res;
}

#undef strcmp
int strcmp(const char *a, const char *b)
{
	size_t i;
	const unsigned char *au = (const unsigned char *)a;
	const unsigned char *bu = (const unsigned char *)b;

	for (i = 0; au[i] && bu[i]; i++) {
		if (au[i] < bu[i])
			return -1;
		if (au[i] > bu[i])
			return 1;
	}

	return 0;
}
