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

#define N		65536
#define MAX_WRITE	8192
#define MAX_READ	3172

static uint8_t pattern[N];
static uint8_t out[N];

/************************************************************************
 * Writer strand
 */
struct writer_proc {
	struct ioq_fd		writer;
	struct waitq_timer	timer;
	int			ptr;
};

static void write_ready(struct ioq_fd *f);
static void wait_done(struct waitq_timer *timer);

static void begin_wait(struct writer_proc *w)
{
	waitq_timer_wait(&w->timer, 50, wait_done);

	/* Deliberately wait for the wrong thing here so that we can try
	 * rewait() later.
	 */
	ioq_fd_wait(&w->writer, IOQ_EVENT_IN, write_ready);
}

static void write_ready(struct ioq_fd *f)
{
	struct writer_proc *w = container_of(f, struct writer_proc, writer);
	int xfer = sizeof(pattern) - w->ptr;
	int ret;

	assert(!ioq_fd_error(f));
	assert(ioq_fd_ready(f) == IOQ_EVENT_OUT);

	if (xfer > MAX_WRITE)
		xfer = MAX_WRITE;

	ret = write(f->fd, pattern + w->ptr, xfer);
	assert(ret > 0);

	printf("-- wrote %d bytes\n", ret);
	w->ptr += ret;

	if (w->ptr >= sizeof(pattern)) {
		printf("Writer done\n");
		close(f->fd);
	} else {
		begin_wait(w);
	}
}

static void wait_done(struct waitq_timer *timer)
{
	struct writer_proc *w = container_of(timer, struct writer_proc, timer);

	ioq_fd_rewait(&w->writer, IOQ_EVENT_OUT);
}

static void writer_start(struct writer_proc *w, struct ioq *q, int fd)
{
	ioq_fd_init(&w->writer, q, fd);
	waitq_timer_init(&w->timer, ioq_waitq(q));
	w->ptr = 0;

	begin_wait(w);
}

/************************************************************************
 * Reader strand
 */
struct reader_proc {
	struct ioq_fd	reader;
	int		ptr;
	int		eof;
};

static void read_ready(struct ioq_fd *f)
{
	struct reader_proc *r = container_of(f, struct reader_proc, reader);
	int xfer = sizeof(out) - r->ptr;
	int ret;

	if (xfer > MAX_READ)
		xfer = MAX_READ;

	ret = read(f->fd, out + r->ptr, xfer);
	assert(ret >= 0);

	if (!ret) {
		r->eof = 1;
		close(f->fd);
		printf("End of read stream\n");
	} else {
		printf("-- read %d bytes\n", ret);
		r->ptr += ret;
		ioq_fd_wait(&r->reader, IOQ_EVENT_IN, read_ready);
	}
}

static void reader_start(struct reader_proc *r, struct ioq *q, int fd)
{
	ioq_fd_init(&r->reader, q, fd);
	r->ptr = 0;
	r->eof = 0;

	ioq_fd_wait(&r->reader, IOQ_EVENT_IN, read_ready);
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
