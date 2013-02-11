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

#ifndef LIST_H_
#define LIST_H_

/* This is the list node definition. It's intended to be embedded within
 * another data structure.
 *
 * All lists are circular and doubly-linked. So, to iterate over the
 * members of a list, do something like this:
 *
 *     struct list_node *n;
 *
 *     for (n = list->next; n != list; n = n->next) {
 *             ...
 *     }
 *
 * An empty list must be created first with list_init(). This sets the
 * next and prev pointers to the list head itself.
 */
struct list_node {
	struct list_node	*next;
	struct list_node	*prev;
};

/* Check to see if a list contains anything. */
#define LIST_EMPTY(h) ((h)->next == (h))

/* Create an empty list */
void list_init(struct list_node *head);

/* Add an item to a list. The item will appear before the item
 * specified as after.
 */
void list_insert(struct list_node *item, struct list_node *after);

/* Remove a node from its containing list. */
void list_remove(struct list_node *item);

#endif