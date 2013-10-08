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
	ioq_ovl_init(&a->read.ovl, q, h);
	ioq_ovl_init(&a->write.ovl, q, h);
}

static void write_done(struct ioq_ovl *o)
{
	struct afile_op *op = container_of(o, struct afile_op, ovl);
	struct afile *a = container_of(op, struct afile, write);

	a->write.func(a);
}

void afile_write(struct afile *a, const void *data, size_t len,
		 afile_func_t func)
{
	a->write.func = func;

	ioq_ovl_wait(&a->write.ovl, write_done);
	WriteFile(ioq_ovl_handle(&a->write.ovl), data, len,
		  NULL, ioq_ovl_lpo(&a->write.ovl));
	if (GetLastError() != ERROR_IO_PENDING)
		ioq_ovl_trigger(&a->write.ovl, GetLastError());
}

static void read_done(struct ioq_ovl *o)
{
	struct afile_op *op = container_of(o, struct afile_op, ovl);
	struct afile *a = container_of(op, struct afile, read);

	a->read.func(a);
}

void afile_read(struct afile *a, void *data, size_t len,
		afile_func_t func)
{
	a->read.func = func;

	ioq_ovl_wait(&a->read.ovl, read_done);
	ReadFile(ioq_ovl_handle(&a->read.ovl), data, len,
		 NULL, ioq_ovl_lpo(&a->read.ovl));
	if (GetLastError() != ERROR_IO_PENDING)
		ioq_ovl_trigger(&a->read.ovl, GetLastError());
}
