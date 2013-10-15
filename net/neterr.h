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

#ifndef NET_NETERR_H_
#define NET_NETERR_H_

/* Network stack error codes */
#ifdef __Windows__
#include <windows.h>
#include <winsock2.h>

typedef DWORD neterr_t;

static inline neterr_t neterr_last(struct neterr *e)
{
	return WSAGetLastError();
}

static inline void neterr_format(neterr_t e, char *buf, size_t max_size)
{
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, e, 0,
		      buf, max_size, NULL);
}
#else
#include <errno.h>
#include <string.h>

typedef int neterr_t;

static inline neterr_t neterr_last(void)
{
	return errno;
}

static inline void neterr_format(neterr_t e, char *buf, size_t max_size)
{
	strncpy(buf, strerror(e), max_size);
	buf[max_size - 1] = 0;
}
#endif

#define NETERR_NONE ((neterr_t)0)

#endif
