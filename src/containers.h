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

#ifndef CONTAINERS_H_
#define CONTAINERS_H_

/* Return the number of elements in a statically allocated array. */
#define lengthof(a) (sizeof(a) / sizeof((a)[0]))

/* Find the offset of a struct's member relative to that struct. */
#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif

/* Given a pointer (ptr) to a member of a struct, produce a pointer
 * to the containing struct.
 */
#define container_of(ptr, type, member) \
        ((type *)((char *)(ptr) - offsetof(type, member)))

#endif
