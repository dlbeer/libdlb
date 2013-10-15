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
#include "ioq.h"
#include "containers.h"

void ioq_notify(struct ioq *q)
{
	int old_state;

	thr_mutex_lock(&q->lock);
	old_state = q->intr_state;
	q->intr_state = 1;
	thr_mutex_unlock(&q->lock);

	if (!old_state)
		write(q->intr[1], "", 1);
}

static void intr_ack(struct ioq *q)
{
	char discard[128];

	while (read(q->intr[0], discard, sizeof(discard)) >= 0);

	thr_mutex_lock(&q->lock);
	q->intr_state = 0;
	thr_mutex_unlock(&q->lock);
}

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
	struct epoll_event evt;
	syserr_t err;

	if (runq_init(&q->run, bg_threads) < 0) {
		err = syserr_last();
		goto fail_runq;
	}

	if (!bg_threads)
		q->run.wakeup = wakeup_runq;

	waitq_init(&q->wait, &q->run);
	q->wait.wakeup = wakeup_waitq;

	thr_mutex_init(&q->lock);
	slist_init(&q->mod_list);

	if (pipe(q->intr) < 0) {
		err = syserr_last();
		goto fail_pipe;
	}

	fcntl(q->intr[0], F_SETFL, fcntl(q->intr[0], F_GETFL) | O_NONBLOCK);

	q->intr_state = 0;

	q->epoll_fd = epoll_create(64);
	if (q->epoll_fd < 0) {
		err = syserr_last();
		goto fail_epoll;
	}

	memset(&evt, 0, sizeof(evt));
	evt.events = EPOLLIN;
	if (epoll_ctl(q->epoll_fd, EPOLL_CTL_ADD, q->intr[0], &evt) < 0) {
		err = syserr_last();
		goto fail_ctl;
	}

	return 0;

fail_ctl:
	close(q->epoll_fd);
fail_epoll:
	close(q->intr[0]);
	close(q->intr[1]);
fail_pipe:
	thr_mutex_destroy(&q->lock);
	waitq_destroy(&q->wait);
	runq_destroy(&q->run);
fail_runq:
	syserr_set(err);
	return -1;
}

void ioq_destroy(struct ioq *q)
{
	runq_destroy(&q->run);
	waitq_destroy(&q->wait);

	thr_mutex_destroy(&q->lock);

	close(q->intr[0]);
	close(q->intr[1]);
	close(q->epoll_fd);
}

static int mod_enqueue_nolock(struct ioq *q, struct ioq_fd *f)
{
	int need_wakeup = 0;

	if (!(f->flags & IOQ_FLAG_MOD_LIST)) {
		need_wakeup = slist_is_empty(&q->mod_list);
		f->flags |= IOQ_FLAG_MOD_LIST;
		slist_append(&q->mod_list, &f->mod_list);
	}

	return need_wakeup;
}

static struct ioq_fd *mod_dequeue(struct ioq *q, int *flags,
				  ioq_fd_mask_t *requested)
{
	struct slist_node *n;
	struct ioq_fd *r = NULL;

	thr_mutex_lock(&q->lock);
	n = slist_pop(&q->mod_list);
	if (n) {
		r = container_of(n, struct ioq_fd, mod_list);
		r->flags &= ~IOQ_FLAG_MOD_LIST;
		*flags = r->flags;
		*requested = r->requested;
	}
	thr_mutex_unlock(&q->lock);

	return r;
}

static int do_wait(struct ioq *q)
{
	struct epoll_event evts[32];
	const int timeout = waitq_next_deadline(&q->wait);
	int ret;
	int i;

	/* This can't fail for any reason but signal interruption */
	ret = epoll_wait(q->epoll_fd, evts, lengthof(evts), timeout);
	if (ret < 0) {
		if (syserr_last() == EINTR)
			return 0;
		return -1;
	}

	intr_ack(q);

	for (i = 0; i < ret; i++) {
		const struct epoll_event *e = &evts[i];
		struct ioq_fd *f = e->data.ptr;

		if (!f)
			continue;

		epoll_ctl(q->epoll_fd, EPOLL_CTL_DEL, f->fd, NULL);
		f->ready = e->events;

		thr_mutex_lock(&q->lock);
		f->flags &= ~(IOQ_FLAG_EPOLL | IOQ_FLAG_WAITING);
		mod_enqueue_nolock(q, f);
		thr_mutex_unlock(&q->lock);
	}

	return 0;
}

static void dispatch_mods(struct ioq *q)
{
	for (;;) {
		ioq_fd_mask_t requested;
		int flags;
		struct ioq_fd *f = mod_dequeue(q, &flags, &requested);

		if (!f)
			break;

		if (!(flags & IOQ_FLAG_WAITING)) {
			runq_task_exec(&f->task, f->task.func);
		} else if (!requested) {
			if (flags & IOQ_FLAG_EPOLL)
				epoll_ctl(q->epoll_fd, EPOLL_CTL_DEL,
					  f->fd, NULL);

			f->ready = 0;

			thr_mutex_lock(&q->lock);
			f->flags &= ~(IOQ_FLAG_EPOLL | IOQ_FLAG_WAITING);
			mod_enqueue_nolock(q, f);
			thr_mutex_unlock(&q->lock);
		} else {
			struct epoll_event evt;

			memset(&evt, 0, sizeof(evt));
			evt.events = requested;
			evt.data.ptr = f;

			if (epoll_ctl(q->epoll_fd,
				(flags & IOQ_FLAG_EPOLL) ?
					EPOLL_CTL_MOD : EPOLL_CTL_ADD,
				    f->fd, &evt) < 0) {
				f->err = syserr_last();

				thr_mutex_lock(&q->lock);
				f->requested = 0;
				mod_enqueue_nolock(q, f);
				thr_mutex_unlock(&q->lock);
			} else {
				thr_mutex_lock(&q->lock);
				f->flags |= IOQ_FLAG_EPOLL;
				thr_mutex_unlock(&q->lock);
			}
		}
	}
}

int ioq_iterate(struct ioq *q)
{
	if (do_wait(q) < 0)
		return -1;

	dispatch_mods(q);
	waitq_dispatch(&q->wait, 0);
	runq_dispatch(&q->run, 0);

	return 0;
}

void ioq_fd_init(struct ioq_fd *f, struct ioq *q, int fd)
{
	runq_task_init(&f->task, ioq_runq(q));
	f->owner = q;
	f->fd = fd;

	f->ready = 0;
	f->err = SYSERR_NONE;

	f->flags = 0;
	f->requested = 0;
}

void ioq_fd_wait(struct ioq_fd *f, ioq_fd_mask_t set, ioq_fd_func_t func)
{
	struct ioq *q = f->owner;
	int need_wakeup = 0;

	f->task.func = (runq_task_func_t)func;
	f->requested = set;
	f->ready = 0;
	f->err = 0;
	f->flags = IOQ_FLAG_WAITING;

	if (!set) {
		runq_task_exec(&f->task, (runq_task_func_t)func);
		return;
	}

	thr_mutex_lock(&q->lock);
	need_wakeup = mod_enqueue_nolock(q, f);
	thr_mutex_unlock(&q->lock);

	if (need_wakeup)
		ioq_notify(q);
}

void ioq_fd_rewait(struct ioq_fd *f, ioq_fd_mask_t set)
{
	struct ioq *q = f->owner;
	int need_wakeup = 0;

	thr_mutex_lock(&q->lock);
	if (f->flags & IOQ_FLAG_WAITING) {
		f->requested = set;
		need_wakeup = mod_enqueue_nolock(q, f);
	}
	thr_mutex_unlock(&q->lock);

	if (need_wakeup)
		ioq_notify(q);
}
