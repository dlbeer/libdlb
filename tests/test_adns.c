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
#include "adns.h"
#include "net.h"

static int count;

const struct addrinfo hints = {
	.ai_family		= AF_INET
};

static void handler(struct adns_request *r)
{
	struct addrinfo *inf = adns_get_result(r);

	printf("%s => ", r->hostname);
	if (inf) {
		const struct sockaddr_in *in =
			(const struct sockaddr_in *)inf->ai_addr;

		printf("%s\n", inet_ntoa(in->sin_addr));
	} else {
		char buf[128];

		adns_error_format(adns_get_error(r), buf, sizeof(buf));
		puts(buf);
	}

	count--;
	adns_request_destroy(r);
}

int main(int argc, char **argv)
{
	struct runq run;
	struct adns_resolver res;
	struct adns_request reqs[argc];
	int i;

	i = net_start();
	assert(i == NETERR_NONE);

	i = runq_init(&run, 0);
	assert(i >= 0);

	i = adns_resolver_init(&res, &run);
	assert(i >= 0);

	for (i = 0; i < argc; i++)
		adns_request_init(&reqs[i], &res);

	adns_request_ask(&reqs[0], "localhost", NULL, &hints, handler);
	for (i = 1; i < argc; i++)
		adns_request_ask(&reqs[i], argv[i], NULL, &hints, handler);
	count = argc;

	while (count)
		runq_dispatch(&run, 0);

	adns_resolver_destroy(&res);
	runq_destroy(&run);
	net_stop();

	return 0;
}
