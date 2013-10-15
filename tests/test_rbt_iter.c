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
#include <time.h>
#include <assert.h>
#include "prng.h"
#include "rbt.h"
#include "rbt_iter.h"

struct record {
	struct rbt_node		node;
	int			key;
};

#define N		1024

static int cmp_record(const void *k, const struct rbt_node *n)
{
	const int *key = (const int *)k;
	const struct record *r = (const struct record *)n;

	if (*key < r->key)
		return -1;
	if (*key > r->key)
		return 1;

	return 0;
}

static struct record recs[N];
static struct record *ordering[N];
static struct rbt tree;
static prng_t prng;

static void test_init(void)
{
	int i;

	for (i = 0; i < N; i++) {
		recs[i].key = i * 2;
		ordering[i] = &recs[i];
	}

	rbt_init(&tree, cmp_record);
}

static void test_shuffle(void)
{
	int i;

	for (i = N - 1; i > 0; i--) {
		int j = prng_next(&prng) % i;

		if (i != j) {
			struct record *t = ordering[i];

			ordering[i] = ordering[j];
			ordering[j] = t;
		}
	}
}

static void test_insert(void)
{
	int i;

	for (i = 0; i < N; i++) {
		struct record *r = ordering[i];

		rbt_insert(&tree, &r->key, &r->node);
	}
}

static void test_edges(void)
{
	struct rbt_node *n;

	n = rbt_iter_next(&recs[N - 1].node);
	assert(!n);

	n = rbt_iter_prev(&recs[0].node);
	assert(!n);

	n = rbt_iter_lt(&tree, &recs[0].key);
	assert(!n);

	n = rbt_iter_gt(&tree, &recs[N - 1].key);
	assert(!n);

	n = rbt_iter_first(&tree);
	assert(n == &recs[0].node);

	n = rbt_iter_last(&tree);
	assert(n == &recs[N - 1].node);
}

static void test_middle(void)
{
	int i;

	for (i = 0; i < N; i++) {
		struct record *r = &recs[i];
		struct rbt_node *n;
		int key;

		if (i > 0) {
			n = rbt_iter_next(&recs[i - 1].node);
			assert(n == &r->node);

			n = rbt_iter_lt(&tree, &recs[i].key);
			assert(n == &recs[i - 1].node);
		}

		if (i + 1 < N) {
			n = rbt_iter_prev(&recs[i + 1].node);
			assert(n == &r->node);

			n = rbt_iter_gt(&tree, &recs[i].key);
			assert(n == &recs[i + 1].node);
		}

		key = recs[i].key;
		n = rbt_iter_le(&tree, &key);
		assert (n == &r->node);
		n = rbt_iter_ge(&tree, &key);
		assert (n == &r->node);

		key = recs[i].key + 1;
		n = rbt_iter_le(&tree, &key);
		assert (n == &r->node);
		n = rbt_iter_lt(&tree, &key);
		assert (n == &r->node);

		key = recs[i].key - 1;
		n = rbt_iter_ge(&tree, &key);
		assert (n == &r->node);
		n = rbt_iter_gt(&tree, &key);
		assert (n == &r->node);
	}
}

int main(void)
{
	int i;

	prng_init(&prng, time(NULL));

	for (i = 0; i < 100; i++) {
		test_init();
		test_shuffle();
		test_insert();

		test_edges();
		test_middle();
	}

	return 0;
}
