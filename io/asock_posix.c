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

#include <unistd.h>
#include <fcntl.h>
#include "asock.h"
#include "containers.h"

#define OP_CONNECT	0x01
#define OP_ACCEPT	0x02
#define OP_SEND		0x04
#define OP_RECV		0x08
#define OP_CANCEL	0x10

/************************************************************************
 * Dispatcher
 */

static void dispatch_func(struct runq_task *task)
{
	struct asock *t = container_of(task, struct asock, dispatch_task);
	int ops;

	thr_mutex_lock(&t->dispatch_lock);
	ops = t->dispatch_queue;
	t->dispatch_queue = 0;
	thr_mutex_unlock(&t->dispatch_lock);

	if (ops & (OP_CONNECT | OP_ACCEPT))
		t->ca_func(t);
	if (ops & OP_SEND)
		t->send_func(t);
	if (ops & OP_RECV)
		t->recv_func(t);
}

static void dispatch_push(struct asock *t, int ops)
{
	int old_queue;

	thr_mutex_lock(&t->dispatch_lock);
	old_queue = t->dispatch_queue;
	t->dispatch_queue |= ops;
	thr_mutex_unlock(&t->dispatch_lock);

	if (!old_queue && ops)
		runq_task_exec(&t->dispatch_task, dispatch_func);
}

/************************************************************************
 * Wait process
 */

static void wait_init(struct asock *t);

static ioq_fd_mask_t wait_mask(int ops)
{
	ioq_fd_mask_t m = 0;

	if (ops & (OP_CONNECT | OP_SEND))
		m |= IOQ_EVENT_OUT;

	if (ops & (OP_CONNECT | OP_ACCEPT | OP_RECV))
		m |= IOQ_EVENT_IN | IOQ_EVENT_ERR | IOQ_EVENT_HUP;

	return m;
}

static int wait_connect(struct asock *t)
{
	int r = connect(t->wait_fd.fd, t->ca_addr, t->ca_size);

	if (r < 0) {
		if (errno == EAGAIN)
			return 0;

		t->ca_error = errno;
	} else {
		t->ca_error = 0;
	}

	return OP_CONNECT;
}

static int wait_accept(struct asock *t)
{
	struct sockaddr_in in;
	socklen_t len = sizeof(in);
	int r;

	if (!(ioq_fd_ready(&t->wait_fd) &
		(IOQ_EVENT_IN | IOQ_EVENT_OUT | IOQ_EVENT_ERR)))
		return 0;

	r = accept(t->wait_fd.fd, (struct sockaddr *)&in, &len);
	if (r < 0) {
		if (errno == EAGAIN)
			return 0;

		t->ca_error = errno;
	} else {
		if (t->ca_client->sock >= 0)
			close(t->ca_client->sock);

		t->ca_client->sock = r;
		t->ca_error = 0;
		wait_init(t->ca_client);
	}

	return OP_ACCEPT;
}

static int wait_send(struct asock *t)
{
	int r;

	if (!(ioq_fd_ready(&t->wait_fd) & (IOQ_EVENT_OUT | IOQ_EVENT_ERR)))
		return 0;

	r = send(t->wait_fd.fd, t->send_data, t->send_size, 0);
	if (r < 0) {
		if (errno == EAGAIN)
			return 0;

		t->send_error = errno;
		t->send_size = 0;
	} else {
		t->send_size = r;
		t->send_error = 0;
	}

	return OP_SEND;
}

static int wait_recv(struct asock *t)
{
	int r;

	if (ioq_fd_ready(&t->wait_fd) & IOQ_EVENT_HUP) {
		t->recv_size = 0;
		t->recv_error = 0;
		return OP_RECV;
	}

	if (!(ioq_fd_ready(&t->wait_fd) & (IOQ_EVENT_IN | IOQ_EVENT_HUP)))
		return 0;

	r = recv(t->wait_fd.fd, t->recv_data, t->recv_size, 0);
	if (r < 0) {
		if (errno == EAGAIN)
			return 0;

		t->recv_error = errno;
		t->recv_size = 0;
	} else {
		t->recv_size = r;
		t->recv_error = 0;
	}

	return OP_RECV;
}

static void wait_end(struct ioq_fd *f)
{
	struct asock *t = container_of(f, struct asock, wait_fd);
	int dispatch_mask = 0;

	thr_mutex_lock(&t->wait_lock);
	if ((t->wait_ops & OP_CANCEL) || ioq_fd_error(f)) {
		const int e = ioq_fd_error(f);

		if (t->wait_ops & (OP_CONNECT | OP_ACCEPT))
			t->ca_error = e;

		if (t->wait_ops & OP_SEND) {
			t->send_size = 0;
			t->send_error = e;
		}

		if (t->wait_ops & OP_RECV) {
			t->recv_size = 0;
			t->recv_error = e;
		}

		if (t->wait_ops & OP_CANCEL)
			close(t->wait_fd.fd);

		dispatch_mask = t->wait_ops;
		t->wait_ops = 0;
	} else {
		if (t->wait_ops & OP_CONNECT)
			dispatch_mask |= wait_connect(t);

		if (t->wait_ops & OP_ACCEPT)
			dispatch_mask |= wait_accept(t);

		if (t->wait_ops & OP_SEND)
			dispatch_mask |= wait_send(t);

		if (t->wait_ops & OP_RECV)
			dispatch_mask |= wait_recv(t);

		t->wait_ops &= ~dispatch_mask;
		if (t->wait_ops)
			ioq_fd_wait(&t->wait_fd,
				wait_mask(t->wait_ops), wait_end);
	}
	thr_mutex_unlock(&t->wait_lock);

	if (dispatch_mask)
		dispatch_push(t, dispatch_mask);
}

static int wait_begin(struct asock *t, int mask)
{
	int r;

	thr_mutex_lock(&t->wait_lock);
	r = t->wait_ops;

	if (mask & OP_CANCEL) {
		if (t->wait_ops) {
			t->wait_ops |= OP_CANCEL;
			ioq_fd_cancel(&t->wait_fd);
		}
	} else {
		ioq_fd_mask_t m;

		t->wait_ops |= mask;
		m = wait_mask(t->wait_ops);

		if (r)
			ioq_fd_rewait(&t->wait_fd, m);
		else
			ioq_fd_wait(&t->wait_fd, m, wait_end);
	}
	thr_mutex_unlock(&t->wait_lock);

	return r;
}

static void wait_init(struct asock *t)
{
	thr_mutex_lock(&t->wait_lock);
	ioq_fd_init(&t->wait_fd, t->ioq, t->sock);
	t->wait_ops = 0;
	thr_mutex_unlock(&t->wait_lock);
}

/************************************************************************
 * Public interface
 */

void asock_init(struct asock *t, struct ioq *q)
{
	memset(t, 0, sizeof(*t));

	t->ioq = q;
	t->sock = -1;

	runq_task_init(&t->dispatch_task, ioq_runq(q));
	thr_mutex_init(&t->wait_lock);
	thr_mutex_init(&t->dispatch_lock);
}

void asock_destroy(struct asock *t)
{
	if (t->sock >= 0)
		close(t->sock);

	thr_mutex_destroy(&t->wait_lock);
	thr_mutex_destroy(&t->dispatch_lock);
}

void asock_close(struct asock *t)
{
	if (t->sock < 0)
		return;

	if (!wait_begin(t, OP_CANCEL))
		close(t->sock);

	t->sock = -1;
}

int asock_listen(struct asock *t, const struct sockaddr *sa,
		 size_t sa_len)
{
	int optval = 1;

	if (t->sock >= 0)
		close(t->sock);

	t->sock = socket(PF_INET, SOCK_STREAM, 0);
	if (t->sock < 0) {
		t->ca_error = errno;
		return -1;
	}

	wait_init(t);

	if (setsockopt(t->sock, SOL_SOCKET, SO_REUSEADDR,
			&optval, sizeof(optval)) < 0) {
		t->ca_error = errno;
		return -1;
	}

	if (bind(t->sock, sa, sa_len) < 0) {
		t->ca_error = errno;
		return -1;
	}

	if (listen(t->sock, SOMAXCONN) < 0) {
		t->ca_error = errno;
		return -1;
	}

	return 0;
}

void asock_accept(struct asock *t, struct asock *client,
		  asock_func_t func)
{
	t->ca_client = client;
	t->ca_func = func;

	if (t->sock < 0) {
		t->ca_error = EBADF;
		dispatch_push(t, OP_ACCEPT);
		return;
	}

	wait_begin(t, OP_ACCEPT);
}

void asock_connect(struct asock *t, const struct sockaddr *sa,
		   size_t sa_len, asock_func_t func)
{
	if (t->sock >= 0)
		close(t->sock);

	t->ca_func = func;
	t->ca_addr = sa;
	t->ca_size = sa_len;

	t->sock = socket(PF_INET, SOCK_STREAM, 0);
	if (t->sock < 0) {
		t->ca_error = errno;
		dispatch_push(t, OP_CONNECT);
		return;
	}

	wait_init(t);

	fcntl(t->sock, F_SETFL, fcntl(t->sock, F_GETFL) | O_NONBLOCK);
	connect(t->sock, sa, sa_len);

	if (errno != EINPROGRESS) {
		t->ca_error = errno;
		dispatch_push(t, OP_CONNECT);
		return;
	}

	wait_begin(t, OP_CONNECT);
}

void asock_send(struct asock *t, const uint8_t *data, size_t len,
		asock_func_t func)
{
	t->send_data = data;
	t->send_size = len;
	t->send_func = func;

	if (t->sock < 0) {
		t->send_error = EBADF;
		dispatch_push(t, OP_SEND);
		return;
	}

	wait_begin(t, OP_SEND);
}

void asock_recv(struct asock *t, uint8_t *data, size_t max_len,
		asock_func_t func)
{
	t->recv_data = data;
	t->recv_size = max_len;
	t->recv_func = func;

	if (t->sock < 0) {
		t->recv_error = EBADF;
		dispatch_push(t, OP_RECV);
		return;
	}

	wait_begin(t, OP_RECV);
}
