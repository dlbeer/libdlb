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

#ifndef RBT_H_
#define RBT_H_

#include <stddef.h>

/* This is the base of a tree node. It should be embedded within a larger
 * data structure containing a key and optionally some data.
 *
 * The RBT_FLAG_MODIFIED field is set whenever the set comprising the
 * node's descendants has changed.
 */
#define RBT_FLAG_RED		0x01
#define RBT_FLAG_MODIFIED	0x02

#define RBT_IS_RED(n)		((n) && (n)->flags & RBT_FLAG_RED)
#define RBT_IS_BLACK(n)		(!(n) || !((n)->flags & RBT_FLAG_RED))

struct rbt_node {
	int			flags;
	struct rbt_node		*left;
	struct rbt_node		*right;
	struct rbt_node		*parent;
};

/* This function should compare the key contained within a tree node to
 * the new key given by a.
 */
typedef int (*rbt_compare_t)(const void *a, const struct rbt_node *b);

/* A tree should be initialized by filling in the compare member and
 * setting root to NULL.
 */
struct rbt {
	rbt_compare_t		compare;
	struct rbt_node		*root;
};

static inline void rbt_init(struct rbt *t, rbt_compare_t cmp)
{
	t->root = NULL;
	t->compare = cmp;
}

struct rbt_node *rbt_find(const struct rbt *t, const void *key);

/* Insert a node into a tree. If there was already a node with the
 * given key, the old node is returned.
 */
struct rbt_node *rbt_insert(struct rbt *t, const void *key,
			    struct rbt_node *n);

/* Remove a node from a tree. If a matching node was found, it's
 * returned after being removed.
 */
void rbt_remove(struct rbt *t, struct rbt_node *n);

/* Mark a node and its ancestors as modified. */
void rbt_mark_modified(struct rbt_node *n);

#endif
