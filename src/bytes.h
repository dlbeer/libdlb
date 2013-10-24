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

#ifndef BYTES_H_
#define BYTES_H_

#include <stdint.h>

/* Utility functions for dealing with endian-specific data of unknown
 * alignment.
 */
static inline uint16_t bytes_r16le(const uint8_t *data)
{
	return ((uint16_t)data[0]) | (((uint16_t)data[1]) << 8);
}

static inline uint16_t bytes_r16net(const uint8_t *data)
{
	return ((uint16_t)data[1]) | (((uint16_t)data[0]) << 8);
}

static inline void bytes_w16le(uint8_t *data, uint16_t v)
{
	data[0] = v;
	data[1] = v >> 8;
}

static inline void bytes_w16net(uint8_t *data, uint16_t v)
{
	data[1] = v;
	data[0] = v >> 8;
}

static inline uint32_t bytes_r32le(const uint8_t *data)
{
	return ((uint32_t)data[0]) |
	       (((uint32_t)data[1]) << 8) |
	       (((uint32_t)data[2]) << 16) |
	       (((uint32_t)data[3]) << 24);
}

static inline uint32_t bytes_r32net(const uint8_t *data)
{
	return ((uint32_t)data[3]) |
	       (((uint32_t)data[2]) << 8) |
	       (((uint32_t)data[1]) << 16) |
	       (((uint32_t)data[0]) << 24);
}

static inline void bytes_w32le(uint8_t *data, uint32_t v)
{
	data[0] = v;
	data[1] = v >> 8;
	data[2] = v >> 16;
	data[3] = v >> 24;
}

static inline void bytes_w32net(uint8_t *data, uint32_t v)
{
	data[3] = v;
	data[2] = v >> 8;
	data[1] = v >> 16;
	data[0] = v >> 24;
}

static inline uint64_t bytes_r64le(const uint8_t *data)
{
	return ((uint64_t)data[0]) |
	       (((uint64_t)data[1]) << 8) |
	       (((uint64_t)data[2]) << 16) |
	       (((uint64_t)data[3]) << 24) |
	       (((uint64_t)data[4]) << 32) |
	       (((uint64_t)data[5]) << 40) |
	       (((uint64_t)data[6]) << 48) |
	       (((uint64_t)data[7]) << 56);
}

static inline uint64_t bytes_r64net(const uint8_t *data)
{
	return ((uint64_t)data[7]) |
	       (((uint64_t)data[6]) << 8) |
	       (((uint64_t)data[5]) << 16) |
	       (((uint64_t)data[4]) << 24) |
	       (((uint64_t)data[3]) << 32) |
	       (((uint64_t)data[2]) << 40) |
	       (((uint64_t)data[1]) << 48) |
	       (((uint64_t)data[0]) << 56);
}

static inline void bytes_w64le(uint8_t *data, uint64_t v)
{
	data[0] = v;
	data[1] = v >> 8;
	data[2] = v >> 16;
	data[3] = v >> 24;
	data[4] = v >> 32;
	data[5] = v >> 40;
	data[6] = v >> 48;
	data[7] = v >> 56;
}

static inline void bytes_w64net(uint8_t *data, uint64_t v)
{
	data[7] = v;
	data[6] = v >> 8;
	data[5] = v >> 16;
	data[4] = v >> 24;
	data[3] = v >> 32;
	data[2] = v >> 40;
	data[1] = v >> 48;
	data[0] = v >> 56;
}

#endif
