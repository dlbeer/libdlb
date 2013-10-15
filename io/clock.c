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

#include "clock.h"

#ifdef __Windows__
#include "winapi.h"

clock_ticks_t clock_now(void)
{
#if _WIN32_WINNT >= 0x0600
	return GetTickCount64();
#else
	return GetTickCount();
#endif
}

void clock_wait(clock_ticks_t delay)
{
	Sleep(delay);
}
#else
#include <time.h>
#include <unistd.h>

clock_ticks_t clock_now(void)
{
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (((clock_ticks_t)tp.tv_sec) * 1000LL) +
	       ((clock_ticks_t)(tp.tv_nsec / 1000000LL));
}

void clock_wait(clock_ticks_t delay)
{
	if (delay >= 1000) {
		sleep(delay / 1000);
		delay %= 1000;
	}

	usleep(delay * 1000);
}
#endif
