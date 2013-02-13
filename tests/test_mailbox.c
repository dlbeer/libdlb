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
#include "mailbox.h"
#include "containers.h"

#define N			10

#define MSG_FORWARD		MAILBOX_FLAG(0)
#define MSG_QUIT		MAILBOX_FLAG(1)

struct waiter {
	struct mailbox		mb;
};

static int terminate;
static struct waiter waiters[N];

static void wait_done(struct mailbox *m)
{
	struct waiter *w = container_of(m, struct waiter, mb);
	mailbox_flags_t f = mailbox_take(m, MAILBOX_ALL_FLAGS);
	int n = w - waiters;

	printf("%d: [f = %x]: ", n, f);

	if (f & MSG_QUIT) {
		printf("received MSG_QUIT\n");
		if (n)
			mailbox_raise(&waiters[n - 1].mb, MSG_QUIT);
		else
			terminate = 1;
		return;
	}

	if (f & MSG_FORWARD) {
		printf("received MSG_FORWARD: ");

		if (n + 1 < N) {
			printf("forwarding\n");
			mailbox_raise(&waiters[n + 1].mb, MSG_FORWARD);
		} else {
			printf("self-terminating\n");
			mailbox_raise(&waiters[n].mb, MSG_QUIT);
		}

		mailbox_wait(&waiters[n].mb, MAILBOX_ALL_FLAGS, wait_done);
	}
}

static void begin_waiter(struct waiter *w, struct runq *q)
{
	int n = w - waiters;

	mailbox_init(&w->mb, q);
	printf("%d: start\n", n);
	mailbox_wait(&w->mb, MAILBOX_ALL_FLAGS, wait_done);
}

int main(void)
{
	struct runq rq;
	int i;

	runq_init(&rq, 0);
	for (i = 0; i < N; i++)
		begin_waiter(&waiters[i], &rq);

	mailbox_raise(&waiters[0].mb, MSG_FORWARD);
	while (!terminate)
		runq_dispatch(&rq, 1);

	for (i = 0; i < N; i++) {
		assert(waiters[i].mb.mode == MAILBOX_WAIT_NONE);
		mailbox_destroy(&waiters[i].mb);
	}
	runq_destroy(&rq);
	return 0;
}
