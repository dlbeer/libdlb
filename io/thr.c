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

#include "thr.h"

#ifndef __Windows__
#include <sys/time.h>

int thr_event_init(thr_event_t *e)
{
	pthread_mutex_init(&e->lock, NULL);
	pthread_cond_init(&e->cond, NULL);
	e->state = 0;
	return 0;
}

void thr_event_destroy(thr_event_t *e)
{
	pthread_cond_destroy(&e->cond);
	pthread_mutex_destroy(&e->lock);
}

void thr_event_raise(thr_event_t *e)
{
	int old;

	pthread_mutex_lock(&e->lock);
	old = e->state;
	e->state = 1;
	pthread_mutex_unlock(&e->lock);

	if (!old)
		pthread_cond_signal(&e->cond);
}

void thr_event_clear(thr_event_t *e)
{
	pthread_mutex_lock(&e->lock);
	e->state = 0;
	pthread_mutex_unlock(&e->lock);
}

void thr_event_wait(thr_event_t *e)
{
	pthread_mutex_lock(&e->lock);
	while (!e->state)
		pthread_cond_wait(&e->cond, &e->lock);
	pthread_mutex_unlock(&e->lock);
}

int thr_event_wait_timeout(thr_event_t *e, int timeout_ms)
{
	struct timeval now;
	struct timespec to;

	gettimeofday(&now, NULL);
	to.tv_sec = now.tv_sec + timeout_ms / 1000;
	to.tv_nsec = now.tv_usec * 1000LL +
		(timeout_ms % 1000) * 1000000LL;

	if (to.tv_nsec >= 1000000000LL) {
		to.tv_nsec -= 1000000000LL;
		to.tv_sec++;
	}

	pthread_mutex_lock(&e->lock);
	while (!e->state)
		if (pthread_cond_timedwait(&e->cond, &e->lock, &to)) {
			pthread_mutex_unlock(&e->lock);
			return -1;
		}
	pthread_mutex_unlock(&e->lock);

	return 0;
}
#endif
