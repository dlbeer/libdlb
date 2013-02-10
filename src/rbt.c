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

#include <string.h>
#include "rbt.h"

/* This is assumed to be larger than the path from any tree's root to
 * any of its leaves. Assuming the properties of a RBT hold, this would
 * allow for trees of up to 2**64 nodes.
 */
#define MAX_H		128

#define MAKE_RED(n)	((n)->flags |= RBT_FLAG_RED)
#define MAKE_BLACK(n)	((n)->flags &= ~RBT_FLAG_RED)

#define MARK_MODIFIED(n) ((n)->flags |= RBT_FLAG_MODIFIED)

struct rbt_node *rbt_find(const struct rbt *t, const void *key)
{
	struct rbt_node *n = t->root;

	while (n) {
		int r = t->compare(key, n);

		if (!r)
			break;

		n = (r < 0) ? n->left : n->right;
	}

	return n;
}

/* Fix the downwards pointer from p to old by setting it to n. If p is
 * NULL, reset the tree root.
 */
static void fix_downptr(struct rbt *t, struct rbt_node *p,
			struct rbt_node *old, struct rbt_node *n)
{
	if (p) {
		if (p->left == old)
			p->left = n;
		else
			p->right = n;
	} else {
		t->root = n;
	}
}

/* Rotating a node (n) left invalidates the summary for (i.e. changes
 * the set of descendants of):
 *
 *    - n itself
 *    - n's originally right child
 *
 *    B                 D
 *   / \       ==>     / \
 *  A   D             B   E
 *     / \           / \
 *    C   E         A   C
 */

static void rotate_left(struct rbt *t, struct rbt_node *n)
{
	struct rbt_node *p = n->parent;
	struct rbt_node *r = n->right;

	n->right = r->left;
	if (n->right)
		n->right->parent = n;

	r->left = n;
	n->parent = r;

	r->parent = p;
	fix_downptr(t, r->parent, n, r);
}

/* Rotating a node (n) right invalidates the summary for (i.e. changes
 * the set of descendants of):
 *
 *    - n itself
 *    - n's originally left child
 *
 *      D                 B
 *     / \       ==>     / \
 *    B   E             A   D
 *   / \                   / \
 *  A   C                 C   E
 */

static void rotate_right(struct rbt *t, struct rbt_node *n)
{
	struct rbt_node *p = n->parent;
	struct rbt_node *l = n->left;

	n->left = l->right;
	if (n->left)
		n->left->parent = n;

	l->right = n;
	n->parent = l;

	l->parent = p;
	fix_downptr(t, l->parent, n, l);
}

static inline struct rbt_node *sibling(struct rbt_node *n)
{
	struct rbt_node *p = n->parent;

	return (n == p->left) ? p->right : p->left;
}

static inline struct rbt_node *grandparent(struct rbt_node *n)
{
	return n->parent->parent;
}

/* An invariant holds during repair_after_insert:
 *
 *   - at each step, n and its ancestors all have RBT_FLAG_MODIFIED set
 */
static void repair_after_insert(struct rbt *t, struct rbt_node *n)
{
	/* We may have violated the property that red nodes have only
	 * black children. Go up from the newly inserted node and
	 * attempt to fix the property by recolouring nodes.
	 *
	 * No structural changes are made here, and no summaries are
	 * invalidated in this portion.
	 */
	for (;;) {
		struct rbt_node *u;

		if (!n->parent) {
			MAKE_BLACK(n);
			return;
		}

		if (RBT_IS_BLACK(n->parent))
			return;

		/* If the both the parent and the uncle of the current
		 * node are red, we can recolour and propagate upwards.
		 * Otherwise, we have to stop and rotate.
		 */
		u = sibling(n->parent);
		if (RBT_IS_BLACK(u))
			break;

		MAKE_BLACK(u);
		MAKE_BLACK(n->parent);
		MAKE_RED(grandparent(n));
		n = grandparent(n);
	}

	/* If the node and its parent descend in different directions,
	 * we rotate the parent.
	 *
	 * The nodes affected by rotation, if any, were already invalidated
	 * during the insertion.
	 */
	if (n == n->parent->left &&
	    n->parent == grandparent(n)->right) {
		rotate_right(t, n->parent);
		n = n->right;
	} else if (n == n->parent->right &&
		   n->parent == grandparent(n)->left) {
		rotate_left(t, n->parent);
		n = n->left;
	}

	/* Now the node and its parent descend via the same path. We
	 * recolour and rotate the grandparent to rebalance the tree.
	 *
	 * The nodes affected by rotation are already invalidated.
	 */
	MAKE_BLACK(n->parent);
	MAKE_RED(grandparent(n));

	if (n == n->parent->left)
		rotate_right(t, grandparent(n));
	else
		rotate_left(t, grandparent(n));
}

struct rbt_node *rbt_insert(struct rbt *t, const void *key,
			    struct rbt_node *n)
{
	struct rbt_node *p = NULL;
	struct rbt_node **nptr = &t->root;

	/* Search for the node pointer which should point to n. */
	while (*nptr) {
		struct rbt_node *c = *nptr;
		int r = t->compare(key, *nptr);

		MARK_MODIFIED(c);

		if (!r) {
			memcpy(n, c, sizeof(*c));
			return c;
		}

		p = *nptr;
		nptr = (r < 0) ? &c->left : &c->right;
	}

	/* Insert n as a leaf. */
	*nptr = n;
	n->left = NULL;
	n->right = NULL;
	n->flags = 0;
	n->parent = p;

	MAKE_RED(n);
	MARK_MODIFIED(n);

	repair_after_insert(t, n);
	return NULL;
}

/* Swap the position of n with that of the lexically smallest node in
 * its right subtree.
 *
 * This operation invalidates n, and all nodes on the path from n to
 * the lexically smallest node. We don't handle that here, though.
 */
static void swap_with_successor(struct rbt *t, struct rbt_node *n)
{
	struct rbt_node *p = n->parent;
	struct rbt_node *s = n->right;

	if (s->left) {
		struct rbt_node tmp;

		while (s->left)
			s = s->left;

		memcpy(&tmp, s, sizeof(tmp));
		memcpy(s, n, sizeof(*s));
		memcpy(n, &tmp, sizeof(*n));

		n->parent->left = n;
	} else {
		struct rbt_node *left = n->left;
		struct rbt_node *right = s->right;
		int n_flags = n->flags;
		int s_flags = s->flags;

		/* If there is no intermediate between n and s,
		 * we can't just swap contents because we'll end
		 * up with a pointer loop.
		 */
		s->left = left;
		s->flags = n_flags;
		s->right = n;
		s->parent = p;

		n->right = right;
		n->left = NULL;
		n->parent = s;
		n->flags = s_flags;
	}

	fix_downptr(t, p, n, s);

	s->left->parent = s;
	s->right->parent = s;
	if (n->right)
		n->right->parent = n;
}

/* An invariant holds during repair_after_remove:
 *
 *   - at each step, n and its ancestors all have RBT_FLAG_MODIFIED set
 */
static void repair_after_remove(struct rbt *t, struct rbt_node *n)
{
	struct rbt_node *p, *s;

	/* We've just removed a black node, and now we have too few black
	 * nodes along this path. Crawl up the tree and fix it. At each
	 * case in the following code:
	 *
	 *   - n is a black node
	 *   - s is not NULL
	 *   - the count through n is one less than the count through s
	 */
	p = n->parent;
	n = NULL;

	for (;;) {
		/* If we've hit the root, we're done. */
		if (!p)
			return;

		s = (p->left == n) ? p->right : p->left;

		/* If n's sibling is red, then its parent must be black,
		 * and it's sibling's children must be black. If we swap
		 * the colours of the sibling and the parent and rotate
		 * towards n, n will have a black sibling without affecting
		 * the counts.
		 *
		 * After this step, we've guaranteed that s is black.
		 *
		 * This operation invalidates the sibling, as per the
		 * normal effects of rotation.
		 */
		if (RBT_IS_RED(s)) {
			MAKE_RED(p);
			MAKE_BLACK(s);
			MARK_MODIFIED(s);

			if (n == p->left) {
				rotate_left(t, p);
				s = p->right;
			} else {
				rotate_right(t, p);
				s = p->left;
			}
		}

		/* If p, s and s's children are all black, then we can
		 * recolour s red to reduce the count along the sibling path,
		 * thus fixing the counts for the entire p subtree. However, p
		 * will then have a count too low by 1 with respect to its
		 * sibling, and we need to fix again from p.
		 *
		 * Otherwise, we need to perform the final fix by rotation.
		 */
		if (RBT_IS_BLACK(p) &&
		    RBT_IS_BLACK(s->left) && RBT_IS_BLACK(s->right)) {
			MAKE_RED(s);
			n = p;
			p = n->parent;
			continue;
		}

		break;
	}

	/* If s and its children are black, but the parent is red, then we
	 * can swap their colours. This increases the count through n, but
	 * doesn't affect the count through n, thus fixing the tree.
	 */
	if (RBT_IS_RED(p) &&
	    RBT_IS_BLACK(s->left) && RBT_IS_BLACK(s->right)) {
		MAKE_RED(s);
		MAKE_BLACK(p);
		return;
	}

	/* We've verified at this point that s must have at least one
	 * red child. We need to make sure that the child of s which
	 * desends in a different direction than n is red.
	 *
	 * We test for this and fix it by a recolouring and rotation
	 * which does not affect the counts.
	 *
	 * This invalidates s and one of its children, depending on
	 * the direction of rotation.
	 */
	if (n == p->left && RBT_IS_BLACK(s->right)) {
		MAKE_RED(s);
		MAKE_BLACK(s->left);
		MARK_MODIFIED(s);
		MARK_MODIFIED(s->left);
		rotate_right(t, s);
		s = s->parent;
	} else if (n == p->right && RBT_IS_BLACK(s->left)) {
		MAKE_RED(s);
		MAKE_BLACK(s->right);
		MARK_MODIFIED(s);
		MARK_MODIFIED(s->right);
		rotate_left(t, s);
		s = s->parent;
	}

	/* Finally, n has a sibling with a red child on the far path. We
	 * don't know the colour of p. We rotate p towards n, making p
	 * black first, and having s take over the colour of p. This
	 * increases the count through n by 1.
	 *
	 * The far child gets its black parent cut from the path, so
	 * we recolour it black to balance the count.
	 *
	 * s is invalidated by this step if it wasn't already.
	 */
	s->flags = p->flags | RBT_FLAG_MODIFIED;
	MAKE_BLACK(p);

	if (n == p->left) {
		MAKE_BLACK(s->right);
		rotate_left(t, p);
	} else {
		MAKE_BLACK(s->left);
		rotate_right(t, p);
	}
}

void rbt_remove(struct rbt *t, struct rbt_node *n)
{
	struct rbt_node *s;

	/* If the node has two children, we swap its position with that
	 * of the smallest node in the right subtree, thus reducing the
	 * problem to that of deleting a node with only one child.
	 */
	if (n->left && n->right)
		swap_with_successor(t, n);

	/* Everything from n up to the root will now have an invalid
	 * summary.
	 */
	rbt_mark_modified(n);

	/* Replace the pointer to n with that of its only child, if any.
	 * Then figure out what to do to preserve the tree properties.
	 */
	s = n->left ? n->left : n->right;
	fix_downptr(t, n->parent, n, s);

	/* If there was a child, it must be red (and n must have been black).
	 * Recolour it black and we're done.
	 */
	if (s) {
		s->parent = n->parent;
		MAKE_BLACK(s);
		return;
	}

	/* If the node we removed was red (and thus has no children), we
	 * haven't broken anything.
	 */
	if (RBT_IS_RED(n))
		return;

	/* Otherwise, handle the complicated cases. */
	repair_after_remove(t, n);
}

void rbt_mark_modified(struct rbt_node *n)
{
	while (n) {
		MARK_MODIFIED(n);
		n = n->parent;
	}
}
