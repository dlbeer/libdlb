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

#ifndef RBT_ITER_H_
#define RBT_ITER_H_

#include "rbt.h"

/* Given a node within a red-black tree, find the in-order successor and
 * predecessor.
 */
struct rbt_node *rbt_iter_next(struct rbt_node *n);
struct rbt_node *rbt_iter_prev(struct rbt_node *n);

/* Retrieve the lexicographically smallest and largest nodes in a tree. */
struct rbt_node *rbt_iter_first(struct rbt *t);
struct rbt_node *rbt_iter_last(struct rbt *t);

/* Find nodes which are:
 *   - le: the largest node not greater than the given key
 *   - ge: the smallest node not less than the given key
 *   - lt: the largest node less than the given key
 *   - gt: the smallest node greater than the given key
 */
struct rbt_node *rbt_iter_le(struct rbt *t, const void *key);
struct rbt_node *rbt_iter_ge(struct rbt *t, const void *key);
struct rbt_node *rbt_iter_lt(struct rbt *t, const void *key);
struct rbt_node *rbt_iter_gt(struct rbt *t, const void *key);

#endif
