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

#ifndef ARENA_H_
#define ARENA_H_

#include <stddef.h>

/* This object is a memory allocator for fixed-size heaps. Initialize it
 * with a large chunk of memory and it will provide implementations of
 * malloc, free and realloc.
 *
 * The algorithm used is buddy allocation. All operations are O(log N)
 * in the size of the heap.
 */
#define ARENA_ORDERS		32

struct arena_block;

struct arena {
	void			*base;
	size_t			size;

	struct arena_block	*blocks[ARENA_ORDERS];
};

/* Initialize a new arena */
void arena_init(struct arena *a, void *base, size_t size);

/* Allocate and free blocks */
void *arena_alloc(struct arena *a, size_t size);
void arena_free(struct arena *a, void *ptr);

/* Reallocate a block (if necessary) */
void *arena_realloc(struct arena *a, void *ptr, size_t new_size);

/* Count up free space available in blocks */
size_t arena_count_free(const struct arena *a);

#endif
