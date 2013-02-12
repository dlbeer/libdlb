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

#ifndef IO_RUNQ_H_
#define IO_RUNQ_H_

#include "thr.h"
#include "slist.h"

/* Asynchronous run-queue. This object manages a pool of worker threads,
 * to which tasks (functions) may be submitted for execution.
 */
struct runq;
typedef void (*runq_wakeup_t)(struct runq *r);

struct runq_worker {
	struct runq		*parent;
	thr_thread_t		thread;
	thr_event_t		wakeup;
};

struct runq {
	/* You can set this hook to a function to be called whenever the
	 * queue goes from empty to non-empty. It must be configured
	 * before submitting any jobs.
	 */
	runq_wakeup_t		wakeup;

	unsigned int		num_workers;
	struct runq_worker	*workers;

	thr_mutex_t		lock;
	struct slist		job_list;
	int			quit_request;
};

/* Initialize a run-queue, specifying the number of background workers.
 * If this number is 0, no background workers will be created, and the
 * queue must be flushed periodically.
 */
int runq_init(struct runq *r, unsigned int bg_workers);

/* Destroy the run-queue, and tear down any background workers. */
void runq_destroy(struct runq *r);

/* If no background threads are available, you need to periodically call
 * this function to execute tasks. You may optionally specify a limit (0
 * means no limit), and the function will return the number of tasks
 * executed.
 */
unsigned int runq_dispatch(struct runq *r, unsigned int limit);

/* A submittable job is represented by the following structure. */
struct runq_task;
typedef void (*runq_task_func_t)(struct runq_task *t);

struct runq_task {
	struct slist_node	job_list;
	runq_task_func_t	func;
	struct runq		*owner;
};

/* Initialize a task by associating it with a run-queue. */
static inline void runq_task_init(struct runq_task *t, struct runq *q)
{
	t->owner = q;
}

/* Submit a job to the run-queue. The specified function will be
 * executed at some later date. Until that occurs, you must not modify
 * or move the runq_task structure. As soon as the function is invoked,
 * the runq_task structure is no longer referenced by the runq.
 *
 * This function can be called from any thread, but concurrent access to
 * the same task is forbidden.
 *
 * You may not re-use a runq_task structure which is still pending
 * execution.
 */
void runq_task_exec(struct runq_task *t, runq_task_func_t func);

#endif
