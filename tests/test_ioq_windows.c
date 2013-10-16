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

#include <stdio.h>
#include <assert.h>
#include "ioq.h"
#include "prng.h"

static uint8_t pattern[32768];
static struct ioq ioq;
static int done;

static void init_pattern(void)
{
	int i;
	prng_t p;

	prng_init(&p, 0);
	for (i = 0; i < sizeof(pattern); i++)
		pattern[i] = prng_next(&p);
}

/************************************************************************
 * Reader process
 */

static HANDLE pipe_read;
static struct ioq_ovl ovl_read;
static int read_ptr;
static uint8_t rbuf[8192];
static syserr_t read_error;

static void read_done(struct ioq_ovl *o)
{
	DWORD count;

	if (!read_error &&
	    !GetOverlappedResult(pipe_read, ioq_ovl_lpo(&ovl_read),
				 &count, FALSE))
		read_error = syserr_last();

	if (read_error) {
		char msg[128];

		syserr_format(read_error, msg, sizeof(msg));
		printf("Read: %s\n", msg);
		CloseHandle(pipe_read);
		done = 1;
		return;
	}

	printf("Read %d bytes\n", (int)count);
	assert(count);
	assert(!memcmp(rbuf, pattern + read_ptr, count));
	read_ptr += count;

	read_error = 0;
	ioq_ovl_wait(&ovl_read, read_done);
	ReadFile(pipe_read, rbuf, sizeof(rbuf),
		NULL, ioq_ovl_lpo(&ovl_read));
	if (GetLastError() != ERROR_IO_PENDING) {
		read_error = syserr_last();
		ioq_ovl_trigger(&ovl_read);
	}
}

static void connect_done(struct ioq_ovl *o)
{
	DWORD count;
	int r;

	assert(!read_error);
	r = GetOverlappedResult(pipe_read, ioq_ovl_lpo(&ovl_read),
				&count, TRUE);
	assert(r);

	printf("Connected\n");
	read_ptr = 0;

	read_error = 0;
	ioq_ovl_wait(&ovl_read, read_done);
	ReadFile(pipe_read, rbuf, sizeof(rbuf),
		NULL, ioq_ovl_lpo(&ovl_read));
	if (GetLastError() != ERROR_IO_PENDING) {
		read_error = syserr_last();
		ioq_ovl_trigger(&ovl_read);
	}
}

static void begin_read(void)
{
	int r;

	pipe_read = CreateNamedPipe("\\\\.\\pipe\\test_ioq",
		PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE,
		PIPE_UNLIMITED_INSTANCES,
		0, 0, 0, NULL);
	assert(pipe_read != INVALID_HANDLE_VALUE);

	ioq_ovl_init(&ovl_read, &ioq);
	r = ioq_bind(&ioq, pipe_read);
	assert(!r);

	printf("Waiting for connection...\n");
	read_error = 0;
	ioq_ovl_wait(&ovl_read, connect_done);
	ConnectNamedPipe(pipe_read, ioq_ovl_lpo(&ovl_read));
	if (GetLastError() != ERROR_IO_PENDING) {
		read_error = syserr_last();
		ioq_ovl_trigger(&ovl_read);
	}
}

/************************************************************************
 * Writer process
 */

static HANDLE pipe_write;
static struct ioq_ovl ovl_write;
static int write_ptr;
static syserr_t write_error;

static void start_write(void);

static void write_done(struct ioq_ovl *o)
{
	DWORD count;
	int r;

	assert(!write_error);
	r = GetOverlappedResult(pipe_write,
		ioq_ovl_lpo(&ovl_write), &count, TRUE);
	assert(r);

	printf("Wrote %d bytes\n", (int)count);
	assert(count);
	write_ptr += count;
	start_write();
}

static void start_write(void)
{
	int len = sizeof(pattern) - write_ptr;

	if (!len) {
		printf("No further data to write\n");
		CloseHandle(pipe_write);
		return;
	}

	read_error = 0;
	ioq_ovl_wait(&ovl_write, write_done);
	WriteFile(pipe_write, pattern + write_ptr, len,
		NULL, ioq_ovl_lpo(&ovl_write));
	if (GetLastError() != ERROR_IO_PENDING) {
		write_error = syserr_last();
		ioq_ovl_trigger(&ovl_read);
	}
}

static void begin_write(void)
{
	int r;

	pipe_write = CreateFile("\\\\.\\pipe\\test_ioq",
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);
	assert(pipe_write != INVALID_HANDLE_VALUE);

	ioq_ovl_init(&ovl_write, &ioq);
	r = ioq_bind(&ioq, pipe_write);
	assert(!r);

	write_ptr = 0;

	printf("Writing begins\n");
	start_write();
}

int main(void)
{
	int r;

	init_pattern();

	r = ioq_init(&ioq, 0);
	assert(r >= 0);

	begin_read();
	begin_write();

	while (!done)
		ioq_iterate(&ioq);

	ioq_destroy(&ioq);
	return 0;
}
