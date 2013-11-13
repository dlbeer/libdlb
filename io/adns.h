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

#ifndef IO_ADNS_H_
#define IO_ADNS_H_

#include "net.h"
#include "list.h"
#include "thr.h"
#include "runq.h"

#ifndef __Windows__
#include <netdb.h>
#endif

/* Asynchronous DNS resolver. One resolver can be shared by multiple
 * threads, but each must have its own adns_request object.
 */
struct adns_resolver {
	struct runq		*runq;

	thr_thread_t		worker;
	thr_event_t		notify;

	thr_mutex_t		lock;
	int			flags;
	struct list_node	requests;
};

/* Initialize/destroy an asynchronous DNS resolver.
 *
 * NOTE: destruction of the resolver may block if the resolver is busy.
 * This can't be avoided, so you should try to defer destruction for as
 * long as possible.
 *
 * If initialization fails, syserr_last() will return a valid error
 * code.
 */
int adns_resolver_init(struct adns_resolver *r, struct runq *q);
void adns_resolver_destroy(struct adns_resolver *r);

/* DNS error codes */
#ifdef __Windows__
typedef DWORD adns_error_t;

static inline void adns_error_format(adns_error_t e,
				     char *buf, size_t max_size)
{
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, e, 0,
		      buf, max_size, NULL);
}
#else
typedef int adns_error_t;

static inline void adns_error_format(adns_error_t e,
				     char *buf, size_t max_size)
{
	strncpy(buf, gai_strerror(e), max_size);
	buf[max_size - 1] = 0;
}
#endif

#define ADNS_ERROR_NONE		0

/* Asynchronous DNS request. This must not be modified simultaneously by
 * multiple threads.
 */
struct adns_request {
	/* Callback -- must be first */
	struct runq_task	task;
	struct adns_resolver	*owner;

	/* State: protected by owner lock */
	struct list_node	list;
	int			flags;

	/* Request */
	const char		*hostname;
	const char		*service;
	const struct addrinfo	*hints;

	/* Result */
	struct addrinfo		*result;
	adns_error_t		error;
};

/* Initialize a request object */
void adns_request_init(struct adns_request *r, struct adns_resolver *v);

/* Free any non-NULL result */
void adns_request_destroy(struct adns_request *r);

typedef void (*adns_request_func_t)(struct adns_request *r);

/* Begin an asynchronous DNS request. Arguments are the same as for
 * getaddrinfo(), except a callback is given instead of a return
 * pointer.
 *
 * Any result left over from a previous query will be freed first.
 */
void adns_request_ask(struct adns_request *r,
		      const char *hostname, const char *service,
		      const struct addrinfo *hints,
		      adns_request_func_t func);

/* Cancel a request. A cancelled request completes with a non-error code
 * and a NULL result.
 */
void adns_request_cancel(struct adns_request *r);

/* Retrieve the result. If non-NULL, you must free this with
 * freeaddrinfo().
 */
static inline struct addrinfo *adns_get_result(const struct adns_request *r)
{
	return r->result;
}

/* Retrieve the error */
static inline adns_error_t adns_get_error(const struct adns_request *r)
{
	return r->error;
}

/* Clear the result, if any */
void adns_clear_result(struct adns_request *r);

#endif
