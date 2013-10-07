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

#include "bint.h"

/* Invariants:
 *
 *    - the last chunk in a dynamically allocated chain is never all-zero
 */

#define COMPONENT_BITS		32
#define CHUNK_BITS		(COMPONENT_BITS * BINT_CHUNK_SIZE)

#define BITS_TO_CHUNKS(n)	((n + CHUNK_BITS - 1) / CHUNK_BITS)

/*************************************************************************
 * Some utility functions for managing chunks
 */

static void destroy_chain(struct bint_chunk *c)
{
	while (c) {
		struct bint_chunk *next = c->next;

		bint_chunk_free(c);
		c = next;
	}
}

static struct bint_chunk *alloc_chain(unsigned int length)
{
	struct bint_chunk *ch = NULL;

	while (length) {
		struct bint_chunk *n = bint_chunk_alloc();

		if (!n) {
			destroy_chain(ch);
			return NULL;
		}

		memset(n, 0, sizeof(*n));

		n->next = ch;
		ch = n;
		length--;
	}

	return ch;
}

static struct bint_chunk *reverse_chain(struct bint_chunk *c)
{
	struct bint_chunk *d = NULL;

	while (c) {
		struct bint_chunk *next = c->next;

		c->next = d;
		d = c;
		c = next;
	}

	return d;
}

static unsigned int chain_length(const struct bint_chunk *c)
{
	unsigned int count = 0;

	while (c) {
		count++;
		c = c->next;
	}

	return count;
}

static int chunk_is_zero(const struct bint_chunk *c)
{
	int i = 0;

	while (i < BINT_CHUNK_SIZE && !c->data[i])
		i++;

	return i >= BINT_CHUNK_SIZE;
}

static int is_zero(const struct bint *b)
{
	return !b->digits.next && chunk_is_zero(&b->digits);
}

static int compare_chunk(const struct bint_chunk *a,
			 const struct bint_chunk *b)
{
	int i;

	for (i = BINT_CHUNK_SIZE - 1; i >= 0; i--) {
		uint32_t ca = a->data[i];
		uint32_t cb = b->data[i];

		if (ca < cb)
			return -1;
		if (ca > cb)
			return 1;
	}

	return 0;
}

static void trim_chunks(struct bint *b)
{
	struct bint_chunk *stack;

	stack = reverse_chain(b->digits.next);

	/* Remove zero chunks */
	while (stack && chunk_is_zero(stack)) {
		struct bint_chunk *next = stack->next;

		bint_chunk_free(stack);
		stack = next;
	}

	b->digits.next = reverse_chain(stack);
}

static int extend_length(struct bint *b, unsigned int count)
{
	struct bint_chunk *c = &b->digits;

	while (c->next && count >= CHUNK_BITS) {
		count -= CHUNK_BITS;
		c = c->next;
	}

	if (count < CHUNK_BITS)
		return 0;

	c->next = alloc_chain(count / CHUNK_BITS);
	if (!c->next)
		return -1;

	return 0;
}

static int bit_length(unsigned int num)
{
	int count = 0;

	while (num) {
		count++;
		num >>= 1;
	}

	return count;
}

/************************************************************************
 * Magnitude addition/subtraction/compare.
 */

int mag_cmp(const struct bint *a, const struct bint *b)
{
	int last_diff = 0;
	const struct bint_chunk *ca = &a->digits;
	const struct bint_chunk *cb = &b->digits;

	/* Compare chunk-by-chunk, keeping track of the most significant
	 * difference in magnitude.
	 */
	while (ca && cb) {
		int diff = compare_chunk(ca, cb);

		if (diff)
			last_diff = diff;

		ca = ca->next;
		cb = cb->next;
	}

	/* If the numbers are different lengths, the longer number is
	 * bigger in magnitude.
	 */
	if (ca)
		last_diff = 1;
	else if (cb)
		last_diff = -1;

	return last_diff;
}

static int mag_add(struct bint *dst, const struct bint *src)
{
	unsigned int dst_len = bint_bit_length(dst);
	unsigned int src_len = bint_bit_length(src);
	unsigned int result_len = (dst_len > src_len ? dst_len : src_len) + 1;
	int carry = 0;
	struct bint_chunk *dc = &dst->digits;
	const struct bint_chunk *sc = &src->digits;

	/* Make sure we'll have enough room for the result. */
	if (extend_length(dst, result_len) < 0)
		return -1;

	/* Add corresponding digits */
	while (dc && sc) {
		int i;

		for (i = 0; i < BINT_CHUNK_SIZE; i++) {
			const uint64_t d = dc->data[i];
			const uint64_t c = sc->data[i];
			uint64_t r = d + c;

			if (carry)
				r++;

			dc->data[i] = r;
			carry = r >> COMPONENT_BITS;
		}

		dc = dc->next;
		sc = sc->next;
	}

	/* Carry through */
	while (carry && dc) {
		int i;

		for (i = 0; i < BINT_CHUNK_SIZE; i++) {
			uint64_t r = dc->data[i];

			if (carry)
				r++;

			dc->data[i] = r;
			carry = r & (1LL << COMPONENT_BITS);
		}

		dc = dc->next;
	}

	trim_chunks(dst);
	return 0;
}

/* dst may point to the same number as either big or small. It must
 * contain at least as many chunks as the big number.
 */
static void mag_sub(struct bint *dst,
		    const struct bint *big, const struct bint *small)
{
	struct bint_chunk *dc = &dst->digits;
	const struct bint_chunk *bc = &big->digits;
	const struct bint_chunk *sc = &small->digits;
	int borrow = 0;

	while (bc && sc) {
		int i;

		for (i = 0; i < BINT_CHUNK_SIZE; i++) {
			const uint64_t d = bc->data[i];
			const uint64_t c = sc->data[i];
			uint64_t r = d - c;

			if (borrow)
				r--;

			dc->data[i] = r;
			borrow = r >> COMPONENT_BITS;
		}

		dc = dc->next;
		bc = bc->next;
		sc = sc->next;
	}

	while (borrow && bc) {
		int i;

		for (i = 0; i < BINT_CHUNK_SIZE; i++) {
			uint64_t r = bc->data[i];

			if (borrow)
				r--;

			dc->data[i] = r;
			borrow = r >> COMPONENT_BITS;
		}

		bc = bc->next;
		dc = dc->next;
	}

	trim_chunks(dst);
}

static int do_subtract(struct bint *dst, const struct bint *src)
{
	if (mag_cmp(dst, src) < 0) {
		if (extend_length(dst, bint_bit_length(src)) < 0)
			return -1;

		mag_sub(dst, src, dst);
		dst->negative = !dst->negative;
		return 0;
	}

	mag_sub(dst, dst, src);
	return 0;
}

/************************************************************************
 * Chunk multiplication helper
 */

/* Multiply a + b, add the result and the incoming carry to r. Store
 * the outgoing carry before returning.
 */
static void chunk_mac(const struct bint_chunk *a,
		      const struct bint_chunk *b,
		      struct bint_chunk *r, uint32_t *carry_inout)
{
	uint32_t mm[BINT_CHUNK_SIZE * 2] = {0};
	uint32_t carry = 0;
	int i;

	/* Add carry and result */
	for (i = 0; i < BINT_CHUNK_SIZE; i++) {
		uint64_t rd = r->data[i];
		uint64_t cd = carry_inout[i];

		rd += cd;
		rd += carry;
		mm[i] = rd;
		carry = rd >> COMPONENT_BITS;
	}

	/* Add component products */
	if (a && b) {
		for (i = 0; i < BINT_CHUNK_SIZE; i++) {
			const uint64_t ad = a->data[i];
			int j;

			carry = 0;
			for (j = 0; j < BINT_CHUNK_SIZE; j++) {
				const uint64_t bd = b->data[j];
				uint64_t r = ad * bd;

				r += mm[i + j];
				r += carry;
				mm[i + j] = r;
				carry = r >> COMPONENT_BITS;
			}

			while (carry && (i + j) < BINT_CHUNK_SIZE * 2) {
				uint64_t r = mm[i + j];

				r += carry;
				mm[i + j] = r;
				carry = r >> COMPONENT_BITS;
				j++;
			}
		}
	}

	/* Copy result and carry out */
	for (i = 0; i < BINT_CHUNK_SIZE; i++) {
		r->data[i] = mm[i];
		carry_inout[i] = mm[i + BINT_CHUNK_SIZE];
	}
}

/************************************************************************
 * Public interface
 */

BINT_DEF_CONSTANT(bint_zero, 0);
BINT_DEF_CONSTANT(bint_one, 1);

void bint_init(struct bint *b, int32_t value)
{
	memset(b, 0, sizeof(*b));
	bint_set(b, value);
}

void bint_destroy(struct bint *b)
{
	destroy_chain(b->digits.next);
}

int bint_copy(struct bint *dst, const struct bint *src)
{
	struct bint_chunk *dc = &dst->digits;
	const struct bint_chunk *sc = &src->digits;

	/* Make sure there's enough room in the destination. */
	if (extend_length(dst, bint_bit_length(src)) < 0)
		return -1;

	/* Copy sign */
	dst->negative = src->negative;

	/* Trace the two chains, copying data */
	while (sc) {
		memcpy(dc->data, sc->data, sizeof(dc->data));

		if (!sc->next) {
			destroy_chain(dc->next);
			dc->next = NULL;
		}

		dc = dc->next;
		sc = sc->next;
	}

	return 0;
}

void bint_swap(struct bint *a, struct bint *b)
{
	struct bint tmp;

	memcpy(&tmp, a, sizeof(tmp));
	memcpy(a, b, sizeof(*a));
	memcpy(b, &tmp, sizeof(*b));
}

void bint_set(struct bint *b, int32_t value)
{
	destroy_chain(b->digits.next);
	memset(b, 0, sizeof(*b));

	b->digits.data[0] = abs(value);
	if (value < 0)
		b->negative = 1;
}

int32_t bint_get(const struct bint *b)
{
	int32_t v = b->digits.data[0];

	if (b->negative)
		v = -v;

	return v;
}

int bint_get_sign(const struct bint *b)
{
	if (is_zero(b))
		return 0;
	if (b->negative)
		return -1;

	return 1;
}

void bint_set_sign(struct bint *b, int sign)
{
	b->negative = sign < 0;
}

int bint_compare(const struct bint *a, const struct bint *b)
{
	int cmp;

	/* If exactly one of the numbers is negative, just compare signs. */
	if (a->negative && !b->negative)
		return -1;
	if (b->negative && !a->negative)
		return 1;

	cmp = mag_cmp(a, b);

	/* If both numbers are negative, the difference is in the opposite
	 * direction to the difference in magnitude.
	 */
	if (a->negative)
		cmp = -cmp;

	return cmp;
}

int bint_add(struct bint *dst, const struct bint *src)
{
	if ((dst->negative && src->negative) ||
	    !(dst->negative || src->negative))
		return mag_add(dst, src);

	return do_subtract(dst, src);
}

int bint_sub(struct bint *dst, const struct bint *src)
{
	if ((dst->negative && !src->negative) ||
	    (!dst->negative && src->negative))
		return mag_add(dst, src);

	return do_subtract(dst, src);
}

int bint_mul(struct bint *result, const struct bint *a, const struct bint *b)
{
	struct bint_chunk *rc;
	const struct bint_chunk *ac;

	/* Allocate room for the result */
	if (extend_length(result, bint_bit_length(a) +
				  bint_bit_length(b)) < 0)
		return -1;

	for (rc = &result->digits; rc; rc = rc->next)
		memset(rc->data, 0, sizeof(rc->data));

	/* Perform chunk-wise multiplication and carry */
	rc = &result->digits;
	for (ac = &a->digits; ac; ac = ac->next) {
		uint32_t carry[BINT_CHUNK_SIZE] = {0};
		struct bint_chunk *rcs = rc;
		const struct bint_chunk *bc;

		for (bc = &b->digits; bc; bc = bc->next) {
			chunk_mac(ac, bc, rcs, carry);
			rcs = rcs->next;
		}

		while (rcs) {
			chunk_mac(NULL, NULL, rcs, carry);
			rcs = rcs->next;
		}

		rc = rc->next;
	}

	bint_set_sign(result, bint_get_sign(a) * bint_get_sign(b));
	return 0;
}

int bint_div(struct bint *quotient, const struct bint *dividend,
	     const struct bint *divisor, struct bint *remainder)
{
	struct bint sdiv;
	struct bint rem;
	struct bint quot;
	unsigned int num_len = bint_bit_length(dividend);
	unsigned int den_len = bint_bit_length(divisor);
	int pos = 0;

	if (!den_len)
		return 1;

	bint_init(&rem, 0);
	bint_init(&sdiv, 0);
	bint_init(&quot, 0);

	/* Set up the shifted divisor */
	if (bint_copy(&sdiv, divisor) < 0)
		goto fail;

	if (den_len < num_len) {
		pos = num_len - den_len;
		if (bint_shift_left(&sdiv, pos) < 0)
			goto fail;
	}

	/* Set up remainder */
	if (bint_copy(&rem, dividend) < 0)
		goto fail;

	/* Work on magnitudes only -- handle sign later */
	sdiv.negative = 0;
	rem.negative = 0;

	/* Test and subtract for each bit position */
	while (pos >= 0) {
		if (mag_cmp(&sdiv, &rem) <= 0) {
			if (bint_bit_set(&quot, pos) < 0)
				goto fail;
			mag_sub(&rem, &rem, &sdiv);
		}

		pos--;
		bint_shift_right(&sdiv, 1);
	}

	/* Make the remainder the same sign as the divisor */
	rem.negative = divisor->negative;

	/* If the quotient is negative, borrow from the remainder */
	if ((divisor->negative && !dividend->negative) ||
	    (!divisor->negative && dividend->negative)) {
		if (bint_add(&quot, &bint_one) < 0)
			goto fail;
		if (bint_sub(&rem, divisor) < 0)
			goto fail;

		quot.negative = 1;
		rem.negative = divisor->negative;
	}

	/* Copy out results */
	if (remainder)
		bint_swap(remainder, &rem);
	if (quotient)
		bint_swap(quotient, &quot);

	bint_destroy(&rem);
	bint_destroy(&sdiv);
	bint_destroy(&quot);
	return 0;

fail:
	bint_destroy(&rem);
	bint_destroy(&sdiv);
	bint_destroy(&quot);
	return -1;
}

int bint_expt(struct bint *result,
	      const struct bint *base, const struct bint *exponent)
{
	struct bint r;
	struct bint b;
	struct bint p;
	unsigned int len = bint_bit_length(exponent);
	int i;

	bint_init(&r, 1);
	bint_init(&p, 0);
	bint_init(&b, 0);

	if (bint_copy(&b, base) < 0)
		goto fail;

	for (i = 0; i < len; i++) {
		if (bint_bit_get(exponent, i)) {
			if (bint_mul(&p, &r, &b) < 0)
				goto fail;
			bint_swap(&p, &r);
		}

		if (bint_mul(&p, &b, &b) < 0)
			goto fail;
		bint_swap(&p, &b);
	}

	bint_swap(result, &r);
	bint_destroy(&r);
	bint_destroy(&b);
	bint_destroy(&p);
	return 0;

fail:
	bint_destroy(&r);
	bint_destroy(&b);
	bint_destroy(&p);
	return -1;
}

int bint_bit_get(const struct bint *b, unsigned int pos)
{
	const struct bint_chunk *c = &b->digits;

	while (c && pos >= CHUNK_BITS) {
		c = c->next;
		pos -= CHUNK_BITS;
	}

	if (!c)
		return 0;

	if (c->data[pos / COMPONENT_BITS] & (1 << (pos % COMPONENT_BITS)))
		return 1;

	return 0;
}

unsigned int bint_bit_length(const struct bint *b)
{
	const struct bint_chunk *c = &b->digits;
	unsigned int length = 0;
	int i = BINT_CHUNK_SIZE - 1;
	uint32_t d;

	while (c->next) {
		length += CHUNK_BITS;
		c = c->next;
	}

	while (i >= 0 && !c->data[i])
		i--;

	/* This should only be possible if b has no extended chunk
	 * chain.
	 */
	if (i < 0)
		return 0;

	length += i * COMPONENT_BITS;
	d = c->data[i];

	while (d) {
		d >>= 1;
		length++;
	}

	return length;
}

int bint_bit_set(struct bint *b, unsigned int pos)
{
	struct bint_chunk *c = &b->digits;

	/* Traverse the existing chain as far as possible */
	while (pos >= CHUNK_BITS && c->next) {
		c = c->next;
		pos -= CHUNK_BITS;
	}

	/* Extend the chain and traverse it again if necessary */
	if (pos >= CHUNK_BITS) {
		c->next = alloc_chain(pos / CHUNK_BITS);
		if (!c->next)
			return -1;

		while (c->next)
			c = c->next;

		pos %= CHUNK_BITS;
	}

	/* Set the appropriate bit */
	c->data[pos / COMPONENT_BITS] |= (1 << (pos % COMPONENT_BITS));
	return 0;
}

void bint_bit_clear(struct bint *b, unsigned int pos)
{
	struct bint_chunk *c = &b->digits;

	/* Find the right chunk. Push chunks onto a stack as we pass them. */
	while (c && pos >= CHUNK_BITS) {
		c = c->next;
		pos -= CHUNK_BITS;
	}

	if (!c)
		return;

	/* Clear the bit */
	c->data[pos / COMPONENT_BITS] &= ~(1 << (pos % COMPONENT_BITS));

	/* Trim the stack if necessary */
	if (!c->next)
		trim_chunks(b);
}

int bint_or(struct bint *dst, const struct bint *src)
{
	struct bint_chunk *dc = &dst->digits;
	const struct bint_chunk *sc = &src->digits;

	/* Make sure the dst chain is at least as long as the src chain */
	while (dc->next && sc->next) {
		dc = dc->next;
		sc = sc->next;
	}

	if (sc->next) {
		dc->next = alloc_chain(chain_length(sc->next));
		if (!dc->next)
			return -1;
	}

	/* Trace the chain again, or'ing bits together */
	dc = &dst->digits;
	sc = &src->digits;
	while (sc) {
		int i;

		for (i = 0; i < BINT_CHUNK_SIZE; i++)
			dc->data[i] |= sc->data[i];

		dc = dc->next;
		sc = sc->next;
	}

	return 0;
}

void bint_and(struct bint *dst, const struct bint *src)
{
	struct bint_chunk *dc = &dst->digits;
	const struct bint_chunk *sc = &src->digits;
	int need_trim = 0;

	/* Trace the chain, and'ing bits together. Chop the length of the
	 * destination chain to that of the source chain.
	 */
	while (dc && sc) {
		int i;

		need_trim = 1;
		for (i = 0; i < BINT_CHUNK_SIZE; i++) {
			dc->data[i] &= sc->data[i];
			if (dc->data[i])
				need_trim = 0;
		}

		if (!sc->next) {
			destroy_chain(dc->next);
			dc->next = NULL;
		}

		dc = dc->next;
		sc = sc->next;
	}

	if (need_trim)
		trim_chunks(dst);
}

void bint_bic(struct bint *dst, const struct bint *src)
{
	struct bint_chunk *dc = &dst->digits;
	const struct bint_chunk *sc = &src->digits;
	int need_trim = 0;

	/* Trace the chain, clearing bits. */
	while (dc && sc) {
		int i;

		need_trim = 1;
		for (i = 0; i < BINT_CHUNK_SIZE; i++) {
			dc->data[i] &= ~sc->data[i];
			if (dc->data[i])
				need_trim = 0;
		}

		dc = dc->next;
		sc = sc->next;
	}

	if (need_trim)
		trim_chunks(dst);
}

int bint_xor(struct bint *dst, const struct bint *src)
{
	struct bint_chunk *dc = &dst->digits;
	const struct bint_chunk *sc = &src->digits;
	int need_trim = 0;

	/* Make sure the dst chain is at least as long as the src chain */
	while (dc->next && sc->next) {
		dc = dc->next;
		sc = sc->next;
	}

	if (sc->next) {
		dc->next = alloc_chain(chain_length(sc->next));
		if (!dc->next)
			return -1;
	}

	/* Trace the chain again, xor'ing bits together */
	dc = &dst->digits;
	sc = &src->digits;
	while (sc) {
		int i;

		need_trim = 1;
		for (i = 0; i < BINT_CHUNK_SIZE; i++) {
			dc->data[i] ^= sc->data[i];
			if (dc->data[i])
				need_trim = 0;
		}

		dc = dc->next;
		sc = sc->next;
	}

	if (need_trim)
		trim_chunks(dst);

	return 0;
}

int bint_shift_left(struct bint *b, unsigned int count)
{
	unsigned int cur_len = bint_bit_length(b);
	struct bint_chunk *dst;
	unsigned int bit_shift;
	uint32_t tmp[BINT_CHUNK_SIZE] = {0};
	uint32_t carry = 0;

	if (is_zero(b))
		return 0;

	/* Allocate extra space if necessary */
	if (extend_length(b, cur_len + count) < 0)
		return -1;

	/* Shift entire chunks */
	if (count >= CHUNK_BITS) {
		struct bint_chunk *top = reverse_chain(&b->digits);
		struct bint_chunk *src = top;

		dst = top;
		while (count >= CHUNK_BITS) {
			src = src->next;
			count -= CHUNK_BITS;
		}

		while (src) {
			memcpy(dst->data, src->data, sizeof(dst->data));
			dst = dst->next;
			src = src->next;
		}

		while (dst) {
			memset(dst->data, 0, sizeof(dst->data));
			dst = dst->next;
		}

		reverse_chain(top);
	}

	/* Shift components and bits */
	bit_shift = count % COMPONENT_BITS;
	count /= COMPONENT_BITS;

	for (dst = &b->digits; dst; dst = dst->next) {
		int i;
		uint32_t next_digits[BINT_CHUNK_SIZE] = {0};

		/* Shift components out the MSB end */
		for (i = 0; i < count; i++)
			next_digits[i] =
				dst->data[BINT_CHUNK_SIZE - count + i];

		/* Shift components within the chunk */
		for (i = BINT_CHUNK_SIZE - 1; i >= (signed)count; i--)
			dst->data[i] = dst->data[i - count];

		/* Shift components in the MSB end */
		for (i = 0; i < count; i++)
			dst->data[i] = tmp[i];

		/* Shift bits along */
		if (bit_shift) {
			for (i = 0; i < BINT_CHUNK_SIZE; i++) {
				const uint32_t d = dst->data[i];

				dst->data[i] = (d << bit_shift) | carry;
				carry = d >> (COMPONENT_BITS - bit_shift);
			}
		}

		memcpy(tmp, next_digits, sizeof(tmp));
	}

	return 0;
}

void bint_shift_right(struct bint *b, unsigned int count)
{
	struct bint_chunk *dst;
	struct bint_chunk *top;
	unsigned int bit_shift;
	uint32_t tmp[BINT_CHUNK_SIZE] = {0};
	uint32_t carry = 0;

	/* Shift entire chunks */
	if (count >= CHUNK_BITS) {
		struct bint_chunk *src = &b->digits;

		dst = src;
		while (src && count >= CHUNK_BITS) {
			count -= CHUNK_BITS;
			src = src->next;
		}

		while (src) {
			memcpy(dst->data, src->data, sizeof(dst->data));
			src = src->next;
			dst = dst->next;
		}

		if (dst) {
			destroy_chain(dst->next);
			memset(dst, 0, sizeof(*dst));
		}
	}

	if (count >= CHUNK_BITS)
		return;

	/* Shift components and bits */
	bit_shift = count % COMPONENT_BITS;
	count /= COMPONENT_BITS;

	top = reverse_chain(&b->digits);
	for (dst = top; dst; dst = dst->next) {
		int i;
		uint32_t next_digits[BINT_CHUNK_SIZE] = {0};

		/* Shift components out the LSB end */
		for (i = 0; i < count; i++)
			next_digits[BINT_CHUNK_SIZE - count + i] =
				dst->data[i];

		/* Shift components within the chunk */
		for (i = count; i < BINT_CHUNK_SIZE; i++)
			dst->data[i - count] = dst->data[i];

		/* Shift components in the MSB end */
		for (i = 0; i < count; i++)
			dst->data[BINT_CHUNK_SIZE - i - 1] =
				tmp[BINT_CHUNK_SIZE - i - 1];

		/* Shift bits */
		if (bit_shift) {
			for (i = BINT_CHUNK_SIZE - 1; i >= 0; i--) {
				uint32_t d = dst->data[i];

				dst->data[i] = (d >> bit_shift) | carry;
				carry = (d & ((1 << bit_shift) - 1))
					<< (COMPONENT_BITS - bit_shift);
			}
		}

		memcpy(tmp, next_digits, sizeof(tmp));
	}

	reverse_chain(top);
	trim_chunks(b);
}

int bint_digit_push(struct bint *b, unsigned int base, unsigned int digit)
{
	struct bint_chunk *c;
	uint32_t carry = 0;

	if (digit >= base)
		return -1;

	/* Make sure there's enough room for the result */
	if (extend_length(b, bint_bit_length(b) + bit_length(base) + 1) < 0)
		return -1;

	/* Short-multiply */
	for (c = &b->digits; c; c = c->next) {
		int i;

		for (i = 0; i < BINT_CHUNK_SIZE; i++) {
			uint64_t d = c->data[i];

			d *= base;
			d += carry;
			c->data[i] = d;
			carry = d >> COMPONENT_BITS;
		}
	}

	/* Add digit */
	carry = digit;
	for (c = &b->digits; c && carry; c = c->next) {
		int i;

		for (i = 0; i < BINT_CHUNK_SIZE; i++) {
			uint64_t d = c->data[i];

			d += carry;
			c->data[i] = d;
			carry = d >> COMPONENT_BITS;
		}
	}

	/* Remove unused chunks */
	trim_chunks(b);
	return 0;
}

unsigned int bint_digit_pop(struct bint *b, unsigned int base)
{
	struct bint_chunk *top;
	struct bint_chunk *c;
	uint64_t remainder = 0;

	if (!base)
		return 0;

	/* Short-divide */
	top = reverse_chain(&b->digits);
	for (c = top; c; c = c->next) {
		int i;

		for (i = BINT_CHUNK_SIZE - 1; i >= 0; i--) {
			uint64_t d = c->data[i];

			d += (remainder << ((uint64_t)COMPONENT_BITS));
			c->data[i] = d / base;
			remainder = d % base;
		}
	}

	/* Restore order and remove unused chunks */
	reverse_chain(top);
	trim_chunks(b);
	return remainder;
}

#ifndef __Windows__
__attribute__((weak)) struct bint_chunk *bint_chunk_alloc(void)
{
	struct bint_chunk *c = malloc(sizeof(struct bint_chunk));

	if (!c)
		return NULL;

	return c;
}

__attribute__((weak)) void bint_chunk_free(struct bint_chunk *c)
{
	free(c);
}
#endif
