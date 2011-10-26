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

#ifndef ISTR_H_
#define ISTR_H_

#include "hash.h"
#include "strbuf.h"
#include "slab.h"

/* This structure is an immutable reference-counted string pool. It allows
 * for reduced memory usage and fast comparison when dealing with large
 * numbers of strings.
 */
struct istr_pool {
	struct strbuf		text;
	struct hash		index;
	struct slab		descs;

	unsigned int		gc_threshold;
};

struct istr_desc {
	struct hash_node	node;
	struct istr_pool	*owner;
	unsigned int		offset;
	unsigned int		length;
	unsigned int		refcnt;
};

/* This is the type of immutable string references. */
typedef const struct istr_desc *istr_t;

/* Initialize and destroy a string pool. */
void istr_pool_init(struct istr_pool *p);
void istr_pool_destroy(struct istr_pool *p);

/* Allocate a string from the pool. Strings may have an explicit length,
 * in which case they can contain embedded nuls. Otherwise, if the length
 * given is negative, it will be calculated automatically with strlen().
 *
 * This function returns a reference to an immutable string on success, or
 * NULL if memory allocation failed. istr_unref() should be called to
 * release the allocated string when it's no longer needed by the caller.
 */
istr_t istr_pool_alloc(struct istr_pool *p, const char *text, int length);

/* Garbage-collect the pool. This is done automatically when necessary on
 * allocated, but can also be performed manually.
 *
 * Returns 0 on success, or -1 if memory couldn't be allocated (not a fatal
 * error).
 */
int istr_pool_gc(struct istr_pool *p);

/* Increment and decrement string reference counts. */
static inline void istr_ref(istr_t s)
{
	((struct istr_desc *)s)->refcnt++;
}

static inline void istr_unref(istr_t s)
{
	((struct istr_desc *)s)->refcnt--;
}

/* Compare two strings, returning negative/zero/positive. */
int istr_compare(istr_t a, istr_t b);

/* Fast comparison function. If two strings are known to be from the same
 * pool, you can test for equality using istr_eq(). Otherwise, istr_equal()
 * can be used.
 */
static inline int istr_eq(istr_t a, istr_t b)
{
	return a == b;
}

int istr_equal(istr_t a, istr_t b);

/* Obtain length and text from an immutable string reference. Note that the
 * text pointer returned is valid only up to the next pool allocation or
 * garbage collection.
 *
 * The reference count can also be queried. This may be useful for using a
 * string pool to count word frequencies.
 */
static inline const char *istr_text(istr_t s)
{
	return s->owner->text.text + s->offset;
}

static inline unsigned int istr_length(istr_t s)
{
	return s->length;
}

static inline unsigned int istr_refcnt(istr_t s)
{
	return s->refcnt;
}

#endif
