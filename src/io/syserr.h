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

#ifndef IO_SYSERR_H_
#define IO_SYSERR_H_

#include <string.h>

/* System error type */
#ifdef __Windows__
#include <windows.h>

typedef DWORD syserr_t;

static inline syserr_t syserr_last(void)
{
	return GetLastError();
}

static inline void syserr_format(syserr_t err, char *buf, size_t max_size)
{
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0,
		      buf, max_size, NULL);
}
#else
#include <errno.h>

typedef int syserr_t;

static inline syserr_t syserr_last(void)
{
	return errno;
}

static inline void syserr_format(syserr_t err, char *buf, size_t max_size)
{
	strncpy(buf, strerror(err), max_size);
	buf[max_size - 1] = 0;
}
#endif

#define SYSERR_NONE ((syserr_t)0)

#endif
