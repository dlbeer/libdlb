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

/* DO NOT INCLUDE THIS FILE DIRECTLY */
#include <sys/epoll.h>
#include "syserr.h"
#include "slist.h"

struct ioq {
	struct runq		run;
	struct waitq		wait;

	thr_mutex_t		lock;
	struct slist		mod_list;

	/* Wakeup pipe */
	int			intr[2];
	int			intr_state;

	/* epoll file descriptor */
	int			epoll_fd;
};

/* This is the set of POSIX file descriptor events which can be waited
 * for. These are level-triggered events.
 */
typedef uint32_t ioq_fd_mask_t;

#define IOQ_EVENT_IN		EPOLLIN
#define IOQ_EVENT_OUT		EPOLLOUT
#define IOQ_EVENT_ERR		EPOLLERR
#define IOQ_EVENT_HUP		EPOLLHUP

/* Wait object for use with an IO queue. Each one of these objects is
 * associated with a single file descriptor and may have one outstanding
 * IO operation.
 *
 * No two ioq_fd objects may refer to the same file descriptor.
 *
 * Simultaneous access to the same ioq_fd by multiple threads is not
 * allowed.
 */
#define IOQ_FLAG_MOD_LIST	0x01
#define IOQ_FLAG_EPOLL		0x02
#define IOQ_FLAG_WAITING	0x04

struct ioq_fd {
	/* This must be the first element */
	struct runq_task	task;

	/* This data never changes after init */
	int			fd;
	struct ioq		*owner;

	/* This data is set when dispatching */
	ioq_fd_mask_t		ready;
	syserr_t		err;

	/* If any changes must be made to this fd's epoll association,
	 * it's done by placing the ioq_fd in the mod_list, and setting
	 * the requested field. This data is shared between the client
	 * strand and the dispatch loop.
	 *
	 * The flags field tells us which data structures this ioq_fd
	 * belongs to (mod_list and kernel's internal epoll structures).
	 */
	int			flags;
	struct slist_node	mod_list;
	ioq_fd_mask_t		requested;
};

/* Type of asynchronous callback for an IO wait operation. */
typedef void (*ioq_fd_func_t)(struct ioq_fd *f);

/* Initialize an ioq_fd object, associating it with an IO queue and a
 * file descriptor.
 *
 * The file descriptor is never modified, and the caller remains
 * responsible for closing it.
 */
void ioq_fd_init(struct ioq_fd *f, struct ioq *q, int fd);

/* Get or change the file descriptor associated with this ioq_fd
 * structure. The file descriptor can be changed only if there is no
 * wait in progress.
 */
static inline int ioq_fd_get_fd(const struct ioq_fd *f)
{
	return f->fd;
}

static inline void ioq_fd_set_fd(struct ioq_fd *f, int fd)
{
	f->fd = fd;
}

/* Obtain the set of IO events which are ready (if any) */
static inline ioq_fd_mask_t ioq_fd_ready(const struct ioq_fd *f)
{
	return f->ready;
}

/* Obtain the error code from the last wait operation (will be 0 if no
 * error occured). Note that a successful wait may still result in an
 * error *event*. This is different to an error encountered during the
 * wait operation itself.
 */
static inline syserr_t ioq_fd_error(const struct ioq_fd *f)
{
	return f->err;
}

/* Begin a wait operation on an ioq_fd object. The given callback will
 * be invoked when any of the (level-triggered) events in the set occur.
 * The following rules must be observed:
 *
 *    (1) only one wait operation may be in progress at any time
 *    (2) the ioq_fd may not be modified/destroyed/reused while a wait
 *        operation is in progress (except for cancel/rewait).
 *    (3) the wait operation is ended as soon as the callback begins
 *        execution, at which point it is safe to reuse/modify/destroy
 *        the ioq_fd.
 */
void ioq_fd_wait(struct ioq_fd *f, ioq_fd_mask_t set, ioq_fd_func_t func);

/* Attempt to alter the conditions of an IO operation which is in
 * progress. If successful, the set of events waited for will be
 * altered. If the wait has already terminated (or is in the process of
 * terminating), this operation will have no effect.
 */
void ioq_fd_rewait(struct ioq_fd *f, ioq_fd_mask_t set);

/* Attempt to cancel an IO wait operation. */
static inline void ioq_fd_cancel(struct ioq_fd *f)
{
	ioq_fd_rewait(f, 0);
}
