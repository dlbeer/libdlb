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

#include <assert.h>
#include <stdio.h>
#include "prng.h"
#include "cbuf.h"

#define N		65536
#define MAX_XFER	8192

static uint8_t pattern[N];
static uint8_t out[N];

static uint8_t data_a[4096];
static uint8_t data_b[4096];

static struct cbuf buf_a;
static struct cbuf buf_b;

static prng_t prng;

static void init_pattern(void)
{
	int i;

	for (i = 0; i < sizeof(pattern); i++)
		pattern[i] = prng_next(&prng);
}

static size_t choose(size_t n, size_t suggest)
{
	return n < suggest ? n : suggest;
}

static void check_empty(struct cbuf *c)
{
	assert(!cbuf_used(&buf_a));
	assert(cbuf_avail(&buf_a) == c->capacity);
}

static void stream_pattern(size_t is, size_t ms, size_t os)
{
	size_t pat_ptr = 0;
	size_t out_ptr = 0;

	check_empty(&buf_a);
	check_empty(&buf_b);
	memset(out, 0, N);

	while (out_ptr < N) {
		/* Transfer a random amount to buf_a */
		pat_ptr += cbuf_move_in(&buf_a,
			pattern + pat_ptr, choose(N - pat_ptr, is));

		/* Transfer a random amount between buf_a and buf_b */
		cbuf_move(&buf_b, &buf_a, choose(cbuf_used(&buf_a), ms));

		/* Transfer a random amount out again */
		out_ptr += cbuf_move_out(&buf_b,
			out + out_ptr, choose(N - out_ptr, os));
	}

	check_empty(&buf_a);
	check_empty(&buf_b);
	assert(pat_ptr == N);
	assert(!memcmp(out, pattern, N));
}

int main(void)
{
	prng_init(&prng, 1);
	init_pattern();

	cbuf_init(&buf_a, data_a, sizeof(data_a));
	cbuf_init(&buf_b, data_b, sizeof(data_b));

	stream_pattern(4096, 4096, 4096);
	stream_pattern(4096, 256, 4096);
	stream_pattern(11, 13, 7);

	return 0;
}
