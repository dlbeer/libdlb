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

#ifndef IO_WAITQ_H_
#define IO_WAITQ_H_

#include "runq.h"
#include "clock.h"
#include "rbt.h"
#include "thr.h"

/* Wakeup hook. This can be used to run a function whenever the deadline
 * of the next-to-expire timer changes.
 */
struct waitq;
typedef void (*waitq_wakeup_t)(struct waitq *q);

/* Wait queue data structure */
struct waitq {
	waitq_wakeup_t	wakeup;
	struct runq	*run;

	thr_mutex_t	lock;
	struct rbt	waiting_set;
};

/* Initialize/destroy a wait queue. Destroying a wait queue does not
 * cause any timers to expire.
 *
 * A waitq needs to be linked to a runq. Timers become runnable tasks on
 * expiry.
 */
void waitq_init(struct waitq *wq, struct runq *rq);
void waitq_destroy(struct waitq *wq);

/* Find the time, in milliseconds, to the next timer expiry. Returns 0
 * if there are timers expired already. Returns -1 if there are no
 * timers in the set.
 */
int waitq_next_deadline(struct waitq *wq);

/* For each timer which has expired, enqueue the completion handler in
 * the linked run queue. A limit may be specified (0 means no limit).
 */
unsigned int waitq_dispatch(struct waitq *wq, unsigned int limit);

/* Timer completion handler function. This is invoked after expiry or
 * cancellation of a timer.
 */
struct waitq_timer;
typedef void (*waitq_timer_func_t)(struct waitq_timer *t);

/* Timer data structure. This does not need to be initialized, provided
 * that no use of it is made before a call to waitq_wait(). If this is a
 * possibility, it should be set memset() to 0 first.
 */
struct waitq_timer {
	/* This must be the first element in the struct */
	struct runq_task	task;

	struct rbt_node		waiting_set;
	clock_ticks_t		deadline;

	struct waitq		*owner;
};

/* Initialize a timer by associating it with a wait queue. */
void waitq_timer_init(struct waitq_timer *t, struct waitq *q);

/* Asynchronously wait for an interval. The given function will be
 * executed after the specified interval has elapsed.
 *
 * The waitq_timer structure may not be modified, reused or destroyed
 * until the callback function has been invoked. As soon as the callback
 * function starts executing, the timer is safe to modify/destroy.
 */
void waitq_timer_wait(struct waitq_timer *t,
		      int interval_ms, waitq_timer_func_t func);

/* Determine whether the given timer expired due to cancellation. This
 * function is not safe to use on an unexpired timer.
 */
static inline int waitq_timer_cancelled(struct waitq_timer *t)
{
	return !t->deadline;
}

/* Attempt to cancel a timer. If the timer is already expired, this
 * function has no effect. If the timer has yet to expire, requesting
 * cancellation will hasten its expiry.
 */
void waitq_timer_cancel(struct waitq_timer *t);

/* Attempt to reschedule a timer. If the timer has already expired, this
 * function has no effect. Otherwise, the timer's deadline is changed.
 */
void waitq_timer_reschedule(struct waitq_timer *t, int interval_ms);

#endif
