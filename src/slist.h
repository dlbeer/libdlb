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

#ifndef SLIST_H_
#define SLIST_H_

#include <stdlib.h>

/* This data structure is a singly-linked list. It allows efficient
 * insertion at both ends, and efficient removal from the beginning of
 * the list.
 */
struct slist_node {
	struct slist_node	*next;
};

struct slist {
	struct slist_node	*start;
	struct slist_node	*end;
};

/* Check to see if a list contains anything. */
static inline int slist_is_empty(const struct slist *s)
{
	return !s->start;
}

/* Create an empty list */
void slist_init(struct slist *s);

/* Add an item to the beginning of a list */
void slist_push(struct slist *s, struct slist_node *n);

/* Remove the first item from the beginning of a list */
struct slist_node *slist_pop(struct slist *s);

/* Add an item to the end of a list. */
void slist_append(struct slist *s, struct slist_node *n);

#endif
