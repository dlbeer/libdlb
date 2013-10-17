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

#include "containers.h"
#include "asock.h"

void asock_init(struct asock *t, struct ioq *q)
{
	memset(t, 0, sizeof(*t));
	t->ioq = q;
	t->sock = NET_SOCK_INVALID;
	t->ca_accept_sock = NET_SOCK_INVALID;

	ioq_ovl_init(&t->ca_ovl, q);
	ioq_ovl_init(&t->send_ovl, q);
	ioq_ovl_init(&t->recv_ovl, q);
}

void asock_destroy(struct asock *t)
{
	if (net_sock_is_valid(t->sock))
		closesocket(t->sock);
}

void asock_close(struct asock *t)
{
	if (net_sock_is_valid(t->sock)) {
		shutdown(t->sock, SD_BOTH);
		closesocket(t->sock);
		t->sock = NET_SOCK_INVALID;
	}
}

int asock_listen(struct asock *t, const struct sockaddr *sa,
		 size_t sa_size)
{
	if (net_sock_is_valid(t->sock))
		closesocket(t->sock);

	t->sock = WSASocket(AF_INET, SOCK_STREAM, 0,
			    NULL, 0, WSA_FLAG_OVERLAPPED);
	if (!net_sock_is_valid(t->sock)) {
		t->ca_error = neterr_last();
		return -1;
	}

	if (ioq_bind(t->ioq, (HANDLE)t->sock) < 0) {
		t->ca_error = syserr_last();
		return -1;
	}

	if (bind(t->sock, sa, sa_size) < 0) {
		t->ca_error = neterr_last();
		return -1;
	}

	if (listen(t->sock, SOMAXCONN) < 0) {
		t->ca_error = neterr_last();
		return -1;
	}

	return 0;
}

static void accept_done(struct ioq_ovl *o)
{
	struct asock *t = container_of(o, struct asock, ca_ovl);
	DWORD count;
	DWORD flags;

	if (t->ca_error == WSA_IO_PENDING) {
		if (WSAGetOverlappedResult(t->ca_sock,
			    ioq_ovl_lpo(&t->ca_ovl),
			    &count, FALSE, &flags))
			t->ca_error = 0;
		else
			t->ca_error = neterr_last();
	}

	if (t->ca_error) {
		if (net_sock_is_valid(t->ca_accept_sock))
			closesocket(t->ca_accept_sock);
	} else {
		t->ca_client->sock = t->ca_accept_sock;
	}

	t->ca_accept_sock = NET_SOCK_INVALID;
	t->ca_func(t);
}

void asock_accept(struct asock *t, struct asock *client,
		  asock_func_t func)
{
	DWORD count;

	t->ca_func = func;
	t->ca_sock = t->sock;
	t->ca_error = WSA_IO_PENDING;
	t->ca_client = client;

	ioq_ovl_wait(&t->ca_ovl, accept_done);

	t->ca_accept_sock = WSASocket(AF_INET, SOCK_STREAM, 0,
				      NULL, 0, WSA_FLAG_OVERLAPPED);
	if (!net_sock_is_valid(t->ca_accept_sock)) {
		t->ca_error = neterr_last();
		ioq_ovl_trigger(&t->ca_ovl);
		return;
	}

	if (ioq_bind(t->ioq, (HANDLE)t->ca_accept_sock) < 0) {
		t->ca_error = syserr_last();
		ioq_ovl_trigger(&t->ca_ovl);
		return;
	}

	if (!net_AcceptEx(t->sock, t->ca_accept_sock, t->ca_addr_info, 0,
		     sizeof(t->ca_addr_info) / 2,
		     sizeof(t->ca_addr_info) / 2,
		     &count,
		     ioq_ovl_lpo(&t->ca_ovl))) {
		const neterr_t e = WSAGetLastError();

		if (e != WSA_IO_PENDING) {
			t->ca_error = e;
			ioq_ovl_trigger(&t->ca_ovl);
		}
	}
}

static void connect_done(struct ioq_ovl *o)
{
	struct asock *t = container_of(o, struct asock, ca_ovl);
	DWORD count;
	DWORD flags;

	if (t->ca_error == WSA_IO_PENDING) {
		if (WSAGetOverlappedResult(t->ca_sock,
			    ioq_ovl_lpo(&t->ca_ovl),
			    &count, FALSE, &flags))
			t->ca_error = 0;
		else
			t->ca_error = neterr_last();
	}

	t->ca_func(t);
}

void asock_connect(struct asock *t, const struct sockaddr *sa,
		   size_t sa_size, asock_func_t func)
{
	DWORD count;
	struct sockaddr_in local;

	if (net_sock_is_valid(t->sock))
		closesocket(t->sock);

	t->sock = WSASocket(AF_INET, SOCK_STREAM, 0,
			    NULL, 0, WSA_FLAG_OVERLAPPED);
	if (!net_sock_is_valid(t->sock)) {
		t->ca_error = neterr_last();
		ioq_ovl_trigger(&t->ca_ovl);
		return;
	}

	if (ioq_bind(t->ioq, (HANDLE)t->sock) < 0) {
		t->ca_error = syserr_last();
		ioq_ovl_trigger(&t->ca_ovl);
		return;
	}

	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = 0;
	if (bind(t->sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
		t->ca_error = neterr_last();
		ioq_ovl_trigger(&t->ca_ovl);
		return;
	}

	t->ca_sock = t->sock;
	t->ca_func = func;
	t->ca_error = WSA_IO_PENDING;
	ioq_ovl_wait(&t->ca_ovl, connect_done);

	if (!net_ConnectEx(t->sock, sa, sa_size, NULL, 0,
		      &count, ioq_ovl_lpo(&t->ca_ovl))) {
		const neterr_t e = WSAGetLastError();

		if (e != WSA_IO_PENDING) {
			t->ca_error = e;
			ioq_ovl_trigger(&t->ca_ovl);
		}
	}
}

static void send_done(struct ioq_ovl *o)
{
	struct asock *t = container_of(o, struct asock, send_ovl);
	DWORD flags;

	if (t->send_error == WSA_IO_PENDING) {
		if (WSAGetOverlappedResult(t->send_sock,
			    ioq_ovl_lpo(&t->send_ovl),
			    &t->send_size, FALSE, &flags))
			t->send_error = 0;
		else
			t->send_error = neterr_last();
	}

	if (t->send_error)
		t->send_size = 0;

	t->send_func(t);
}

void asock_send(struct asock *t, const uint8_t *data, size_t len,
		asock_func_t func)
{
	t->send_func = func;
	t->send_sock = t->sock;
	t->send_error = WSA_IO_PENDING;
	t->send_buf.len = len;
	t->send_buf.buf = (char *)data;

	ioq_ovl_wait(&t->send_ovl, send_done);

	if (WSASend(t->sock, &t->send_buf, 1, NULL, 0,
		    ioq_ovl_lpo(&t->send_ovl), NULL)) {
		const neterr_t e = WSAGetLastError();

		if (e != WSA_IO_PENDING) {
			t->send_error = e;
			ioq_ovl_trigger(&t->send_ovl);
		}
	}
}

static void recv_done(struct ioq_ovl *o)
{
	struct asock *t = container_of(o, struct asock, recv_ovl);
	DWORD flags;

	if (t->recv_error == WSA_IO_PENDING) {
		if (WSAGetOverlappedResult(t->recv_sock,
			    ioq_ovl_lpo(&t->recv_ovl),
			    &t->recv_size, FALSE, &flags))
			t->recv_error = 0;
		else
			t->recv_error = neterr_last();
	}

	if (t->recv_error)
		t->recv_size = 0;

	t->recv_func(t);
}

void asock_recv(struct asock *t, uint8_t *data, size_t max_size,
		asock_func_t func)
{
	DWORD flags = 0;

	t->recv_func = func;
	t->recv_sock = t->sock;
	t->recv_error = WSA_IO_PENDING;
	t->recv_buf.len = max_size;
	t->recv_buf.buf = (char *)data;

	ioq_ovl_wait(&t->recv_ovl, recv_done);

	if (WSARecv(t->sock, &t->recv_buf, 1, NULL, &flags,
		     ioq_ovl_lpo(&t->recv_ovl), NULL)) {
		const neterr_t e = WSAGetLastError();

		if (e != WSA_IO_PENDING) {
			t->recv_error = e;
			ioq_ovl_trigger(&t->recv_ovl);
		}
	}
}
