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

#include "rbt_range.h"

static void query_low(struct rbt_node *n, void *query_data,
		      rbt_range_cmp_t cmp, rbt_range_add_t cb)
{
	while (n) {
		int r = cmp(query_data, n);

		if (r < 0) {
			n = n->right;
		} else {
			query_low(n->left, query_data, cmp, cb);
			cb(query_data, n, RBT_RANGE_ADD_NODE);
			if (n->right)
				cb(query_data, n->right, RBT_RANGE_ADD_TREE);
			break;
		}
	}
}

static void query_high(struct rbt_node *n, void *query_data,
		       rbt_range_cmp_t cmp, rbt_range_add_t cb)
{
	while (n) {
		int r = cmp(query_data, n);

		if (r > 0) {
			n = n->left;
		} else {
			if (n->left)
				cb(query_data, n->left, RBT_RANGE_ADD_TREE);
			cb(query_data, n, RBT_RANGE_ADD_NODE);
			query_high(n->right, query_data, cmp, cb);
			break;
		}
	}
}

static void query_over(struct rbt_node *n, void *query_data,
		       rbt_range_cmp_t cmp, rbt_range_add_t cb)
{
	while (n) {
		int r = cmp(query_data, n);

		if (r < 0) {
			n = n->right;
		} else if (r > 0) {
			n = n->left;
		} else {
			query_low(n->left, query_data, cmp, cb);
			cb(query_data, n, RBT_RANGE_ADD_NODE);
			query_high(n->right, query_data, cmp, cb);
			break;
		}
	}
}

void rbt_range_query(struct rbt *tree, void *query_data,
		     rbt_range_cmp_t cmp, rbt_range_add_t cb)
{
	query_over(tree->root, query_data, cmp, cb);
}
