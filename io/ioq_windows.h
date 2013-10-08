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

#include <windows.h>
#include "syserr.h"

/* DO NOT INCLUDE THIS FILE DIRECTLY */
struct ioq {
	struct runq		run;
	struct waitq		wait;

	/* IO completion port */
	HANDLE			iocp;

	/* Notification status */
	thr_mutex_t		lock;
	int			notify_status;
};

/* If you want to perform IO on a handle and use the IO queue for
 * notification, you first need to bind the handle to the queue's
 * completion port. This operation is irreversible.
 *
 * If you perform IO on a bound handle, you must *first* either wait for
 * it, or set the lower-order bit of the OVERLAPPED structure's hEvent
 * member, to disable IOCP notification. Failure to follow these rules
 * may result in wild-pointer dereference and/or crash.
 */
static inline int ioq_bind(struct ioq *q, HANDLE hnd)
{
	return CreateIoCompletionPort(hnd, q->iocp, 0, 0) ? 0 : -1;
}

/* IO queue handle objects.
 *
 * To perform an asynchronous IO operation, do the following:
 *
 *   0. Bind the HANDLE to the IO queue, if not done already.
 *   1. Wait for the operation (*before* it is started).
 *   2. Start the operation, using the LPOVERLAPPED value returned by
 *      ioq_ovl_lpo().
 *   3. Wait for completion.
 *
 * If step (2) fails, no further action is necessary. Waiting for the
 * operation actually does nothing except fill out the OVERLAPPED
 * structure as necessary for the IO queue.
 *
 * Another option, if the operation fails immediately, is to call
 * ioq_ovl_trigger(), specifying the error code.
 */
struct ioq_ovl;
typedef void (*ioq_ovl_func_t)(struct ioq_ovl *h);

struct ioq_ovl {
	struct runq_task	task;
	HANDLE			hnd;
	OVERLAPPED		overlapped;

	/* Completion status */
	DWORD			number_of_bytes;
	syserr_t		error;
};

void ioq_ovl_init(struct ioq_ovl *h, struct ioq *q, HANDLE hnd);
void ioq_ovl_wait(struct ioq_ovl *h, ioq_ovl_func_t f);
void ioq_ovl_trigger(struct ioq_ovl *h, syserr_t error);

static inline HANDLE ioq_ovl_handle(const struct ioq_ovl *h)
{
	return h->hnd;
}

static inline LPOVERLAPPED ioq_ovl_lpo(struct ioq_ovl *h)
{
	return &h->overlapped;
}

static inline DWORD ioq_ovl_count(const struct ioq_ovl *h)
{
	return h->number_of_bytes;
}

static inline syserr_t ioq_ovl_error(const struct ioq_ovl *h)
{
	return h->error;
}
