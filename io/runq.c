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

#include "syserr.h"
#include "runq.h"
#include "containers.h"

static int run_one(struct runq *r)
{
	struct slist_node *n;
	struct runq_task *t;

	thr_mutex_lock(&r->lock);
	if (r->quit_request) {
		thr_mutex_unlock(&r->lock);
		return -1;
	}

	n = slist_pop(&r->job_list);
	thr_mutex_unlock(&r->lock);

	if (!n)
		return 0;

	t = container_of(n, struct runq_task, job_list);
	t->func(t);
	return 1;
}

static void worker_func(void *arg)
{
	struct runq_worker *w = (struct runq_worker *)arg;

	for (;;) {
		thr_event_wait(&w->wakeup);
		thr_event_clear(&w->wakeup);

		for (;;) {
			int result = run_one(w->parent);

			if (result < 0)
				return;
			if (!result)
				break;
		}
	}
}

static void join_worker(struct runq_worker *w)
{
	thr_event_raise(&w->wakeup);
	thr_join(w->thread);
	thr_event_destroy(&w->wakeup);
}

static void request_quit(struct runq *r)
{
	thr_mutex_lock(&r->lock);
	r->quit_request = 1;
	thr_mutex_unlock(&r->lock);
}

static int init_worker(struct runq *r, struct runq_worker *w)
{
	w->parent = r;

	if (thr_event_init(&w->wakeup) < 0)
		return -1;

	if (thr_start(&w->thread, worker_func, w) < 0) {
		thr_event_destroy(&w->wakeup);
		return -1;
	}

	return 0;
}

int runq_init(struct runq *r, unsigned int bg_workers)
{
	int i;

	r->wakeup = NULL;
	r->num_workers = bg_workers;
	r->quit_request = 0;
	slist_init(&r->job_list);
	thr_mutex_init(&r->lock);

	if (!bg_workers) {
		r->workers = NULL;
		return 0;
	}

	r->workers = malloc(sizeof(r->workers[0]) * bg_workers);
	if (!r->workers) {
		thr_mutex_destroy(&r->lock);
		return -1;
	}

	for (i = 0; i < bg_workers; i++)
		if (init_worker(r, &r->workers[i]) < 0) {
			syserr_t err = syserr_last();

			request_quit(r);

			while (i > 0) {
				i--;
				join_worker(&r->workers[i]);
			}

			free(r->workers);
			thr_mutex_destroy(&r->lock);
			syserr_set(err);
			return -1;
		}

	return 0;
}

void runq_destroy(struct runq *r)
{
	int i;

	if (r->num_workers) {
		request_quit(r);

		for (i = 0; i < r->num_workers; i++)
			join_worker(&r->workers[i]);

		free(r->workers);
	}

	thr_mutex_destroy(&r->lock);
}

unsigned int runq_dispatch(struct runq *r, unsigned int limit)
{
	int count = 0;

	while (!limit || (count < limit)) {
		if (run_one(r) <= 0)
			break;

		count++;
	}

	return count;
}

void runq_task_exec(struct runq_task *t, runq_task_func_t func)
{
	struct runq *r = t->owner;
	int was_empty;

	t->func = func;

	thr_mutex_lock(&r->lock);
	was_empty = slist_is_empty(&r->job_list);
	slist_append(&r->job_list, &t->job_list);
	thr_mutex_unlock(&r->lock);

	if (was_empty) {
		int i;

		for (i = 0; i < r->num_workers; i++)
			thr_event_raise(&r->workers[i].wakeup);

		if (r->wakeup)
			r->wakeup(r);
	}
}
