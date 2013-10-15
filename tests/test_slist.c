/* libdlb - data structures and utilities library
 * Copyright (C) 2013 Daniel Beer <dlbeer@gmail.com>
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
#include "slist.h"

#define N		1024

static struct slist_node recs[N];
static struct slist lst;

/* Add the nodes to an empty list in forward order using
 * slist_append().
 */
static void test_append(void)
{
	int i;

	for (i = 0; i < N; i++)
		slist_append(&lst, &recs[i]);
}

/* Add the nodes to an empty list in reverse order using
 * slist_push().
 */
static void test_push(void)
{
	int i;

	for (i = N - 1; i >= 0; i--)
		slist_push(&lst, &recs[i]);
}

/* Verify that the list contains all nodes in forward order. */
static void test_verify(void)
{
	struct slist_node *n = lst.start;
	int i;

	assert(lst.start == &recs[0]);
	assert(lst.end == &recs[N - 1]);

	for (i = 0; i < N; i++) {
		assert(n == &recs[i]);
		n = n->next;
	}

	assert(!n);
}

/* Remove the nodes from a full list in forward order using slist_pop().
 */
static void test_pop(void)
{
	struct slist_node *n;
	int i;

	for (i = 0; i < N; i++) {
		n = slist_pop(&lst);
		assert(n == &recs[i]);
	}

	assert(!lst.start);
	assert(!lst.end);

	n = slist_pop(&lst);
	assert(!n);
}

int main(void)
{
	slist_init(&lst);
	assert(slist_is_empty(&lst));

	test_append();
	test_verify();
	test_pop();
	assert(slist_is_empty(&lst));

	test_push();
	test_verify();

	return 0;
}
