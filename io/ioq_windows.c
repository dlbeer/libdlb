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

#include "ioq.h"
#include "containers.h"

static void wakeup_runq(struct runq *q)
{
	ioq_notify(container_of(q, struct ioq, run));
}

static void wakeup_waitq(struct waitq *q)
{
	ioq_notify(container_of(q, struct ioq, wait));
}

int ioq_init(struct ioq *q, unsigned int bg_threads)
{
	if (runq_init(&q->run, bg_threads) < 0)
		return -1;
	waitq_init(&q->wait, &q->run);

	q->run.wakeup = wakeup_runq;
	q->wait.wakeup = wakeup_waitq;

	q->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!q->iocp) {
		syserr_t err = syserr_last();

		waitq_destroy(&q->wait);
		runq_destroy(&q->run);
		syserr_set(err);
		return -1;
	}

	thr_mutex_init(&q->lock);
	return 0;
}

void ioq_destroy(struct ioq *q)
{
	thr_mutex_destroy(&q->lock);
	CloseHandle(q->iocp);
	waitq_destroy(&q->wait);
	runq_destroy(&q->run);
}

int ioq_iterate(struct ioq *q)
{
	const int timeout = waitq_next_deadline(&q->wait);
	DWORD number_of_bytes;
	ULONG_PTR comp_key;
	LPOVERLAPPED overlapped;

	if (GetQueuedCompletionStatus(q->iocp, &number_of_bytes, &comp_key,
		    &overlapped, (timeout < 0) ? INFINITE : timeout)) {
		struct ioq_ovl *h = container_of(overlapped,
			struct ioq_ovl, overlapped);

		if (overlapped)
			runq_task_exec(&h->task, h->task.func);
	} else if (GetLastError() != WAIT_TIMEOUT) {
		return -1;
	}

	thr_mutex_lock(&q->lock);
	q->notify_status = 0;
	thr_mutex_unlock(&q->lock);

	waitq_dispatch(&q->wait, 0);
	runq_dispatch(&q->run, 0);
	return 0;
}

void ioq_notify(struct ioq *q)
{
	int old;

	thr_mutex_lock(&q->lock);
	old = q->notify_status;
	q->notify_status = 1;
	thr_mutex_unlock(&q->lock);

	if (!old)
		PostQueuedCompletionStatus(q->iocp, 0, 0, NULL);
}

void ioq_ovl_wait(struct ioq_ovl *h, ioq_ovl_func_t f)
{
	memset(&h->overlapped, 0, sizeof(h->overlapped));
	h->task.func = (runq_task_func_t)f;
}
