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

#ifndef BINT_H_
#define BINT_H_

#include <stdint.h>
#include <stdlib.h>

/* This file defines big (arbitrary-precision) integer types. They are
 * implemented as singly-linked lists of chunks, with the first chunk
 * embedded in the type itself.
 *
 * This means that for small-ish integers, no memory allocation is
 * required. Only integers that overflow the chunk size will degrade
 * to dynamic memory allocation.
 *
 * The number of bits represented by a single chunk is BINT_CHUNK_SIZE * 32.
 *
 * Operations that fail will leave their operands in the same state they
 * were before the operation was attempted.
 *
 * All struct bint * pointers passed as arguments must point to initialized
 * structures. The only exception is the argument passed to bint_init().
 */

#ifndef BINT_CHUNK_SIZE
#define BINT_CHUNK_SIZE		4
#endif

struct bint_chunk {
	uint32_t		data[BINT_CHUNK_SIZE];
	struct bint_chunk	*next;
};

struct bint {
	int			negative;
	struct bint_chunk	digits;
};

/* Constant definitions. Constants are statically initialized, and
 * don't need to be destroyed.
 */
#define BINT_DEF_CONSTANT(name, value) \
	const struct bint name = { \
		.negative = (value) < 0, \
		.digits = { \
			.next = NULL, \
			.data = {(value) < 0 ? -(value) : (value), 0} \
		} \
	};

extern const struct bint bint_zero;
extern const struct bint bint_one;

/* Initialize and destroy big integers. */
void bint_init(struct bint *b, int32_t value);
void bint_destroy(struct bint *b);

/* Copy an integer. The destination is expected to be initialized
 * already.
 *
 * Returns 0 on success or -1 if memory couldn't be allocated. If this
 * operation fails, the destination is left unchanged.
 */
int bint_copy(struct bint *dst, const struct bint *src);

/* Swap two integers. This operation never fails. */
void bint_swap(struct bint *a, struct bint *b);

/* Set the value of an integer. This operation never fails, because a
 * primitive int can always fit inside a single chunk.
 */
void bint_set(struct bint *b, int32_t value);

/* Cast big integer values back to primitive ints.
 *
 * Note that bint_get() will automatically cast the value, losing
 * precision if it doesn't fit.
 */
int32_t bint_get(const struct bint *b);

/* Fetch and set the sign of an integer. Note that setting the sign of
 * a zero-valued integer has no effect.
 *
 * bint_get_sign() returns -1/0/1 depending on whether the value is negative,
 * zero or positive.
 */
int bint_get_sign(const struct bint *b);
void bint_set_sign(struct bint *b, int sign);

/* Compare two integers. Returns -1/0/1 depending on whether the first
 * value is less than, equal, or greater than the second.
 */
int bint_compare(const struct bint *a, const struct bint *b);

/* Add/subtract the second integer to/from the first. Returns 0 on success
 * or -1 if memory couldn't be allocated for an extra-large result.
 */
int bint_add(struct bint *dst, const struct bint *src);
int bint_sub(struct bint *dst, const struct bint *src);

/* Multiply two big integers to produce a third. Returns 0 on success or
 * -1 if memory couldn't be allocated.
 *
 * The two source values may point to the same integer, but neither of
 * them are allowed to point to the result.
 */
int bint_mul(struct bint *result, const struct bint *a, const struct bint *b);

/* Integer division. This operation divides the dividend by the divisor, and
 * optionally returns the quotient and remainder. The remainder is
 * of the same sign as the divisor and satisfies:
 *
 *    dividend = divisor * quotient + remainder
 *
 * Returns 0 on success, -1 if memory couldn't be allocated, and 1 if the
 * divisor was zero. Either, or both, of divisor/remainder may be NULL.
 */
int bint_div(struct bint *quotient, const struct bint *dividend,
	     const struct bint *divisor, struct bint *remainder);

/* Exponentiation. Raises the base to the power of the exponent and returns
 * the result. Returns 0 on success or -1 if memory couldn't be allocated.
 */
int bint_expt(struct bint *result,
	      const struct bint *base, const struct bint *exponent);

/* Bitwise operations: bit set, length, clear and test. Set may fail if memory
 * needs to be allocated, in which case -1 is returned.
 *
 * bint_bit_length() returns the index of the position after the most
 * significant 1.
 */
int bint_bit_get(const struct bint *b, unsigned int pos);
unsigned int bint_bit_length(const struct bint *b);

int bint_bit_set(struct bint *b, unsigned int pos);
void bint_bit_clear(struct bint *b, unsigned int pos);

/* Bitwise operations: and, or and xor. All three of these may required memory
 * allocation, and will return -1 on failure. Since bitwise not can't be
 * implemented (because the result would logically be of infinite length), we
 * provide an operation "bic":
 *
 *    A bic B == A & ~B
 *
 * Note that these operations ignore the sign of the integer.
 */
int bint_or(struct bint *dst, const struct bint *src);
void bint_and(struct bint *dst, const struct bint *src);
void bint_bic(struct bint *dst, const struct bint *src);
int bint_xor(struct bint *dst, const struct bint *src);

/* Bitwise operations: shift left and right. Shift left might fail if memory
 * can't be allocated, and will return -1 on failure. Shift right always
 * succeeds.
 *
 * These operations ignore the sign of the integer.
 */
int bint_shift_left(struct bint *b, unsigned int count);
void bint_shift_right(struct bint *b, unsigned int count);

/* Base conversion functions. These functions treat the integer as a
 * stack of digits in the given base, with the top of the stack being
 * the least significant digit. They both ignore the sign of the integer.
 *
 * bint_digit_push() computes: b = b * base + digit. It returns 0 on
 * success or -1 if memory couldn't be allocated.
 *
 * bint_digit_pop() divides b by the base and returns the remainder. It
 * always succeeds. If the given base is 0, it returns 0.
 */
int bint_digit_push(struct bint *b, unsigned int base, unsigned int digit);
unsigned int bint_digit_pop(struct bint *b, unsigned int base);

/* These two functions comprise the chunk allocator. By default, malloc() and
 * free() are used. If you want to provide your own allocator, these symbols
 * should be overridden.
 *
 * bint_chunk_alloc() should return a block of memory of
 * sizeof(struct bint_chunk), or NULL if allocation fails.
 */
struct bint_chunk *bint_chunk_alloc(void);
void bint_chunk_free(struct bint_chunk *c);

#endif
