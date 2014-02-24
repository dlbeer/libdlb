/* libdlb - data structures and utilities library
 * Copyright (C) 2014 Daniel Beer <dlbeer@gmail.com>
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
#include <stdio.h>
#include <assert.h>
#include "arena.h"

#define TEST_SIZE		(100 * 1048576)
#define TEST_COUNT		128

struct test {
	char		*ptr;
	size_t		size;
};

static char *base;
static struct test tests[TEST_COUNT];
static struct arena arena;
static size_t total_available;

static int cmp_by_ptr(const void *a, const void *b)
{
	struct test *ta = (struct test *)a;
	struct test *tb = (struct test *)b;

	if (ta->ptr < tb->ptr)
		return -1;
	if (ta->ptr > tb->ptr)
		return 1;

	return 0;
}

static void shuffle(void)
{
	int i;

	for (i = TEST_COUNT - 1; i >= 0; i--) {
		const int j = random() % (i + 1);
		struct test tmp;

		memcpy(&tmp, &tests[i], sizeof(tmp));
		memcpy(&tests[i], &tests[j], sizeof(tests[i]));
		memcpy(&tests[j], &tmp, sizeof(tests[j]));
	}
}

static void test_alloc(void)
{
	int i;

	for (i = 0; i < TEST_COUNT; i++) {
		struct test *t = &tests[i];

		t->size = (random() % 256) << (random() % 10);
		t->ptr = arena_alloc(&arena, t->size);
		assert(t->ptr);

		printf("  %3d. alloc %d -> %p\n", i, (int)t->size, t->ptr);
	}
}

static void test_realloc(void)
{
	int i;

	for (i = 0; i < TEST_COUNT; i++) {
		struct test *t = &tests[i];

		printf("  %3d. realloc %p/%d", i, t->ptr, (int)t->size);

		t->size = (random() % 256) << (random() % 15);
		t->ptr = arena_realloc(&arena, t->ptr, t->size);
		assert(t->ptr);

		printf(" %d -> %p\n", (int)t->size, t->ptr);
	}
}

static void test_free(void)
{
	int i;

	for (i = 0; i < TEST_COUNT; i++)
		arena_free(&arena, tests[i].ptr);
}

static void check(void)
{
	size_t used = 0;
	size_t free = arena_count_free(&arena);
	size_t wasted;
	int i;

	for (i = 0; i < TEST_COUNT; i++)
		used += tests[i].size;

	wasted = total_available - used - free;

	printf("Used:     %d\n", (int)used);
	printf("Free:     %d\n", (int)free);
	printf("Total:    %d\n", (int)total_available);
	printf("Wasted:   %d (%.02f%%)\n", (int)wasted,
		(double)wasted * 100.0 / (double)total_available);
	printf("\n");

	assert(used + free <= total_available);

	qsort(tests, TEST_COUNT, sizeof(tests[0]), cmp_by_ptr);

	for (i = 0; i + 1 < TEST_COUNT; i++) {
		struct test *t = &tests[i];
		struct test *n = &tests[i + 1];

		assert(t->ptr >= base);
		assert(t->ptr < base + TEST_SIZE);
		assert(t->ptr + t->size <= n->ptr);
	}
}

int main(void)
{
	int i;

	srandom(0);

	base = calloc(1, TEST_SIZE);
	assert(base);

	arena_init(&arena, base, TEST_SIZE);
	total_available = arena_count_free(&arena);

	printf("Alloc...\n");
	test_alloc();
	check();

	for (i = 0; i < 10; i++) {
		printf("Realloc...\n");
		shuffle();
		test_realloc();
		check();
	}

	printf("Free...\n");
	shuffle();
	test_free();
	assert(arena_count_free(&arena) == total_available);

	free(base);
	return 0;
}
