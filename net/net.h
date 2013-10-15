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

#ifndef NET_NET_H_
#define NET_NET_H_

#include "neterr.h"

/* Start up/shut down the network stack. This pair of functions must be
 * called at startup before any network functions are used, and at exit
 * to clean up.
 *
 * net_start() returns an error code (NETERR_NONE if successful).
 */
#ifdef __Windows__
static inline neterr_t net_start(void)
{
	WSADATA data;

	return WSAStartup(MAKEWORD(2, 2), &data);
}

void net_stop(void)
{
	WSACleanup();
}
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static inline neterr_t net_start(void)
{
	return NETERR_NONE;
}

static inline void net_stop(void) { }
#endif

#endif
