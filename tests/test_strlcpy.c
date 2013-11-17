/* strlcpy - safe string copy
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
#include <assert.h>
#include "strlcpy.h"

int main(void)
{
	char buf[16];
	int i;

	buf[0] = 0;
	i = strlcpy(buf, "Foo", sizeof(buf));
	assert(i == 3);

	buf[0] = 0;
	i = strlcpy(buf, "This is a long string", sizeof(buf));
	assert(i >= sizeof(buf));
	assert(!strcmp(buf, "This is a long "));

	buf[0] = 0;
	i = strlcat(buf, "This is", sizeof(buf));
	assert(i == 7);
	assert(!strcmp(buf, "This is"));

	i = strlcat(buf, " a ", sizeof(buf));
	assert(i == 10);
	assert(!strcmp(buf, "This is a "));

	i = strlcat(buf, "long string", sizeof(buf));
	assert(i >= sizeof(buf));
	assert(!strcmp(buf, "This is a long "));

	return 0;
}
