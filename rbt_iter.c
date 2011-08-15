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

#include <stdlib.h>
#include "rbt_iter.h"

struct rbt_node *rbt_iter_next(struct rbt_node *n)
{
	if (n->right) {
		n = n->right;
		while (n->left)
			n = n->left;

		return n;
	}

	while (n->parent && n->parent->right == n)
		n = n->parent;

	return n->parent;
}

struct rbt_node *rbt_iter_prev(struct rbt_node *n)
{
	if (n->left) {
		n = n->left;
		while (n->right)
			n = n->right;

		return n;
	}

	while (n->parent && n->parent->left == n)
		n = n->parent;

	return n->parent;
}

struct rbt_node *rbt_iter_first(struct rbt *t)
{
	struct rbt_node *n = t->root;

	while (n && n->left)
		n = n->left;

	return n;
}

struct rbt_node *rbt_iter_last(struct rbt *t)
{
	struct rbt_node *n = t->root;

	while (n && n->right)
		n = n->right;

	return n;
}

struct rbt_node *rbt_iter_le(struct rbt *t, const void *key)
{
	struct rbt_node *n = t->root;
	struct rbt_node *m = NULL;

	while (n) {
		int r = t->compare(key, n);

		if (!r)
			return n;

		if (r < 0) {
			n = n->left;
		} else {
			m = n;
			n = n->right;
		}
	}

	return m;
}

struct rbt_node *rbt_iter_ge(struct rbt *t, const void *key)
{
	struct rbt_node *n = t->root;
	struct rbt_node *m = NULL;

	while (n) {
		int r = t->compare(key, n);

		if (!r)
			return n;

		if (r < 0) {
			m = n;
			n = n->left;
		} else {
			n = n->right;
		}
	}

	return m;
}

struct rbt_node *rbt_iter_lt(struct rbt *t, const void *key)
{
	struct rbt_node *n = t->root;
	struct rbt_node *m = NULL;

	while (n) {
		int r = t->compare(key, n);

		if (r <= 0) {
			n = n->left;
		} else {
			m = n;
			n = n->right;
		}
	}

	return m;
}

struct rbt_node *rbt_iter_gt(struct rbt *t, const void *key)
{
	struct rbt_node *n = t->root;
	struct rbt_node *m = NULL;

	while (n) {
		int r = t->compare(key, n);

		if (r < 0) {
			m = n;
			n = n->left;
		} else {
			n = n->right;
		}
	}

	return m;
}
