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

#ifndef IO_HANDLE_H_
#define IO_HANDLE_H_

/* File handle abstraction. */
#ifdef __Windows__

#include "winapi.h"

typedef HANDLE handle_t;

#define HANDLE_NONE		NULL

static inline int handle_is_valid(handle_t h)
{
	return (h != NULL) && (h != INVALID_HANDLE_VALUE);
}

static inline void handle_close(handle_t h)
{
	CloseHandle(h);
}

#else

#include <unistd.h>

typedef int handle_t;

#define HANDLE_NONE		((handle_t)(-1))

static inline int handle_is_valid(handle_t h)
{
	return h >= 0;
}

static inline void handle_close(handle_t h)
{
	close(h);
}

#endif

#endif
