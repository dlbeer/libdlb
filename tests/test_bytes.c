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
#include "bytes.h"

#define WRITE_TEST(format, value) do { \
    bytes_w##format(buffer, value); \
    assert(bytes_r##format(buffer) == value); \
} while (0);

int main(void)
{
	uint8_t buffer[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	assert(bytes_r16le(buffer) == 0x0201);
	assert(bytes_r16net(buffer) == 0x0102);
	assert(bytes_r32le(buffer) == 0x04030201);
	assert(bytes_r32net(buffer) == 0x01020304);
	assert(bytes_r64le(buffer) == 0x0807060504030201LL);
	assert(bytes_r64net(buffer) == 0x0102030405060708LL);

	WRITE_TEST(16le, 0xabcd);
	WRITE_TEST(16net, 0xabcd);
	WRITE_TEST(32le, 0xdeadbeef);
	WRITE_TEST(32net, 0xdeadbeef);
	WRITE_TEST(64le, 0x2468abcd789a0101LL);
	WRITE_TEST(64net, 0x2468abcd789a0101LL);

	return 0;
}
