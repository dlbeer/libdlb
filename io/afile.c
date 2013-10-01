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
#include "afile.h"

#define F_WANT_READ		0x01
#define F_WANT_WRITE		0x02
#define F_WANT_CANCEL		0x04

static void ioq_cb(struct ioq_fd *f);

static int end_wait(struct afile *a, syserr_t *err)
{
	ioq_fd_mask_t wait_mask = 0;
	int perform = 0;

	thr_mutex_lock(&a->lock);

	/* Check for an error */
	if (ioq_fd_error(&a->fd)) {
		perform = a->flags | F_WANT_CANCEL;
		a->flags = 0;
		*err = ioq_fd_error(&a->fd);
		goto out;
	}

	*err = 0;

	/* Check for cancellation request */
	if (a->flags & F_WANT_CANCEL) {
		perform = a->flags | F_WANT_CANCEL;
		a->flags = 0;
		goto out;
	}

	/* Choose actions to perform, and new things to wait for */
	if (a->flags & F_WANT_WRITE) {
		if (ioq_fd_ready(&a->fd) &
		    (IOQ_EVENT_OUT | IOQ_EVENT_ERR | IOQ_EVENT_HUP)) {
			perform |= F_WANT_WRITE;
			a->flags &= ~F_WANT_WRITE;
		} else {
			wait_mask |= IOQ_EVENT_OUT;
		}
	}

	if (a->flags & F_WANT_READ) {
		if (ioq_fd_ready(&a->fd) &
		    (IOQ_EVENT_IN | IOQ_EVENT_ERR | IOQ_EVENT_HUP)) {
			perform |= F_WANT_READ;
			a->flags &= ~F_WANT_READ;
		} else {
			wait_mask |= IOQ_EVENT_IN;
		}
	}

	/* Wait for more IO events */
	if (wait_mask)
		ioq_fd_wait(&a->fd, wait_mask, ioq_cb);

out:
	thr_mutex_unlock(&a->lock);
	return perform;
}

static void ioq_cb(struct ioq_fd *f)
{
	struct afile *a = (struct afile *)f;
	syserr_t error;
	int perform = end_wait(a, &error);

	/* Perform actions */
	if (perform & F_WANT_READ) {
		if (perform & F_WANT_CANCEL) {
			a->read.size = 0;
			a->read.error = error;
		} else {
			int r = read(a->fd.fd, a->read.buffer, a->read.size);

			if (r < 0) {
				a->read.size = 0;
				a->read.error = syserr_last();
			} else {
				a->read.size = r;
				a->read.error = SYSERR_NONE;
			}
		}

		a->read.func(a);
	}

	if (perform & F_WANT_WRITE) {
		if (perform & F_WANT_CANCEL) {
			a->write.size = 0;
			a->write.error = error;
		} else {
			int r = write(a->fd.fd,
				a->write.buffer, a->write.size);

			if (r < 0) {
				a->read.size = 0;
				a->read.error = syserr_last();
			} else {
				a->read.size = r;
				a->read.error = SYSERR_NONE;
			}
		}

		a->write.func(a);
	}
}

void afile_init(struct afile *a, struct ioq *q, handle_t h)
{
	ioq_fd_init(&a->fd, q, h);
	a->flags = 0;
	memset(&a->read, 0, sizeof(a->read));
	memset(&a->write, 0, sizeof(a->write));
	thr_mutex_init(&a->lock);
}

void afile_destroy(struct afile *a)
{
	thr_mutex_destroy(&a->lock);
}

void afile_write(struct afile *a, const void *data, size_t len,
		 afile_func_t func)
{
	a->write.buffer = (void *)data;
	a->write.size = len;
	a->write.func = func;

	thr_mutex_lock(&a->lock);

	if (a->flags)
		ioq_fd_rewait(&a->fd, IOQ_EVENT_IN | IOQ_EVENT_OUT);
	else
		ioq_fd_wait(&a->fd, IOQ_EVENT_OUT, ioq_cb);

	a->flags |= F_WANT_WRITE;
	thr_mutex_unlock(&a->lock);
}

void afile_read(struct afile *a, void *data, size_t len,
		afile_func_t func)
{
	a->read.buffer = data;
	a->read.size = len;
	a->read.func = func;

	thr_mutex_lock(&a->lock);

	if (a->flags)
		ioq_fd_rewait(&a->fd, IOQ_EVENT_IN | IOQ_EVENT_OUT);
	else
		ioq_fd_wait(&a->fd, IOQ_EVENT_IN, ioq_cb);

	a->flags |= F_WANT_READ;
	thr_mutex_unlock(&a->lock);
}

void afile_cancel(struct afile *a)
{
	thr_mutex_lock(&a->lock);
	ioq_fd_cancel(&a->fd);
	a->flags |= F_WANT_CANCEL;
	thr_mutex_unlock(&a->lock);
}
