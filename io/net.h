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

#ifndef IO_NET_H_
#define IO_NET_H_

#define NETERR_NONE ((neterr_t)0)

#ifdef __Windows__
#include "winapi.h"
#include <winsock2.h>
#include <ws2tcpip.h>

typedef DWORD neterr_t;

static inline neterr_t neterr_last(void)
{
	return WSAGetLastError();
}

static inline void neterr_format(neterr_t e, char *buf, size_t max_size)
{
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, e, 0,
		      buf, max_size, NULL);
}

typedef SOCKET net_sock_t;

static inline int net_sock_is_valid(net_sock_t s)
{
	return s != INVALID_SOCKET;
}

#define NET_SOCK_INVALID INVALID_SOCKET

/* Start up/shut down the network stack. This pair of functions must be
 * called at startup before any network functions are used, and at exit
 * to clean up.
 *
 * net_start() returns an error code (NETERR_NONE if successful).
 */
neterr_t net_start(void);

static inline void net_stop(void)
{
	WSACleanup();
}

/* These pointers are looked up by net_start() */
extern BOOL PASCAL (*net_ConnectEx)(
	SOCKET s,
	const struct sockaddr *name,
	int namelen,
	PVOID lpSendBuffer,
	DWORD dwSendDataLength,
	LPDWORD lpdwBytesSent,
	LPOVERLAPPED lpOverlapped
);

extern BOOL (*net_AcceptEx)(
	SOCKET sListenSocket,
	SOCKET sAcceptSocket,
	PVOID lpOutputBuffer,
	DWORD dwReceiveDataLength,
	DWORD dwLocalAddressLength,
	DWORD dwRemoteAddressLength,
	LPDWORD lpdwBytesReceived,
	LPOVERLAPPED lpOverlapped
);
#else
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

typedef int net_sock_t;

static inline int net_sock_is_valid(net_sock_t s)
{
	return s >= 0;
}

#define NET_SOCK_INVALID ((net_sock_t)(-1))

static inline neterr_t net_start(void)
{
	return NETERR_NONE;
}

static inline void net_stop(void) { }
#endif

#endif
