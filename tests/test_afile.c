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

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "ioq.h"
#include "containers.h"
#include "afile.h"

#define N		65536
#define MAX_WRITE	8192
#define MAX_READ	3172

static uint8_t pattern[N];
static uint8_t out[N + 1];

/************************************************************************
 * Writer strand
 */
struct writer_proc {
	struct afile		writer;
	struct waitq_timer	timer;
	int			ptr;
};

static void wait_done(struct waitq_timer *timer);

static void begin_wait(struct writer_proc *w)
{
	waitq_timer_wait(&w->timer, 50, wait_done);
}

static void write_done(struct afile *f)
{
	struct writer_proc *w = container_of(f, struct writer_proc, writer);

	assert(!afile_write_error(f));

	printf("-- wrote %d bytes\n", (int)afile_write_size(f));
	w->ptr += afile_write_size(f);

	if (w->ptr >= sizeof(pattern)) {
		printf("Writer done\n");
		close(afile_get_handle(&w->writer));
	} else {
		begin_wait(w);
	}
}

static void wait_done(struct waitq_timer *timer)
{
	struct writer_proc *w = container_of(timer, struct writer_proc, timer);
	int xfer = sizeof(pattern) - w->ptr;

	if (xfer > MAX_WRITE)
		xfer = MAX_WRITE;

	afile_write(&w->writer, pattern + w->ptr, xfer, write_done);
}

static void writer_start(struct writer_proc *w, struct ioq *q, int fd)
{
	afile_init(&w->writer, q, fd);
	waitq_timer_init(&w->timer, ioq_waitq(q));
	w->ptr = 0;

	begin_wait(w);
}

/************************************************************************
 * Reader strand
 */
struct reader_proc {
	struct afile	reader;
	int		ptr;
	int		eof;
};

static void read_done(struct afile *a);

static void begin_read(struct reader_proc *r)
{
	int xfer = sizeof(out) - r->ptr;

	if (xfer > MAX_READ)
		xfer = MAX_READ;

	afile_read(&r->reader, out + r->ptr, xfer, read_done);
}

static void read_done(struct afile *a)
{
	struct reader_proc *r = container_of(a, struct reader_proc, reader);

	assert (!afile_read_error(&r->reader));

	if (!afile_read_size(&r->reader)) {
		r->eof = 1;
		close(afile_get_handle(&r->reader));
		printf("End of read stream\n");
	} else {
		r->ptr += afile_read_size(&r->reader);
		printf("-- read %d bytes\n",
			(int)afile_read_size(&r->reader));
		begin_read(r);
	}
}

static void reader_start(struct reader_proc *r, struct ioq *q, int fd)
{
	afile_init(&r->reader, q, fd);
	r->ptr = 0;
	r->eof = 0;
	begin_read(r);
}

/************************************************************************
 * Main thread/test
 */

static void init_pattern(void)
{
	int i;

	for (i = 0; i < sizeof(pattern); i++)
		pattern[i] = random();
}

int main(void)
{
	struct ioq ioq;
	struct writer_proc writer;
	struct reader_proc reader;
	int pfd[2];
	int r;

	init_pattern();

	r = pipe(pfd);
	assert(r >= 0);

	r = ioq_init(&ioq, 0);
	assert(r >= 0);

	writer_start(&writer, &ioq, pfd[1]);
	reader_start(&reader, &ioq, pfd[0]);

	while (!reader.eof) {
		r = ioq_iterate(&ioq);
		assert(r >= 0);
	}

	ioq_destroy(&ioq);

	assert(!memcmp(pattern, out, N));
	return 0;
}
