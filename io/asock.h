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

#ifndef IO_ASOCK_H_
#define IO_ASOCK_H_

#include "net.h"
#include "ioq.h"
#include "handle.h"

/* Asynchronous socket */
struct asock;
typedef void (*asock_func_t)(struct asock *t);

#ifdef __Windows__
struct asock {
	struct ioq		*ioq;
	net_sock_t		sock;

	/* Connect/accept request */
	asock_func_t		ca_func;
	net_sock_t		ca_sock;
	neterr_t		ca_error;
	struct ioq_ovl		ca_ovl;
	struct asock		*ca_client;
	uint8_t			ca_addr_info[64];
	net_sock_t		ca_accept_sock;

	/* Send request */
	asock_func_t		send_func;
	net_sock_t		send_sock;
	DWORD			send_size;
	neterr_t		send_error;
	struct ioq_ovl		send_ovl;
	WSABUF			send_buf;

	/* Receive request */
	asock_func_t		recv_func;
	net_sock_t		recv_sock;
	DWORD			recv_size;
	neterr_t		recv_error;
	struct ioq_ovl		recv_ovl;
	WSABUF			recv_buf;
};
#else
struct asock {
	struct ioq		*ioq;
	net_sock_t		sock;

	/* Connect/accept request */
	asock_func_t		ca_func;
	neterr_t		ca_error;
	const struct sockaddr	*ca_addr;
	size_t			ca_size;
	struct asock		*ca_client;

	/* Send request */
	asock_func_t		send_func;
	const uint8_t		*send_data;
	size_t			send_size;
	neterr_t		send_error;

	/* Receive request */
	asock_func_t		recv_func;
	uint8_t			*recv_data;
	size_t			recv_size;
	neterr_t		recv_error;

	/* Event wait process */
	thr_mutex_t		wait_lock;
	struct ioq_fd		wait_fd;
	int			wait_ops;

	/* Dispatcher */
	thr_mutex_t		dispatch_lock;
	struct runq_task	dispatch_task;
	int			dispatch_queue;
};
#endif

/* Initialize a socket. This socket is created in an inactive state --
 * there is no system resource allocated until you either listen or
 * connect.
 */
void asock_init(struct asock *t, struct ioq *q);

/* Destroy a socket. */
void asock_destroy(struct asock *t);

/* Retrieve the last error from a network operation */
static inline neterr_t asock_get_error(const struct asock *t)
{
	return t->ca_error;
}

/* Obtain the operating system handle for this socket */
static inline net_sock_t asock_get_handle(const struct asock *t)
{
	return t->sock;
}

/* Close the socket. This does not destroy the object, but it does make
 * it inactive. All outstanding operations are canceled, and will
 * complete very soon.
 */
void asock_close(struct asock *t);

/* Begin listening on the given socket address. If this fails, -1 is
 * returned, and the last error is available via asock_get_error().
 */
int asock_listen(struct asock *t, const struct sockaddr *sa,
		 size_t sa_size);

/* Wait for, and accept an incoming socket. The given client pointer
 * must point to an already-initialized (but inactive) socket. If
 * successful, there will be no error reported by asock_get_error().
 *
 * You may not simultaneously have outstanding accept and connect
 * operations.
 *
 * If this operation is canceled, there is no change to the client
 * socket.
 */
void asock_accept(struct asock *t, struct asock *client,
		  asock_func_t func);

/* Connect to a server. The socket will report no error if
 * successful.
 *
 * You may not simultaneously have outstanding accept and connect
 * operations.
 *
 * The sockaddr supplied must remain valid until the operation
 * completes.
 */
void asock_connect(struct asock *t, const struct sockaddr *sa,
		   size_t sa_size, asock_func_t func);

/* Send data on a connected socket. This operation has its own separate
 * result/error codes, and can be performed simultaneously with other
 * operations.
 *
 * It may be the case that not all of the supplied data is sent.
 */
void asock_send(struct asock *t, const uint8_t *data, size_t len,
		asock_func_t func);

static inline neterr_t asock_get_send_error(const struct asock *t)
{
	return t->send_error;
}

static inline size_t asock_get_send_size(const struct asock *t)
{
	return t->send_size;
}

/* Send data on a connected socket. This operation has its own separate
 * result/error codes, and can be performed simultaneously with other
 * operations.
 *
 * A read on a gracefully closed socket returns 0 bytes, with no error
 * code.
 */
void asock_recv(struct asock *t, uint8_t *data, size_t max_size,
		asock_func_t func);

static inline neterr_t asock_get_recv_error(const struct asock *t)
{
	return t->recv_error;
}

static inline size_t asock_get_recv_size(const struct asock *t)
{
	return t->recv_size;
}

#endif
