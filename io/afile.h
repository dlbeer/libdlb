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

#ifndef IO_AFILE_H_
#define IO_AFILE_H_

#include <stddef.h>
#include "syserr.h"
#include "ioq.h"
#include "thr.h"
#include "handle.h"

/* Asynchronous file handle manager. This allows, independently, reads
 * and writes to be started on a file handle. The object may be
 * destroyed if no operations are outstanding.
 */
struct afile;
typedef void (*afile_func_t)(struct afile *a);

#ifdef __Windows__
struct afile_op {
	afile_func_t		func;
	struct ioq_ovl		ovl;
	DWORD			size;
	syserr_t		error;
};

struct afile {
	handle_t		handle;
	struct afile_op		read;
	struct afile_op		write;
};
#else
struct afile_op {
	afile_func_t		func;
	void			*buffer;
	size_t			size;
	syserr_t		error;
};

struct afile {
	struct ioq_fd		fd; /* Must be first */

	struct afile_op		read;
	struct afile_op		write;

	thr_mutex_t		lock;
	int			flags;
};
#endif

/* Initialize an asynchronous file. Ownership of the file handle is not
 * taken -- it must be destroyed independently.
 */
void afile_init(struct afile *a, struct ioq *q, handle_t h);

#ifdef __Windows__
static inline void afile_destroy(struct afile *a) { }
#else
void afile_destroy(struct afile *a);
#endif

/* Obtain or change the handle associated with the asynchronous file.
 * The handle can be changed only if there are no wait operations in
 * progress.
 */
#ifdef __Windows__
static inline handle_t afile_get_handle(const struct afile *a)
{
	return a->handle;
}

static inline void afile_set_handle(struct afile *a, handle_t h)
{
	a->handle = h;
}
#else
static inline handle_t afile_get_handle(const struct afile *a)
{
	return ioq_fd_get_fd(&a->fd);
}

static inline void afile_set_handle(struct afile *a, handle_t h)
{
	ioq_fd_set_fd(&a->fd, h);
}
#endif

/* Begin an asynchronous write operation */
void afile_write(struct afile *a, const void *data, size_t len,
		 afile_func_t func);

/* Obtain the error code and result of a write operation. If the write
 * was successful, the error will be SYSERR_NONE.
 */
static inline size_t afile_write_size(const struct afile *a)
{
	return a->write.size;
}

static inline syserr_t afile_write_error(const struct afile *a)
{
	return a->write.error;
}

/* Begin an asynchronous read operation */
void afile_read(struct afile *a, void *data, size_t len,
		afile_func_t func);

/* Obtain the error code and result of a read operation. If the read
 * was successful, the error will be SYSERR_NONE.
 */
static inline size_t afile_read_size(const struct afile *a)
{
	return a->read.size;
}

static inline syserr_t afile_read_error(const struct afile *a)
{
	return a->read.error;
}

/* Cancel all outstanding operations. The result of any IO operations
 * will be undefined.
 */
#ifdef __Windows__
static inline void afile_cancel(struct afile *a)
{
	CancelIo(a->handle);
}
#else
void afile_cancel(struct afile *a);
#endif

#endif
