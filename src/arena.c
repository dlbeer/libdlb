/* libdlb - data structures and utilities library
 * Copyright (C) 2014 Daniel Beer <dlbeer@gmail.com>
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

#include <stdint.h>
#include <string.h>
#include "arena.h"

struct arena_block {
	struct arena_block	*prev;
	struct arena_block	*next;

	uint8_t			order;
	uint8_t			is_free;
};

/* Link/unlink items from block lists */
static void block_link(struct arena_block **list, struct arena_block *blk)
{
	blk->next = *list;
	blk->prev = NULL;

	if (blk->next)
		blk->next->prev = blk;

	*list = blk;
}

static void block_unlink(struct arena_block **list, struct arena_block *blk)
{
	if (blk->next)
		blk->next->prev = blk->prev;

	if (blk->prev)
		blk->prev->next = blk->next;
	else
		*list = blk->next;
}

/* Initialize a new arena */
void arena_init(struct arena *a, void *base, size_t size)
{
	int o = ARENA_ORDERS - 1;

	memset(a, 0, sizeof(*a));
	a->base = base;
	a->size = size;

	for (;;) {
		const size_t bs = 1 << o;
		struct arena_block *blk = (struct arena_block *)base;

		if (bs < sizeof(struct arena_block))
			break;

		if (bs > size) {
			o--;
			continue;
		}

		blk->is_free = 1;
		blk->order = o;

		block_link(&a->blocks[o], blk);

		size -= bs;
		base = ((char *)base) + bs;
	}
}

/* Allocate a new block */
void *arena_alloc(struct arena *a, size_t size)
{
	struct arena_block *b;
	int o = 1;
	int i;

	/* Figure out the required block order */
	if (size >> (ARENA_ORDERS - 1))
		return NULL;
	while ((o < ARENA_ORDERS) &&
	       (1 << o) < size + sizeof(struct arena_block))
		o++;
	if (o >= ARENA_ORDERS)
		return NULL;

	/* Find the smallest free block at least as big */
	i = o;
	while ((i < ARENA_ORDERS) && !a->blocks[i])
		i++;
	if (i >= ARENA_ORDERS)
		return NULL;

	/* Split blocks until we have one of the optimal order */
	while (i > o) {
		struct arena_block *c;

		b = a->blocks[i];
		block_unlink(&a->blocks[i], b);

		i--;
		c = (struct arena_block *)(((char *)b) + (1 << i));
		c->is_free = 1;
		c->order = i;
		b->order = i;

		block_link(&a->blocks[i], b);
		block_link(&a->blocks[i], c);
	}

	/* Return the free block */
	b = a->blocks[o];
	block_unlink(&a->blocks[o], b);

	b->is_free = 0;
	return b + 1;
}

static inline struct arena_block *buddy_of(void *base, struct arena_block *b,
					   int order)
{
	const size_t offset = ((char *)b) - ((char *)base);
	const size_t boff = offset ^ (1 << order);

	return (struct arena_block *)(((char *)base) + boff);
}

void arena_free(struct arena *a, void *ptr)
{
	struct arena_block *b = ((struct arena_block *)ptr) - 1;
	int o;

	if (!ptr)
		return;

	o = b->order;
	b->is_free = 1;
	block_link(&a->blocks[o], b);

	/* Join blocks */
	while (o + 1 < ARENA_ORDERS) {
		struct arena_block *c = buddy_of(a->base, b, o);

		if ((char *)(c + 1) >
		    ((char *)a->base) + a->size)
			break;

		if (!c->is_free || c->order != o)
			break;

		if (b > c) {
			struct arena_block *t = c;

			c = b;
			b = t;
		}

		block_unlink(&a->blocks[o], b);
		block_unlink(&a->blocks[o], c);

		b->order = ++o;
		block_link(&a->blocks[o], b);
	}
}

void *arena_realloc(struct arena *a, void *ptr, size_t size)
{
	struct arena_block *b = ((struct arena_block *)ptr) - 1;
	size_t copy_size;
	void *new_ptr;
	int o = 1;

	/* No pointer? Revert to malloc */
	if (!ptr)
		return arena_alloc(a, size);

	/* Figure out the required order */
	if (size >> (ARENA_ORDERS - 1))
		return NULL;

	while ((o < ARENA_ORDERS) &&
	       (1 << o) < size + sizeof(struct arena_block))
		o++;
	if (o >= ARENA_ORDERS)
		return NULL;

	/* Is about right already? */
	if ((b->order == o) || (o + 1 == b->order))
		return ptr;

	/* Allocate new and copy */
	new_ptr = arena_alloc(a, size);
	if (!new_ptr)
		return NULL;

	copy_size = (1 << b->order) - sizeof(struct arena_block);
	if (copy_size > size)
		copy_size = size;

	memcpy(new_ptr, ptr, copy_size);

	arena_free(a, ptr);
	return new_ptr;
}

size_t arena_count_free(const struct arena *a)
{
	size_t count = 0;
	int i;

	for (i = 0; i < ARENA_ORDERS; i++) {
		size_t c = 0;
		struct arena_block *b = a->blocks[i];

		while (b) {
			b = b->next;
			c++;
		}

		count += c * ((1 << i) - sizeof(struct arena_block));
	}

	return count;
}
