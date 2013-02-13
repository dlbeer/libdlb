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

#ifndef IO_MAILBOX_H_
#define IO_MAILBOX_H_

#include <stdint.h>
#include "runq.h"
#include "thr.h"

/* A mailbox is an asynchronous IPC primitive. Associated with a mailbox
 * are a set of 32 flags, which can be either raised or cleared.
 *
 * Flags can be raised from any of multiple producer threads. A single
 * consumer thread can asynchronously wait for any subset of the flags
 * to become raised.
 *
 * Flag sets are represented using a bitmask.
 */
typedef uint32_t mailbox_flags_t;

#define MAILBOX_NUM_FLAGS	32
#define MAILBOX_ALL_FLAGS	((mailbox_flags_t)0xffffffff)
#define MAILBOX_FLAG(n)		(((mailbox_flags_t)1) << (n))

/* Mailbox data structure. A mailbox may be accessed from only a single
 * thread at a time, with the exception of mailbox_raise(), which may be
 * called from any number of threads on a valid mailbox.
 */
typedef enum {
	MAILBOX_WAIT_NONE	= 0,
	MAILBOX_WAIT_ANY,
	MAILBOX_WAIT_ALL
} mailbox_wait_mode_t;

struct mailbox {
	/* Must be first element */
	struct runq_task	task;

	thr_mutex_t		lock;
	mailbox_flags_t		state;
	mailbox_flags_t		expected;
	mailbox_wait_mode_t	mode;
};

/* Initialize a mailbox, linking it with a run queue */
void mailbox_init(struct mailbox *m, struct runq *q);

/* Destroy a mailbox */
void mailbox_destroy(struct mailbox *m);

/* Obtain the current mailbox state. The optional clear argument
 * specifies which flags should be cleared, if any.
 */
mailbox_flags_t mailbox_take(struct mailbox *m, mailbox_flags_t clear_mask);

/* Raise the given set mailbox flags. Other flags are untouched. This
 * function may be called from any thread.
 */
void mailbox_raise(struct mailbox *m, mailbox_flags_t set_mask);

/* Begin an asynchronous wait on a mailbox. The wait will be completed
 * when any of the flags in the set become raised. If any of the flags
 * are already raised, the wait completes immediately. Waiting for the
 * empty set will also complete immediately.
 *
 * You may not use/modify/destroy the mailbox until the wait is complete
 * (apart from calling mailbox_raise()).
 */
typedef void (*mailbox_func_t)(struct mailbox *m);

void mailbox_wait(struct mailbox *m, mailbox_flags_t set, mailbox_func_t cb);

/* Begin an asynchronous wait on a mailbox, but don't allow it to
 * complete until *all* of the flags in the waiting set become raised.
 */
void mailbox_wait_all(struct mailbox *m, mailbox_flags_t set,
		      mailbox_func_t cb);

#endif
