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

#ifndef IO_THR_H_
#define IO_THR_H_

/* Thread start routine */
typedef void (*thr_func_t)(void *arg);

#ifdef __Windows__
#include "winapi.h"

/* Thread creation/join */
typedef HANDLE thr_thread_t;

static inline int thr_start(thr_thread_t *thr, thr_func_t func, void *arg)
{
	*thr = CreateThread(NULL, 0,
	    (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);

	return (*thr) ? 0 : -1;
}

static inline void thr_join(thr_thread_t thr)
{
	WaitForSingleObject(thr, INFINITE);
}

/* Mutexes */
typedef CRITICAL_SECTION thr_mutex_t;

static inline void thr_mutex_init(thr_mutex_t *m)
{
	InitializeCriticalSection(m);
}

static inline void thr_mutex_destroy(thr_mutex_t *m)
{
	DeleteCriticalSection(m);
}

static inline void thr_mutex_lock(thr_mutex_t *m)
{
	EnterCriticalSection(m);
}

static inline void thr_mutex_unlock(thr_mutex_t *m)
{
	LeaveCriticalSection(m);
}

/* Event variables */
typedef HANDLE thr_event_t;

static inline int thr_event_init(thr_event_t *e)
{
	*e = CreateEvent(NULL, TRUE, FALSE, NULL);

	return *e ? 0 : -1;
}

static inline void thr_event_destroy(thr_event_t *e)
{
	CloseHandle(*e);
}

static inline void thr_event_raise(thr_event_t *e)
{
	SetEvent(*e);
}

static inline void thr_event_clear(thr_event_t *e)
{
	ResetEvent(*e);
}

static inline void thr_event_wait(thr_event_t *e)
{
	WaitForSingleObject(*e, INFINITE);
}

static inline int thr_event_wait_timeout(thr_event_t *e, int timeout_ms)
{
	return WaitForSingleObject(*e, timeout_ms);
}
#else
#include <pthread.h>

/* Thread creation/join */
typedef pthread_t thr_thread_t;

static inline int thr_start(thr_thread_t *thr, thr_func_t func, void *arg)
{
	return pthread_create(thr, NULL, (void *(*)(void *))func, arg);
}

static inline void thr_join(thr_thread_t thr)
{
	pthread_join(thr, NULL);
}

/* Mutexes */
typedef pthread_mutex_t thr_mutex_t;

static inline void thr_mutex_init(thr_mutex_t *m)
{
	pthread_mutex_init(m, NULL);
}

static inline void thr_mutex_destroy(thr_mutex_t *m)
{
	pthread_mutex_destroy(m);
}

static inline void thr_mutex_lock(thr_mutex_t *m)
{
	pthread_mutex_lock(m);
}

static inline void thr_mutex_unlock(thr_mutex_t *m)
{
	pthread_mutex_unlock(m);
}

/* Event variables */
struct thr_pthread_event {
	pthread_mutex_t		lock;
	pthread_cond_t		cond;
	int			state;
};

typedef struct thr_pthread_event thr_event_t;

int thr_event_init(thr_event_t *e);
void thr_event_destroy(thr_event_t *e);
void thr_event_raise(thr_event_t *e);
void thr_event_clear(thr_event_t *e);
void thr_event_wait(thr_event_t *e);
int thr_event_wait_timeout(thr_event_t *e, int timeout_ms);
#endif

#endif
