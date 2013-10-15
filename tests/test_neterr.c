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
#include <string.h>
#include "neterr.h"

#ifndef __Windows__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

int main(void)
{
	struct sockaddr_in bad;
	struct neterr err;
	char buf[128];

	memset(&bad, 0, sizeof(bad));
	connect(-1, (struct sockaddr *)&bad, sizeof(bad));

	neterr_get(&err);
	neterr_format(&err, buf, sizeof(buf));
	puts(buf);

	return 0;
}
