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

#ifndef VECTOR_H_
#define VECTOR_H_

/* A vector is a flexible array type. It can be used to hold elements of
 * any type.
 *
 * elemsize: size in bytes of each element
 * capacity: maximum number of elements that ptr can hold
 * size:     number of elements currently held
 */
struct vector {
	void            *ptr;
	unsigned int    elemsize;
	unsigned int    capacity;
	unsigned int    size;
};

/* Create and destroy vectors */
void vector_init(struct vector *v, unsigned int elemsize);
void vector_destroy(struct vector *v);

/* Remove all elements */
void vector_clear(struct vector *v);

/* Alter the size of a vector. If the new size is greater than the
 * current size, the new elements will be uninitialized.
 *
 * Returns 0 on success or -1 if memory could not be allocated.
 */
int vector_resize(struct vector *v, unsigned int new_size);

/* Append any number of elements to the end of a vector, reallocating if
 * necessary. Returns 0 on success or -1 if memory could not be allocated.
 */
int vector_push(struct vector *v, const void *data, unsigned int count);

/* Remove the last element from a vector. */
void vector_pop(struct vector *v, unsigned int count);

/* Dereference a vector, giving an expression for the element of type t at
 * position i in vector v. Use as follows:
 *
 *    struct vector v;
 *
 *    VECTOR_AT(v, 3, int) = 57;
 *    *VECTOR_PTR(v, 3, int) = 57;
 */
#define VECTOR_AT(v, i, t) (*VECTOR_PTR(v, i, t))
#define VECTOR_PTR(v, i, t) \
	((t *)((char *)(v).ptr + (i) * (v).elemsize))

#endif
