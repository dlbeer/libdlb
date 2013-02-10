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

#ifndef SLAB_H_
#define SLAB_H_

#include "list.h"

struct slab {
	int			objsize;
	int			slabsize;
	int			count;
	int			info_offset;

	struct list_node	full;
	struct list_node	partial;
};

/* Set up a slab allocator */
void slab_init(struct slab *s, int objsize);

/* Free all objects */
void slab_free_all(struct slab *s);

/* Allocate an object from the slab cache */
void *slab_alloc(struct slab *s);

/* Return an object to the slab cache */
void slab_free(struct slab *s, void *obj);

#endif
