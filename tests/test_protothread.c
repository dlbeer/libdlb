/* libdlb - data structures and utilities library
 * Copyright (C) 2011 Daniel Beer <dlbeer@gmail.com>
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
#include "protothread.h"
#include "containers.h"

static int produce(void)
{
	static protothread_state_t state;
	static int i;

	PROTOTHREAD_BEGIN(state);

	for (i = 0; i < 5; i++)
		PROTOTHREAD_YIELD(state, i);

	PROTOTHREAD_YIELD(state, i);
	PROTOTHREAD_YIELD(state, i);

	for (; i >= 0; i--)
		PROTOTHREAD_YIELD(state, i);

	PROTOTHREAD_END;
	return 0;
}

int main(void)
{
	static const int expect[] = {
		0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0
	};
	int i;

	for (i = 0; i < lengthof(expect); i++) {
		const int v = produce();

		printf("%d\n", v);
		assert(v == expect[i]);
	}

	return 0;
}
