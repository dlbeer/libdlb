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
#include "list.h"

#define N		1024

struct record {
	struct list_node	node;
	int			v;
};

static struct record recs[N];
static struct list_node lst;

static void test_init(void)
{
	int i;

	for (i = 0; i < N; i++)
		recs[i].v = i;

	list_init(&lst);
}

static void test_start_odd(void)
{
	int i;

	for (i = 1; i < N; i += 2)
		list_insert(&recs[i].node, &lst);
}

static void test_add_evens(void)
{
	int i;

	for (i = 0; i < N; i += 2)
		list_insert(&recs[i].node, &recs[i + 1].node);
}

static void test_remove(int mask)
{
	int i;

	for (i = 0; i < N; i++)
		if (((i & 1) && (mask & 1)) ||
		    (!(i & 1) && (mask & 2)))
			list_remove(&recs[i].node);
}

static void test_verify(int mask)
{
	int even_count = (mask & 2) ? (N / 2) : 0;
	int odd_count = (mask & 1) ? (N / 2) : 0;
	struct list_node *n;

	for (n = lst.next; n != &lst; n = n->next) {
		struct record *r = (struct record *)n;

		if (r->v & 1)
			odd_count--;
		else
			even_count--;
	}

	assert(!even_count);
	assert(!odd_count);
}

int main(void)
{
	test_init();

	test_start_odd();
	test_verify(1);
	test_add_evens();
	test_verify(3);
	test_remove(1);
	test_verify(2);
	test_remove(2);
	test_verify(0);

	return 0;
}
