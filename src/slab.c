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
#include "slab.h"

#define MIN_OBJ		8
#define MIN_SLAB	32768
#define ALIGN		(sizeof(struct slab_info *))

/* Slabs are 2^n multiples of MIN_SLAB, and layed out as follows:
 *
 * | OBJ | PTR | OBJ | PTR | OBJ | PTR | ... | INFO
 *
 * OBJ: an allocated object, or struct slab_free
 * PTR: a pointer to the slab's info block
 * INFO: a struct slab_info
 *
 * There is no padding between these parts.
 */

/* Each slab has this tag at the end */
struct slab_info {
	struct list_node	link; /* MUST BE FIRST */
	struct list_node	free_list;
	int			free_count;
};

/* Each free object in the slab is occupied by this struct */
struct slab_free {
	struct list_node	link; /* MUST BE FIRST */
};

void slab_init(struct slab *s, int objsize)
{
	int n;

	/* The object must be able to be occupied by a free object struct,
	 * and it must be aligned for the following pointer.
	 */
	if (objsize < sizeof(struct slab_free))
		objsize = sizeof(struct slab_free);
	objsize += (ALIGN - (objsize % ALIGN)) % ALIGN;

	/* Calculate the number of objects per slab */
	n = (MIN_SLAB - sizeof(struct slab_info)) /
		(objsize + sizeof(struct slab_info *));
	if (n < MIN_OBJ)
		n = MIN_OBJ;

	/* Calculate the slab layout */
	s->objsize = objsize;
	s->count = n;
	s->info_offset = (objsize + sizeof(struct slab_info *)) * s->count;
	s->slabsize = s->info_offset + sizeof(struct slab_info);

	/* Set up empty slab lists */
	list_init(&s->full);
	list_init(&s->partial);
}

static void free_slab(struct slab *s, struct slab_info *inf)
{
	list_remove(&inf->link);
	free(((char *)inf) - s->info_offset);
}

static void free_all_slabs(struct slab *s, struct list_node *list)
{
	while (!list_is_empty(list))
		free_slab(s, (struct slab_info *)list->next);
}

void slab_free_all(struct slab *s)
{
	free_all_slabs(s, &s->full);
	free_all_slabs(s, &s->partial);
}

static int alloc_new_slab(struct slab *s)
{
	char *slab = malloc(s->slabsize);
	struct slab_info *inf;
	int i;

	if (!slab)
		return -1;

	/* Set up the info block */
	inf = (struct slab_info *)(slab + s->info_offset);

	list_init(&inf->free_list);
	inf->free_count = s->count;

	/* Set up the free objects and back pointers, and add them to the
	 * slab's free list.
	 */
	for (i = 0; i < s->count; i++) {
		char *slot = slab +
			i * (s->objsize + sizeof(struct slab_info *));
		struct slab_free *fr = (struct slab_free *)slot;

		list_insert(&fr->link, &inf->free_list);
		*(struct slab_info **)(slot + s->objsize) = inf;
	}

	/* Add the slab to the partial list */
	list_insert(&inf->link, &s->partial);
	return 0;
}

void *slab_alloc(struct slab *s)
{
	struct slab_info *inf;
	struct slab_free *fr;

	if (list_is_empty(&s->partial) && alloc_new_slab(s) < 0)
		return NULL;

	/* Get the first non-full slab, and the first free object within
	 * that slab.
	 */
	inf = (struct slab_info *)s->partial.next;
	fr = (struct slab_free *)inf->free_list.next;

	/* Remove the object from the slab's free list */
	list_remove(&fr->link);
	inf->free_count--;

	/* If the slab is now completely used, remove it from the partial
	 * list and put it on the full list.
	 */
	if (list_is_empty(&inf->free_list)) {
		list_remove(&inf->link);
		list_insert(&inf->link, &s->full);
	}

	return fr;
}

void slab_free(struct slab *s, void *obj)
{
	struct slab_free *fr = (struct slab_free *)obj;
	struct slab_info *inf =
		*(struct slab_info **)(((char *)obj) + s->objsize);

	/* If the slab is completely full, it's about to become partial,
	 * so remove it from the full list and put it on the partial
	 * list.
	 */
	if (list_is_empty(&inf->free_list)) {
		list_remove(&inf->link);
		list_insert(&inf->link, &s->partial);
	}

	/* Put the object back into the slab's free-list */
	list_insert(&fr->link, &inf->free_list);
	inf->free_count++;

	/* If the slab is now completely empty, release it */
	if (inf->free_count >= s->count)
		free_slab(s, inf);
}
