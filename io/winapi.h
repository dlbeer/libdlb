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

#ifndef IO_WINAPI_H_
#define IO_WINAPI_H_

/* Workaround for buggy windows.h header */
#ifdef __Windows__

/* MinGW doesn't define this correctly, which means we end up with some
 * functions hidden.
 */
#if _WIN32_WINNT < 0x0501
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

/* MinGW's windows.h includes winsock.h if we don't define certain
 * guards. This prevents us from later including winsock2.h and
 * ws2tcpip.h.
 */
#define __MSYS__
#include <windows.h>
#undef __MSYS__

#endif

#endif
