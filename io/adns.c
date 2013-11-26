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

#include <string.h>
#include "syserr.h"
#include "containers.h"
#include "adns.h"

/* Resolver flags */
#define RESOLVER_QUIT		0x01

/* Request flags */
#define REQUEST_BUSY		0x01

struct work_req {
	struct adns_request	*source;

	const char		*host;
	const char		*service;
	struct addrinfo		*hints;

	char			host_buf[128];
	char			service_buf[128];
	struct addrinfo		hints_buf;
};

static int peek_request(struct adns_resolver *r, struct work_req *req)
{
	struct adns_request *q;

	thr_mutex_lock(&r->lock);
	if (r->flags & RESOLVER_QUIT) {
		thr_mutex_unlock(&r->lock);
		return -1;
	}

	if (list_is_empty(&r->requests)) {
		thr_mutex_unlock(&r->lock);
		return 0;
	}

	q = container_of(r->requests.next, struct adns_request, list);
	req->source = q;

	if (q->hostname) {
		strncpy(req->host_buf, q->hostname, sizeof(req->host_buf));
		req->host_buf[sizeof(req->host_buf) - 1] = 0;
		req->host = req->host_buf;
	} else {
		req->host = NULL;
	}

	if (q->service) {
		strncpy(req->service_buf, q->service,
			sizeof(req->service_buf));
		req->service_buf[sizeof(req->service_buf) - 1] = 0;
		req->service = req->service_buf;
	} else {
		req->service = NULL;
	}

	if (q->hints) {
		memcpy(&req->hints_buf, q->hints, sizeof(req->hints_buf));
		req->hints = &req->hints_buf;
	} else {
		req->hints = NULL;
	}
	thr_mutex_unlock(&r->lock);

	return 1;
}

static void fulfill_request(struct adns_resolver *r, struct adns_request *q,
			    adns_error_t err_code, struct addrinfo *info)
{
	thr_mutex_lock(&r->lock);
	if (r->requests.next != &q->list) {
		thr_mutex_unlock(&r->lock);
		freeaddrinfo(info);
		return;
	}

	list_remove(&q->list);
	thr_mutex_unlock(&r->lock);

	q->flags = 0;
	q->error = err_code;
	q->result = info;

	runq_task_exec(&q->task, q->task.func);
}

static void do_work(void *arg)
{
	struct adns_resolver *r = (struct adns_resolver *)arg;

	for (;;) {
		struct addrinfo *info = NULL;
		struct work_req req;
		adns_error_t err_code;
		int i;

		i = peek_request(r, &req);
		if (!i) {
			thr_event_wait(&r->notify);
			thr_event_clear(&r->notify);
			continue;
		}

		if (i < 0)
			break;

		err_code = getaddrinfo(req.host, req.service, req.hints,
				       &info);
		fulfill_request(r, req.source, err_code, info);
	}
}

int adns_resolver_init(struct adns_resolver *r, struct runq *q)
{
	r->runq = q;
	r->flags = 0;
	list_init(&r->requests);

	if (thr_event_init(&r->notify) < 0)
		return -1;

	thr_mutex_init(&r->lock);
	if (thr_start(&r->worker, do_work, r) < 0) {
		const syserr_t save = syserr_last();

		thr_event_destroy(&r->notify);
		thr_mutex_destroy(&r->lock);
		syserr_set(save);
		return -1;
	}

	return 0;
}

void adns_resolver_destroy(struct adns_resolver *r)
{
	thr_mutex_lock(&r->lock);
	r->flags |= RESOLVER_QUIT;
	thr_mutex_unlock(&r->lock);

	thr_event_raise(&r->notify);
	thr_join(r->worker);
}

void adns_request_init(struct adns_request *r, struct adns_resolver *v)
{
	memset(r, 0, sizeof(*r));

	r->owner = v;
	runq_task_init(&r->task, v->runq);
	r->flags = 0;
}

void adns_request_destroy(struct adns_request *r)
{
	if (r->result)
		freeaddrinfo(r->result);
}

void adns_clear_result(struct adns_request *r)
{
	if (r->result) {
		freeaddrinfo(r->result);
		r->result = NULL;
	}
}

void adns_request_ask(struct adns_request *r,
		      const char *hostname, const char *service,
		      const struct addrinfo *hints,
		      adns_request_func_t func)
{
	int was_empty;

	adns_clear_result(r);

	r->hostname = hostname;
	r->service = service;
	r->hints = hints;
	r->task.func = (runq_task_func_t)func;
	r->flags = REQUEST_BUSY;

	thr_mutex_lock(&r->owner->lock);
	was_empty = list_is_empty(&r->owner->requests);
	list_insert(&r->list, &r->owner->requests);
	thr_mutex_unlock(&r->owner->lock);

	if (was_empty)
		thr_event_raise(&r->owner->notify);
}

void adns_request_cancel(struct adns_request *r)
{
	int was_busy = 0;

	thr_mutex_lock(&r->owner->lock);
	if (r->flags & REQUEST_BUSY) {
		was_busy = 1;
		r->flags = 0;
		list_remove(&r->list);
	}
	thr_mutex_unlock(&r->owner->lock);

	if (was_busy)
		runq_task_exec(&r->task, r->task.func);
}
