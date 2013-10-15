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
#include "istr.h"

static struct istr_pool pool;

#define JUNK_N		1024

static istr_t		junk[JUNK_N];

static void test_equality(void)
{
	istr_t a = istr_pool_alloc(&pool, "hello", -1);
	istr_t b = istr_pool_alloc(&pool, "foo", -1);
	istr_t c = istr_pool_alloc(&pool, "hello", -1);

	printf("a = \"%s\", b = \"%s\", c = \"%s\"\n",
	       istr_text(a), istr_text(b), istr_text(c));

	assert(!istr_equal(a, b));
	assert(!istr_equal(b, c));
	assert(istr_equal(a, c));

	assert(istr_compare(a, b) > 0);
	assert(istr_compare(b, c) < 0);
	assert(!istr_compare(a, c));

	istr_unref(a);
	istr_unref(b);
	istr_unref(c);
}

static void add_junk(void)
{
	int i;

	for (i = 0; i < JUNK_N; i++) {
		char buf[64];

		snprintf(buf, sizeof(buf), "%d", (i + 1) * 57);
		junk[i] = istr_pool_alloc(&pool, buf, -1);
	}
}

static void remove_junk(void)
{
	int i;

	for (i = 0; i < JUNK_N; i++)
		istr_unref(junk[i]);
}

static void test_gc(void)
{
	istr_t a;
	int old_offset;

	add_junk();
	a = istr_pool_alloc(&pool, "test", -1);
	old_offset = a->offset;
	remove_junk();
	istr_pool_gc(&pool);

	assert(!strcmp(istr_text(a), "test"));
	assert(a->offset < old_offset);

	istr_unref(a);
}

int main(void)
{
	istr_pool_init(&pool);

	test_equality();
	test_gc();

	add_junk();
	remove_junk();

	istr_pool_gc(&pool);
	assert(!pool.desc_count);
	assert(!pool.all);
	assert(!pool.text.length);
	assert(list_is_empty(&pool.descs.full));
	assert(list_is_empty(&pool.descs.partial));

	istr_pool_destroy(&pool);
	return 0;
}
