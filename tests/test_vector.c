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

#include <assert.h>
#include "vector.h"

#define N		131072
#define ALLOC_EXPECT	17

static int last_capacity;
static int realloc_count;

static struct vector vec;

static void realloc_check(void)
{
	assert(vec.capacity >= vec.size);

	if (vec.capacity != last_capacity) {
		last_capacity = vec.capacity;
		realloc_count++;
	}
}

static void test_push(void)
{
	int i;

	realloc_count = 0;

	for (i = 0; i < N; i++) {
		vector_push(&vec, &i, 1);
		assert(vec.size == i + 1);
		realloc_check();
	}

	assert(realloc_count <= ALLOC_EXPECT);
}

static void test_check(int max)
{
	int i;

	assert(vec.size >= max);

	for (i = 0; i < max; i++)
		assert(VECTOR_AT(vec, i, int) == i);
}

static void test_pop(void)
{
	int i;

	realloc_count = 0;

	for (i = 0; i < N; i++) {
		vector_pop(&vec, 1);
		assert(vec.size == N - i - 1);
		realloc_check();
	}

	assert(realloc_count <= ALLOC_EXPECT);
	assert(vec.capacity < 1024);
}

static void test_wiggle(void)
{
	int i;

	realloc_count = 0;

	for (i = 0; i < N; i++) {
		vector_push(&vec, &i, 1);
		vector_push(&vec, &i, 1);
		vector_push(&vec, &i, 1);
		vector_pop(&vec, 1);
		vector_pop(&vec, 1);
		vector_push(&vec, &i, 1);
		vector_pop(&vec, 1);
		assert(vec.size == i + 1);
		realloc_check();
	}

	assert(realloc_count <= ALLOC_EXPECT);
}

static void test_bulk(void)
{
	int bulk[1024];
	int i;

	for (i = 0; i < 1024; i++)
		bulk[i] = i;

	realloc_count = 0;

	vector_push(&vec, bulk, 1024);
	realloc_check();

	assert(vec.size == 1024);
	assert(realloc_count <= 1);
	test_check(1024);

	vector_resize(&vec, 2048);
	realloc_check();

	assert(vec.size == 2048);
	assert(realloc_count <= 2);
	test_check(1024);

	vector_clear(&vec);
}

int main(void)
{
	vector_init(&vec, sizeof(int));
	test_bulk();
	test_push();
	test_check(N);
	test_pop();
	test_wiggle();
	test_check(N);
	vector_destroy(&vec);

	return 0;
}
