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

#include "afile.h"
#include "containers.h"

void afile_init(struct afile *a, struct ioq *q, handle_t h)
{
	a->handle = h;
	ioq_ovl_init(&a->read.ovl, q);
	ioq_ovl_init(&a->write.ovl, q);
}

static void write_done(struct ioq_ovl *o)
{
	struct afile_op *op = container_of(o, struct afile_op, ovl);
	struct afile *a = container_of(op, struct afile, write);

	if (a->write.error == ERROR_IO_PENDING) {
		if (GetOverlappedResult(a->handle, ioq_ovl_lpo(&a->write.ovl),
					&a->write.size, TRUE))
			a->write.error = 0;
		else
			a->write.error = GetLastError();
	}

	if (a->write.error)
		a->write.size = 0;

	a->write.func(a);
}

void afile_write(struct afile *a, const void *data, size_t len,
		 afile_func_t func)
{
	a->write.func = func;
	a->write.error = ERROR_IO_PENDING;

	ioq_ovl_wait(&a->write.ovl, write_done);
	if (!WriteFile(a->handle, data, len, &a->write.size,
		       ioq_ovl_lpo(&a->write.ovl))) {
		const syserr_t e = GetLastError();

		if (e != ERROR_IO_PENDING) {
			a->write.error = e;
			ioq_ovl_trigger(&a->write.ovl);
		}
	} else {
		a->write.error = 0;
		ioq_ovl_trigger(&a->write.ovl);
	}
}

static void read_done(struct ioq_ovl *o)
{
	struct afile_op *op = container_of(o, struct afile_op, ovl);
	struct afile *a = container_of(op, struct afile, read);

	if (a->read.error == ERROR_IO_PENDING) {
		if (GetOverlappedResult(a->handle, ioq_ovl_lpo(&a->read.ovl),
					&a->read.size, TRUE))
			a->read.error = 0;
		else
			a->read.error = GetLastError();
	}

	if (a->read.error)
		a->read.size = 0;

	a->read.func(a);
}

void afile_read(struct afile *a, void *data, size_t len,
		afile_func_t func)
{
	a->read.func = func;
	a->read.error = ERROR_IO_PENDING;

	ioq_ovl_wait(&a->read.ovl, read_done);
	if (!ReadFile(a->handle, data, len, &a->read.size,
		      ioq_ovl_lpo(&a->read.ovl))) {
		const syserr_t e = GetLastError();

		if (e != ERROR_IO_PENDING) {
			a->read.error = e;
			ioq_ovl_trigger(&a->read.ovl);
		}
	} else {
		a->read.error = 0;
		ioq_ovl_trigger(&a->read.ovl);
	}
}
