/* libdlb - data structures and utilities library
 * Copyright (C) 2011 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "strbuf.h"

static const char strbuf_blank[1] = {0};

void strbuf_init(struct strbuf *buf)
{
	memset(buf, 0, sizeof(*buf));
	buf->text = (char *)strbuf_blank;
}

void strbuf_destroy(struct strbuf *buf)
{
	if (buf->text != strbuf_blank)
		free(buf->text);

	memset(buf, 0, sizeof(*buf));
}

void strbuf_clear(struct strbuf *buf)
{
	if (buf->text != strbuf_blank)
		free(buf->text);

	memset(buf, 0, sizeof(*buf));
	buf->text = (char *)strbuf_blank;
}

int strbuf_resize(struct strbuf *buf, size_t new_length)
{
	if (new_length + 1 > buf->capacity &&
	    strbuf_capacity_hint(buf, new_length) < 0) {
		buf->failed = 1;
		return -1;
	}

	buf->length = new_length;
	buf->text[buf->length] = 0;

	if ((new_length + 1) * 4 < buf->capacity)
		strbuf_capacity_hint(buf, new_length + 1);

	return 0;
}

int strbuf_capacity_hint(struct strbuf *buf, size_t length)
{
	size_t new_cap = 32;
	char *new_text;

	if (length < buf->length)
		return 0;

	while (new_cap < length + 1)
		new_cap <<= 1;

	if (new_cap == buf->capacity)
		return 0;

	if (buf->text == strbuf_blank)
		new_text = malloc(new_cap);
	else
		new_text = realloc(buf->text, new_cap);

	if (!new_text)
		return -1;

	buf->text = new_text;
	buf->capacity = new_cap;

	return 0;
}

int strbuf_add_char(struct strbuf *buf, int c)
{
	char ch = c;

	return strbuf_add_string(buf, &ch, 1);
}

int strbuf_add_string(struct strbuf *buf, const char *text, int length)
{
	if (length < 0)
		length = strlen(text);

	if (strbuf_capacity_hint(buf, buf->length + length) < 0)
		return -1;

	memcpy(buf->text + buf->length, text, length);
	buf->length += length;
	buf->text[buf->length] = 0;

	return length;
}

int strbuf_vprintf(struct strbuf *buf, const char *fmt, va_list ap)
{
	if (strbuf_capacity_hint(buf, 1) < 0)
		return -1;

	for (;;) {
		va_list map;
		int room = buf->capacity - buf->length;
		int r;

		va_copy(map, ap);
		r = vsnprintf(buf->text + buf->length, room, fmt, map);
		va_end(map);

		if (r + 1 < room) {
			buf->length += r;
			return r;
		}

		if (strbuf_capacity_hint(buf, buf->capacity + 1) < 0) {
			buf->failed = 1;
			return -1;
		}
	}

	return 0;
}

int strbuf_printf(struct strbuf *buf, const char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = strbuf_vprintf(buf, fmt, ap);
	va_end(ap);

	return r;
}
