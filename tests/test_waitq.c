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
#include <stdio.h>
#include "waitq.h"
#include "clock.h"

#define N_TIMERS	10

static struct waitq		waitq;
static struct runq		runq;
static struct waitq_timer	test_timers[N_TIMERS];
static int			counter;

static void test_timeout(struct waitq_timer *t)
{
	int n = t - test_timers;

	printf("  -- expired %d%s\n", n,
	       waitq_timer_cancelled(t) ? " (cancelled)" : "");
	counter++;
}

int main(void)
{
	clock_ticks_t before = clock_now();
	clock_ticks_t after;
	int i;

	runq_init(&runq, 0);
	waitq_init(&waitq, &runq);

	/* Schedule some timers */
	for (i = 0; i < N_TIMERS; i++) {
		waitq_timer_init(&test_timers[i], &waitq);
		waitq_timer_wait(&test_timers[i], i * 50 + 50,
				 test_timeout);
	}

	waitq_timer_cancel(&test_timers[5]);

	/* Keep running until they all expire */
	while (counter < N_TIMERS) {
		clock_wait(waitq_next_deadline(&waitq));
		waitq_dispatch(&waitq, 0);
		runq_dispatch(&runq, 0);
	}

	after = clock_now();
	waitq_destroy(&waitq);
	runq_destroy(&runq);

	printf("Running time: %" CLOCK_PRI_TICKS "\n", after - before);
	assert(after >= before + 400);
	assert(after <= before + 600);
	return 0;
}
