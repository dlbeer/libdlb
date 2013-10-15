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

struct neterr {
	DWORD		sys;
};

typedef DWORD neterr_t;

static inline void neterr_get(struct neterr *e)
{
	e->sys = WSAGetLastError();
}

static inline void neterr_get_h(struct neterr *e)
{
	e->sys = WSAGetLastError();
}

static inline void neterr_format(const struct neterr *e,
				 char *buf, size_t max_size)
{
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, e->sys, 0,
		      buf, max_size, NULL);
}
#else
#include <errno.h>
#include <netdb.h>

struct neterr {
	int		sys;
	int		herr;
};

static inline void neterr_get(struct neterr *e)
{
	e->sys = errno;
	e->herr = 0;
}

static inline void neterr_get_h(struct neterr *e)
{
	e->sys = 1;
	e->herr = h_errno;
}

void neterr_format(const struct neterr *e, char *buf, size_t max_size);
#endif

static inline void neterr_clear(struct neterr *e)
{
	e->sys = 0;
}

static inline int neterr_is_ok(const struct neterr *e)
{
	return !e->sys;
}

#endif
