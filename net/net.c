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

#ifdef __Windows__
#include "net.h"

BOOL PASCAL (*net_ConnectEx)(
	SOCKET s,
	const struct sockaddr *name,
	int namelen,
	PVOID lpSendBuffer,
	DWORD dwSendDataLength,
	LPDWORD lpdwBytesSent,
	LPOVERLAPPED lpOverlapped
);

BOOL (*net_AcceptEx)(
	SOCKET sListenSocket,
	SOCKET sAcceptSocket,
	PVOID lpOutputBuffer,
	DWORD dwReceiveDataLength,
	DWORD dwLocalAddressLength,
	DWORD dwRemoteAddressLength,
	LPDWORD lpdwBytesReceived,
	LPOVERLAPPED lpOverlapped
);

static GUID id_AcceptEx = {
	0xb5367df1, 0xcbac, 0x11cf,
	{0x95, 0xca, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}
};

static GUID id_ConnectEx = {
	0x25a207b9, 0xddf3, 0x4660,
	{0x8e, 0xe9, 0x76, 0xe5, 0x8c, 0x74, 0x06, 0x3e}
};

neterr_t net_start(void)
{
	WSADATA data;
	DWORD count;
	neterr_t err;
	SOCKET s;

	err = WSAStartup(MAKEWORD(2, 2), &data);
	if (err)
		return err;

	s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
	if (s == INVALID_SOCKET) {
		neterr_t e = neterr_last();

		WSACleanup();
		return e;
	}

	if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
		     &id_AcceptEx, sizeof(id_AcceptEx),
		     &net_AcceptEx, sizeof(net_AcceptEx),
		     &count, NULL, NULL) ||
	    WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
		     &id_ConnectEx, sizeof(id_ConnectEx),
		     &net_ConnectEx, sizeof(net_ConnectEx),
		     &count, NULL, NULL)) {
		neterr_t e = neterr_last();

		closesocket(s);
		WSACleanup();
		return e;
	}

	closesocket(s);
	return 0;
}
#endif
