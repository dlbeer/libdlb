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
#include "thr.h"
#include "syserr.h"
#include "clock.h"

static int counter;
static thr_thread_t worker;
static thr_mutex_t mutex;
static thr_event_t event;

static void report_syserr(const char *prefix)
{
	const syserr_t err = syserr_last();
	char msg[128];

	syserr_format(err, msg, sizeof(msg));
	fprintf(stderr, "%s: %s\n", prefix, msg);
}

static void work_func(void *arg)
{
	int my_count = 0;

	while (my_count < 5) {
		thr_event_wait(&event);
		thr_event_clear(&event);
		thr_mutex_lock(&mutex);
		while (counter) {
			counter--;
			my_count++;
			printf("consumer: counter--\n");
		}
		thr_mutex_unlock(&mutex);
	}
}

static void test_timedwait(void)
{
	clock_ticks_t before, after;
	int r;

	thr_event_clear(&event);
	printf("With event clear...\n");

	before = clock_now();
	printf("  current time: %" CLOCK_PRI_TICKS "\n", before);
	r = thr_event_wait_timeout(&event, 500);
	assert(r);
	after = clock_now();
	printf("  after 500 ms wait: %" CLOCK_PRI_TICKS "\n", after);

	assert((after >= before + 450) && (after <= before + 550));

	thr_event_raise(&event);
	printf("With event set...\n");

	before = clock_now();
	printf("  current time: %" CLOCK_PRI_TICKS "\n", before);
	r = thr_event_wait_timeout(&event, 500);
	assert(!r);
	after = clock_now();
	printf("  after 500 ms wait: %" CLOCK_PRI_TICKS "\n", after);

	assert(after <= before + 50);
}

int main(void)
{
	int my_count = 5;

	thr_mutex_init(&mutex);
	if (thr_event_init(&event) < 0) {
		report_syserr("thr_event_init");
		return -1;
	}

	thr_start(&worker, work_func, NULL);
	while (my_count) {
		thr_mutex_lock(&mutex);
		counter++;
		my_count--;
		printf("producer: counter++\n");
		thr_mutex_unlock(&mutex);
		thr_event_raise(&event);
		clock_wait(10);
	}
	thr_join(worker);

	test_timedwait();

	thr_event_destroy(&event);
	thr_mutex_destroy(&mutex);
	return 0;
}
