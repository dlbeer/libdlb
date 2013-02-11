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

#include "cbuf.h"

void cbuf_init(struct cbuf *c, uint8_t *data, size_t capacity)
{
	c->data = data;
	c->capacity = capacity;
	c->head = 0;
	c->size = 0;
}

void cbuf_clear(struct cbuf *c)
{
	c->head = 0;
	c->size = 0;
}

void cbuf_head_advance(struct cbuf *c, size_t count)
{
	if (count > c->size)
		count = c->size;

	c->size -= count;
	c->head = (c->head + count) % c->capacity;
}

void cbuf_tail_advance(struct cbuf *c, size_t count)
{
	const size_t a = cbuf_avail(c);

	if (count > a)
		count = a;

	c->size += count;
}

size_t cbuf_move_in(struct cbuf *c, const uint8_t *data, size_t max_size)
{
	size_t count = 0;

	for (;;) {
		const size_t tail_size = cbuf_tail_size(c);
		const size_t src_size = max_size - count;
		const size_t x = tail_size < src_size ? tail_size : src_size;

		if (!x)
			break;

		memcpy(cbuf_tail_data(c), data + count, x);
		cbuf_tail_advance(c, x);
		count += x;
	}

	return count;
}

size_t cbuf_move_out(struct cbuf *c, uint8_t *data, size_t max_size)
{
	size_t count = 0;

	for (;;) {
		const size_t head_size = cbuf_head_size(c);
		const size_t dst_size = max_size - count;
		const size_t x = head_size < dst_size ? head_size : dst_size;

		if (!x)
			break;

		memcpy(data + count, cbuf_head_data(c), x);
		cbuf_head_advance(c, x);
		count += x;
	}

	return count;
}

size_t cbuf_move(struct cbuf *dst, struct cbuf *src, size_t max_size)
{
	size_t count = 0;

	if (!max_size)
		max_size = src->size;

	for (;;) {
		const size_t head_size = cbuf_head_size(src);
		const size_t tail_size = cbuf_tail_size(dst);
		const size_t limit = max_size - count;
		const size_t x = head_size < tail_size ?
			head_size : tail_size;
		const size_t y = x < limit ? x : limit;

		if (!y)
			break;

		memcpy(cbuf_tail_data(dst), cbuf_head_data(src), y);
		cbuf_tail_advance(dst, y);
		cbuf_head_advance(src, y);
		count += y;
	}

	return count;
}
