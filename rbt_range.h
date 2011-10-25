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

#ifndef RBT_RANGE_H_
#define RBT_RANGE_H_

/* These functions can be used to implement efficient range queries on
 * augmented RBTs.
 *
 * When a range query is performed, a callback is invoked for a O(log n)
 * nodes and subtrees which are contained in the range. The callback is
 * responsible for collecting the necessary data and compiling the result.
 */

#include "rbt.h"

/* This is the type of range selection functions. The function, given a
 * node, should test the key to see whether its inside the required range.
 *
 * It should return 0 if the key is inside, -1 if it's before the range
 * and 1 if it's after. The range must be contiguous (but it may be
 * open-ended).
 */
typedef int (*rbt_range_cmp_t)(void *query_data, const struct rbt_node *n);

/* This callback is invoked for each node or subtree which is inside the
 * range. The rbt_range_addtype_t parameter specifies whether it's the
 * node or the entire subtree which lies inside the range.
 *
 * To implement an efficient range query, the callback should do the
 * following:
 *
 *     if type == RBT_RANGE_ADD_NODE, add data from only the node itself
 *        to the query result.
 *     if type == RBT_RANGE_ADD_TREE, call update_subtree() to compile
 *        summary data for the subtree, and then add the summary data
 *        to the query result.
 *
 * update_subtree() should be a recursive function which does the following:
 *
 *   - if the node doesn't have RBT_FLAG_MODIFIED set, then return
 *   - if the node is a non-leaf, call update_subtree() on the left and
 *     right subtrees.
 *   - add the summary from the left subtree to the summary for this node
 *   - add the node's own data to the summary
 *   - add the summary from the right subtree to the summary for this node
 *   - clear RBT_FLAG_MODIFIED
 *
 * The trees and nodes presented to the callback function will be given
 * in-order.
 */
typedef enum {
	RBT_RANGE_ADD_NODE,
	RBT_RANGE_ADD_TREE
} rbt_range_addtype_t;

typedef void (*rbt_range_add_t)(void *query_data, struct rbt_node *n,
				rbt_range_addtype_t type);

/* This function performs the actual range query. The given query_data
 * argument will be passed to both the comparison function and the result
 * compilation callback.
 */
void rbt_range_query(struct rbt *tree, void *query_data,
		     rbt_range_cmp_t cmp, rbt_range_add_t cb);

#endif
