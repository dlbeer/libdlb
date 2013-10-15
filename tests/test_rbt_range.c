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
#include <time.h>
#include <assert.h>
#include "prng.h"
#include "rbt.h"
#include "rbt_range.h"

struct record {
	struct rbt_node		node;
	int			key;
	int			sum;
};

#define N		1024

static struct record primes[N];
static struct rbt tree;
static prng_t prng;

static int cmp_record(const void *kd, const struct rbt_node *n)
{
	int key = *(int *)kd;
	struct record *r = (struct record *)n;

	if (key < r->key)
		return -1;
	if (key > r->key)
		return 1;

	return 0;
}

static void update_subtree(struct rbt_node *n)
{
	struct record *r = (struct record *)n;

	if (!(n->flags & RBT_FLAG_MODIFIED))
		return;

	r->sum = r->key;

	if (n->left) {
		update_subtree(n->left);
		r->sum += ((struct record *)n->left)->sum;
	}

	if (n->right) {
		update_subtree(n->right);
		r->sum += ((struct record *)n->right)->sum;
	}

	n->flags &= ~RBT_FLAG_MODIFIED;
}

static void init_primes(void)
{
	int dst = 0;
	int n = 2;

	while (dst < N) {
		int d;

		for (d = 2; d * d <= n; d++)
			if (!(n % d))
				goto not_prime;

		primes[dst++].key = n;
not_prime:
		n++;
	}
}

static void add_evens(void)
{
	int i;

	for (i = 0; i < N; i += 2)
		rbt_insert(&tree, &primes[i].key, &primes[i].node);
}

static void add_odds(void)
{
	int i;

	for (i = 1; i < N; i += 2)
		rbt_insert(&tree, &primes[i].key, &primes[i].node);
}

static void remove_evens(void)
{
	int i;

	for (i = 0; i < N; i += 2)
		rbt_remove(&tree, &primes[i].node);
}

static void remove_odds(void)
{
	int i;

	for (i = 1; i < N; i += 2)
		rbt_remove(&tree, &primes[i].node);
}

static int recursive_sum(const struct rbt_node *n, int low, int high)
{
	const struct record *r = (const struct record *)n;
	int sum = 0;

	if (!n)
		return 0;

	sum += recursive_sum(n->left, low, high);
	sum += recursive_sum(n->right, low, high);

	if (r->key >= low && r->key <= high)
		sum += r->key;

	return sum;
}

struct query_info {
	int		low;
	int		high;
	int		result;
};

static int query_cmp(void *query_data, const struct rbt_node *n)
{
	const struct record *r = (const struct record *)n;
	const struct query_info *qi = (const struct query_info *)query_data;

	if (r->key < qi->low)
		return -1;
	if (r->key > qi->high)
		return 1;

	return 0;
}

static void query_add(void *query_data, struct rbt_node *n,
		      rbt_range_addtype_t type)
{
	struct record *r = (struct record *)n;
	struct query_info *qi = (struct query_info *)query_data;

	if (type == RBT_RANGE_ADD_NODE) {
		qi->result += r->key;
	} else {
		update_subtree(n);
		qi->result += r->sum;
	}
}

static int range_query(int low, int high)
{
	struct query_info qi;

	qi.low = low;
	qi.high = high;
	qi.result = 0;

	rbt_range_query(&tree, &qi, query_cmp, query_add);
	return qi.result;
}

static void query_test(int count)
{
	while (count--) {
		int low = prng_next(&prng) % (primes[N - 1].key + 10);
		int high = prng_next(&prng) % (primes[N - 1].key + 10);
		volatile int result_iter;
		volatile int result_range;

		if (low > high) {
			int tmp = low;

			low = high;
			high = tmp;
		}

		result_iter = recursive_sum(tree.root, low, high);
		result_range = range_query(low, high);

		assert(result_iter == result_range);
	}
}

int main(void)
{
	rbt_init(&tree, cmp_record);
	prng_init(&prng, time(NULL));

	init_primes();

	add_evens();
	query_test(1000);
	add_odds();
	query_test(1000);
	remove_evens();
	query_test(1000);
	add_evens();
	query_test(1000);
	remove_odds();
	query_test(1000);

	return 0;
}
