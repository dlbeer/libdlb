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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "strbuf.h"

static void check(const struct strbuf *buf)
{
	assert(!buf->length || buf->length + 1 <= buf->capacity);
	assert(buf->text);
	assert(!buf->text[buf->length]);
	assert(!buf->failed);
}

static void big_test(struct strbuf *buf)
{
	int i;

	strbuf_clear(buf);
	check(buf);

	for (i = 0; i < 100; i++) {
		strbuf_add_string(buf, "Hello", -1);
		check(buf);
	}

	assert(buf->length == 500);

	for (i = 0; i < 100; i++) {
		int r = memcmp(buf->text + i * 5, "Hello", 5);

		assert(!r);
	}

	strbuf_resize(buf, 250);
	check(buf);

	for (i = 0; i < 50; i++) {
		int r = memcmp(buf->text + i * 5, "Hello", 5);

		assert(!r);
	}
}

static void compare(const struct strbuf *buf, const char *text)
{
	int r = strcmp(buf->text, text);

	assert(!r);
}

int main(void)
{
	struct strbuf buf;

	strbuf_init(&buf);
	check(&buf);

	strbuf_add_char(&buf, 'x');
	check(&buf);

	strbuf_add_string(&buf, "Hello World", -1);
	check(&buf);

	strbuf_add_char(&buf, 'y');
	check(&buf);

	compare(&buf, "xHello Worldy");
	strbuf_clear(&buf);
	check(&buf);

	strbuf_printf(&buf, "%d %d %d %s",
			1, 2, 3, "foo");
	check(&buf);
	strbuf_printf(&buf, "%x", 0xdeadbeef);
	check(&buf);
	compare(&buf, "1 2 3 foodeadbeef");

	strbuf_resize(&buf, 3);
	check(&buf);
	compare(&buf, "1 2");

	big_test(&buf);

	strbuf_destroy(&buf);
	return 0;
}
