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
#include "strlcpy.h"

size_t strlcpy(char *dst, const char *src, size_t size)
{
	const size_t src_len = strlen(src);
	const size_t max_len = size - 1;
	const size_t len = src_len > max_len ? max_len : src_len;

	if (!size)
		return 0;

	memcpy(dst, src, len);
	dst[len] = 0;

	return src_len;
}

size_t strlcat(char *dst, const char *src, size_t size)
{
	const size_t dst_len = strlen(dst);
	const size_t src_len = strlen(src);
	const size_t res_len = dst_len + src_len;
	const size_t max_len = size - dst_len - 1;
	const size_t copy = src_len > max_len ? max_len : src_len;

	if (!size || (size - 1) < dst_len)
		return 0;

	memcpy(dst + dst_len, src, copy);
	dst[dst_len + copy] = 0;

	return res_len;
}
