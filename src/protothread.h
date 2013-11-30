/* Simple protothread implementation
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

#ifndef PROTOTHREAD_H_
#define PROTOTHREAD_H_

/* Protothreads are a scheme for describing state machines using the
 * higher-level constructs provided by C. They by using a feature of the
 * switch statement which allows case labels to be placed in sub-scopes.
 *
 * This idea is described in detail by Simon Tatham here:
 *
 *     http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
 *
 * This particular implementation is slightly different in that it makes
 * the context variable explicit, which suits embedded systems and other
 * environments where you might not want to dynamically allocate memory.
 *
 * To use this system, you must declare a protothread_state_t,
 * initialized to PROTOTHREAD_INIT (== 0). The protothread body must be
 * enclosed in some function and wrapped between the BEGIN/END macros.
 * When control enters the protothread body, execution resumes at the
 * position of the last YIELD (or at the start, if the state is 0).
 *
 * Issuing a YIELD causes the function to return. On the next call, the
 * protothread resumes execution immediately following the YIELD
 * statement.
 *
 * Limitations:
 *
 *    - you can't have two YIELD statements on the same line
 *    - you can't have a YIELD within a switch statement within a
 *      protothread body
 *    - lexically scoped variables don't retain their state during a
 *      YIELD
 */

/* This is the type of the variable which is used to store protothread
 * state for suspend and resume. If your thread is single instance, you
 * might declare a static variable in the protothread function.
 * Otherwise, this might be a member of a context structure.
 */
typedef int protothread_state_t;

/* Initial value for protothread state */
#define PROTOTHREAD_INIT ((protothread_state_t)0)

/* The protothread body should be wrapped with this pair of macros. When
 * the protothread body is entered, execution resumes at the point where
 * the thread last yielded.
 */
#define PROTOTHREAD_BEGIN(state) switch (state) { case PROTOTHREAD_INIT:
#define PROTOTHREAD_END }

/* Suspend the protothread and return. When the protothread body is next
 * entered, execution resumes immediately following the YIELD statement.
 */
#define PROTOTHREAD_YIELD(state, ...) \
    do { state = __LINE__; return __VA_ARGS__; case __LINE__:; } while (0);

#endif
