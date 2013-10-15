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

struct record {
	struct rbt_node		node;
	int			key;

	struct rbt_node		*sum_left;
	struct rbt_node		*sum_right;
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
static prng_t prng;

static struct rbt tree;

static void check_summary(struct rbt_node *n)
{
	struct record *r = (struct record *)n;

	if (!n)
		return;

	if (n->flags & RBT_FLAG_MODIFIED) {
		assert(!n->parent || (n->parent->flags & RBT_FLAG_MODIFIED));

		check_summary(n->left);
		check_summary(n->right);

		r->sum_left = n->left;
		r->sum_right = n->right;
		n->flags &= ~RBT_FLAG_MODIFIED;
	} else {
		assert(r->sum_left == n->left);
		assert(r->sum_right == n->right);
	}
}

static int check_recurse(struct rbt_node *n, struct rbt_node *p)
{
	int lc, rc;

	if (!n)
		return 0;

	assert(n->parent == p);

	if (RBT_IS_RED(n)) {
		assert(RBT_IS_BLACK(n->left));
		assert(RBT_IS_BLACK(n->right));
	}

	lc = check_recurse(n->left, n);
	rc = check_recurse(n->right, n);
	assert(lc == rc);

	if (RBT_IS_RED(n))
		return lc;

	return lc + 1;
}

static void test_check(void)
{
	assert(RBT_IS_BLACK(tree.root));
	check_recurse(tree.root, NULL);
	check_summary(tree.root);
}

static void test_present(int f, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		struct record *r = ordering[f + i];
		struct rbt_node *n = rbt_find(&tree, &r->key);
		struct record *ret = (struct record *)n;

		assert(r == ret);
	}
}

static void test_not_present(int f, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		struct record *r = ordering[f + i];
		struct rbt_node *n = rbt_find(&tree, &r->key);

		assert(!n);
	}
}

static void test_init(void)
{
	int i;

	rbt_init(&tree, cmp_record);

	for (i = 0; i < N; i++) {
		recs[i].key = i;
		ordering[i] = &recs[i];
	}
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
		struct rbt_node *n;

		n = rbt_insert(&tree, &r->key, &r->node);
		test_check();
		test_present(0, i);
		test_not_present(i + 1, N - i - 1);
		assert(!n);
	}
}

static void dump_recurse(const struct rbt_node *n, int depth)
{
	const struct record *r = (const struct record *)n;
	int i;

	if (!n)
		return;

	dump_recurse(n->left, depth + 1);
	printf("    ");
	for (i = 0; i < depth; i++)
		printf("- ");
	printf("%d (%c)\n", r->key,
		RBT_IS_BLACK(n) ? 'b' : 'R');
	dump_recurse(n->right, depth + 1);
}

static void test_dump(void)
{
	printf("Tree contents:\n");
	dump_recurse(tree.root, 0);
	printf("\n");
}

static void test_delete(void)
{
	int i;

	for (i = 0; i < N; i++) {
		struct record *r = ordering[i];

		rbt_remove(&tree, &r->node);
		test_check();
		test_not_present(0, i + 1);
		test_present(i + 1, N - i - 1);
	}
}

int main(void)
{
	prng_init(&prng, time(NULL));

	test_init();
	test_shuffle();
	test_insert();
	test_dump();

	test_shuffle();
	test_delete();
	test_dump();

	return 0;
}
