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
#include <assert.h>
#include "slab.h"

#define N		131072

struct item {
	int		x;
	int		y;
};

static struct item *objects[N];
static struct slab slb;

static void test_alloc_all(void)
{
	int i;

	for (i = 0; i < N; i++) {
		objects[i] = slab_alloc(&slb);
		objects[i]->x = i;
		objects[i]->y = i * 2;
	}
}

static void test_check(void)
{
	int i;

	for (i = 0; i < N; i++) {
		assert(objects[i]->x == i);
		assert(objects[i]->y == i * 2);
	}
}

static void test_free_all(void)
{
	int i;

	for (i = 0; i < N; i++)
		slab_free(&slb, objects[i]);
}

int main(void)
{
	slab_init(&slb, sizeof(struct item));

	printf("objsize     = %d (original %d)\n",
		slb.objsize, (int)sizeof(struct item));
	printf("count       = %d\n", slb.count);
	printf("info_offset = %d\n", slb.info_offset);
	printf("slabsize    = %d\n", slb.slabsize);

	assert(slb.objsize >= sizeof(struct item));
	assert(slb.count * slb.objsize <= slb.info_offset);
	assert(slb.info_offset < slb.slabsize);

	test_alloc_all();
	test_check();
	test_free_all();

	return 0;
}
