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
#include "clock.h"
#include "runq.h"

#define N_TASKS		10

static struct runq_task		tests[N_TASKS];
static struct runq		queue;

static int			counter;
static thr_mutex_t		counter_lock;
static thr_event_t		counter_event;

static void task_func(struct runq_task *t)
{
	int n = t - tests;

	printf("  - task func %d dispatched\n", n);

	thr_mutex_lock(&counter_lock);
	counter++;
	thr_mutex_unlock(&counter_lock);
	thr_event_raise(&counter_event);
}

static int read_counter(void)
{
	int r;

	thr_mutex_lock(&counter_lock);
	r = counter;
	thr_mutex_unlock(&counter_lock);

	return r;
}

static void wait_counter(unsigned int bg_threads)
{
	if (bg_threads) {
		thr_event_wait(&counter_event);
		thr_event_clear(&counter_event);
	} else {
		runq_dispatch(&queue, 0);
	}
}

static void test_tasks(unsigned int bg_threads)
{
	int i;

	printf("Test with %d background threads\n", bg_threads);
	counter = 0;
	i = runq_init(&queue, bg_threads);
	assert(i >= 0);

	for (i = 0; i < N_TASKS; i++) {
		runq_task_init(&tests[i], &queue);
		runq_task_exec(&tests[i], task_func);
	}

	while (read_counter() != N_TASKS)
		wait_counter(bg_threads);

	printf("...\n");
	clock_wait(100);
	i = read_counter();
	assert(i == N_TASKS);

	for (i = 0; i < N_TASKS; i++)
		runq_task_exec(&tests[i], task_func);

	while (read_counter() != N_TASKS * 2)
		wait_counter(bg_threads);

	clock_wait(100);
	i = read_counter();
	assert(i == N_TASKS * 2);

	runq_destroy(&queue);
	printf("\n");
}

int main(void)
{
	int r;

	thr_mutex_init(&counter_lock);
	r = thr_event_init(&counter_event);
	assert(r >= 0);

	test_tasks(0);
	test_tasks(4);

	thr_mutex_destroy(&counter_lock);
	thr_event_destroy(&counter_event);
	return 0;
}
