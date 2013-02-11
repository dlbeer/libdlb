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

#ifndef CBUF_H_
#define CBUF_H_

#include <stdint.h>
#include <string.h>

/* Circular buffer library. This implements a circular buffer which is
 * using up to (and including) the allocated capacity.
 *
 * It provides functions for returning contiguous head/tail portions,
 * whose positions remain stable during modifications to the other side
 * of the queue.
 *
 * None of the API functions will touch the actual data pointed to --
 * it's safe to make copies of the cbuf structure to use for iterating
 * over data.
 */
struct cbuf {
	uint8_t		*data;
	size_t		capacity;
	size_t		head;
	size_t		size;
};

/* Initialize a circular buffer. */
void cbuf_init(struct cbuf *c, uint8_t *data, size_t capacity);

/* Make a shallow copy of a circular buffer, without altering the
 * original or the referenced data.
 */
static inline void cbuf_copy(struct cbuf *dst, const struct cbuf *src)
{
	memcpy(dst, src, sizeof(*dst));
}

/* Clear a circular buffer, discarding all contents. This function
 * should not be used if there are active references to head/tail
 * regions.
 */
void cbuf_clear(struct cbuf *c);

/* Return the total amount of space used/available. */
static inline size_t cbuf_used(const struct cbuf *c)
{
	return c->size;
}

static inline size_t cbuf_avail(const struct cbuf *c)
{
	return c->capacity - c->size;
}

/* Return the head/tail indices */
static inline size_t cbuf_head(const struct cbuf *c)
{
	return c->head;
}

static inline size_t cbuf_tail(const struct cbuf *c)
{
	return (c->head + c->size) % c->capacity;
}

/* Obtain a pointer and size for a contiguous consumer area (head). */
static inline uint8_t *cbuf_head_data(const struct cbuf *c)
{
	return c->data + c->head;
}

static inline size_t cbuf_head_size(const struct cbuf *c)
{
	const size_t r = c->capacity - c->head;

	return r < c->size ? r : c->size;
}

/* Obtain a pointer and size for a contiguous producer area (tail). */
static inline uint8_t *cbuf_tail_data(const struct cbuf *c)
{
	return c->data + cbuf_tail(c);
}

static inline size_t cbuf_tail_size(const struct cbuf *c)
{
	const size_t t = cbuf_tail(c);

	return c->capacity - (t > c->size ? t : c->size);
}

/* Advance producer/consumer indices */
void cbuf_head_advance(struct cbuf *c, size_t count);
void cbuf_tail_advance(struct cbuf *c, size_t count);

/* Helpers for transferring data to/from flat memory areas. Each of
 * these functions returns the number of bytes transferred.
 */
size_t cbuf_move_in(struct cbuf *c, const uint8_t *data, size_t max_size);
size_t cbuf_move_out(struct cbuf *c, uint8_t *data, size_t max_size);

/* Transfer data between two circular buffers. If max_size is 0, the
 * maximum possible amount will be transferred.
 */
size_t cbuf_move(struct cbuf *dst, struct cbuf *src, size_t max_size);

#endif
