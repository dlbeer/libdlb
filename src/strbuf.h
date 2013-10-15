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

#ifndef STRBUF_H_
#define STRBUF_H_

#include <stdarg.h>
#include <stddef.h>

/* This structure is a resizable text buffer. It allows easy construction
 * and editing of dynamically sized strings. Strings kept in the buffer,
 * are always nul terminated (including those of zero length).
 *
 * Errors are reported in two ways: the failing function with return an
 * error code, and the buffer's failed flag is set. This allows easy error
 * handling when constructing long strings with many calls.
 *
 * The user is allowed to modify the string directly, but only the first
 * ``length`` bytes of the text.
 */
struct strbuf {
	char		*text;
	size_t		length;
	size_t		capacity;
	int		failed;
};

/* Initialize and destroy the string buffer. Upon initialization, the
 * string is empty and text points to a constant zero-length C string.
 */
void strbuf_init(struct strbuf *buf);
void strbuf_destroy(struct strbuf *buf);

/* Inlines for obtaining length, text */
static inline const char *strbuf_text(const struct strbuf *buf)
{
	return buf->text;
}

static inline size_t strbuf_len(const struct strbuf *buf)
{
	return buf->length;
}

/* Clear a string, and reset the failed flag. */
void strbuf_clear(struct strbuf *buf);

/* Resize the string. If the new size is smaller than the old size,
 * then the string is truncated. If the new size is larger, the extra
 * characters are uninitialized (but a nul terminator is added at
 * the correct offset).
 *
 * Returns 0 on success or -1 if memory couldn't be allocated (and
 * the failed flag will be set in this case).
 */
int strbuf_resize(struct strbuf *buf, size_t new_length);

/* This function may be called to preallocate memory. If successful,
 * 0 is returned and further calls to increase the string size or
 * append data are guaranteed to succeed, provided they don't overrun
 * the preallocation.
 *
 * If the function fails, -1 is returned (but the failed flag is _not_
 * set).
 */
int strbuf_capacity_hint(struct strbuf *buf, size_t length);

/* Append characters and strings to the buffer. For strings containing
 * embedded nuls, a length may be explicitly specified. Otherwise, if
 * this length is -1, it will be calculated using strlen().
 *
 * On success, add_char will return 0, and add_string will return the
 * number of characters added. On failure, both functions return -1 and
 * set the failed flag.
 */
int strbuf_add_char(struct strbuf *buf, int c);
int strbuf_add_string(struct strbuf *buf, const char *text, int length);

/* Add text to the buffer with a format string. The buffer will be
 * automatically resized as necessary to accommodate the text.
 *
 * Returns the number of characters added on success, or -1 on failure
 * (in which case the failed flag is also set).
 */
int strbuf_printf(struct strbuf *buf, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

int strbuf_vprintf(struct strbuf *buf, const char *fmt, va_list ap);

#endif
