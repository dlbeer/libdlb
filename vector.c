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
#include <string.h>
#include "vector.h"

void vector_init(struct vector *v, unsigned int elemsize)
{
	memset(v, 0, sizeof(*v));
	v->elemsize = elemsize;
}

void vector_clear(struct vector *v)
{
	if (v->ptr)
		free(v->ptr);

	v->ptr = NULL;
	v->capacity = 0;
	v->size = 0;
}

void vector_destroy(struct vector *v)
{
	vector_clear(v);
	memset(v, 0, sizeof(*v));
}

int size_for(struct vector *v, unsigned int needed)
{
	int cap = needed;
	void *new_ptr;

	/* Find the smallest power of 2 which is greater than the
	 * necessary capacity.
	 */
	while (cap & (cap - 1))
		cap &= (cap - 1);
	if (cap < needed)
		cap <<= 1;

	/* Don't allocate fewer than 8 elements */
	if (cap < 8)
		cap = 8;

	/* If we're already big enough, don't reallocate.
	 *
	 * Similarly, don't reallocate to a smaller size unless we're
	 * using far too much more space than is necessary.
	 */
	if (v->capacity >= cap && v->capacity <= cap * 2)
		return 0;

	new_ptr = realloc(v->ptr, cap * v->elemsize);
	if (!new_ptr) {
		if (v->capacity >= needed)
			return 0;
		return -1;
	}

	v->ptr = new_ptr;
	v->capacity = cap;

	return 0;
}

int vector_resize(struct vector *v, unsigned int new_size)
{
	if (size_for(v, new_size) < 0)
		return -1;

	v->size = new_size;
	return 0;
}

int vector_push(struct vector *v, const void *data, unsigned int count)
{
	int needed = v->size + count;

	if (size_for(v, needed) < 0)
		return -1;

	memcpy((char *)v->ptr + v->size * v->elemsize,
	       data,
	       count * v->elemsize);
	v->size += count;

	return 0;
}

void vector_pop(struct vector *v, unsigned int count)
{
	if (count > v->size)
		count = v->size;

	vector_resize(v, v->size - count);
}
