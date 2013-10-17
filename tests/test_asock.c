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
#include <stdint.h>
#include <assert.h>
#include "prng.h"
#include "asock.h"

#define N		65535
#define MAX_WRITE	8192
#define MAX_READ	3172

static uint8_t pattern[N];
static int is_done;

/************************************************************************
 * Reading strand
 */

static struct asock server;
static struct asock reader;
static int read_ptr;
static uint8_t read_buf[MAX_READ];

static void do_receive(void);

static void recv_done(struct asock *a)
{
	const int len = asock_get_recv_size(&reader);

	assert(!asock_get_recv_error(&reader));

	if (!len) {
		printf("server: EOF\n");
		assert(read_ptr == N);
		asock_close(&reader);
		is_done = 1;
		return;
	}

	printf("server: read %d bytes\n", len);

	assert(len <= N - read_ptr);
	assert(!memcmp(pattern + read_ptr, read_buf, len));
	read_ptr += len;

	do_receive();
}

static void do_receive(void)
{
	asock_recv(&reader, read_buf, sizeof(read_buf), recv_done);
}

static void accept_done(struct asock *a)
{
	assert(!asock_get_error(&server));
	printf("server: Connected\n");
	do_receive();
}

static void reader_init(struct ioq *q)
{
	struct sockaddr_in addr;
	int i;

	asock_init(&server, q);
	asock_init(&reader, q);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(50999);

	i = asock_listen(&server, (struct sockaddr *)&addr, sizeof(addr));
	assert(i >= 0);

	printf("server: Accepting...\n");
	asock_accept(&server, &reader, accept_done);
}

static void reader_exit(void)
{
	asock_destroy(&server);
	asock_destroy(&reader);
}

/************************************************************************
 * Writing strand
 */

static struct asock writer;
static int write_ptr;
static struct sockaddr_in peer;

static void begin_write(void);

static void write_done(struct asock *a)
{
	const int len = asock_get_send_size(&writer);

	assert(!asock_get_send_error(&writer));

	printf("client: Sent %d bytes\n", len);
	write_ptr += len;
	begin_write();
}

static void begin_write(void)
{
	int len = N - write_ptr;

	if (len > MAX_WRITE)
		len = MAX_WRITE;

	assert(len >= 0);

	if (len) {
		printf("client: Sending %d bytes\n", len);
		asock_send(&writer, pattern + write_ptr, len, write_done);
	} else {
		printf("client: Close\n");
		asock_close(&writer);
	}
}

static void connect_done(struct asock *a)
{
	assert(!asock_get_error(&writer));
	printf("client: Connected\n");

	begin_write();
}

static void writer_init(struct ioq *q)
{
	asock_init(&writer, q);
	printf("client: Connecting...\n");

	peer.sin_family = AF_INET;
	peer.sin_addr.s_addr = inet_addr("127.0.0.1");
	peer.sin_port = htons(50999);

	asock_connect(&writer, (struct sockaddr *)&peer, sizeof(peer),
		      connect_done);
}

static void writer_exit(void)
{
	asock_destroy(&writer);
}

/************************************************************************
 * Main thread/test
 */

static void init_pattern(void)
{
	int i;
	prng_t prng;

	prng_init(&prng, 1);
	for (i = 0; i < sizeof(pattern); i++)
		pattern[i] = prng_next(&prng);
}

int main(void)
{
	struct ioq q;
	int r;

	init_pattern();

	r = net_start();
	assert(r >= 0);

	r = ioq_init(&q, 0);
	assert(r >= 0);

	reader_init(&q);
	writer_init(&q);

	while (!is_done) {
		const int r = ioq_iterate(&q);

		assert(r >= 0);
	}

	writer_exit();
	reader_exit();
	ioq_destroy(&q);
	net_stop();
	return 0;
}
