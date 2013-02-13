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

#include "mailbox.h"

void mailbox_init(struct mailbox *m, struct runq *q)
{
	runq_task_init(&m->task, q);
	thr_mutex_init(&m->lock);
	m->state = 0;
	m->expected = 0;
	m->mode = MAILBOX_WAIT_NONE;
}

void mailbox_destroy(struct mailbox *m)
{
	thr_mutex_destroy(&m->lock);
}

mailbox_flags_t mailbox_take(struct mailbox *m, mailbox_flags_t clear_mask)
{
	mailbox_flags_t ret;

	thr_mutex_lock(&m->lock);
	ret = m->state;
	m->state &= ~clear_mask;
	thr_mutex_unlock(&m->lock);

	return ret;
}

void mailbox_raise(struct mailbox *m, mailbox_flags_t set_mask)
{
	int fire = 0;

	thr_mutex_lock(&m->lock);
	m->state |= set_mask;

	switch (m->mode) {
	case MAILBOX_WAIT_NONE:
		break;

	case MAILBOX_WAIT_ANY:
		if (m->expected & m->state)
			fire = 1;
		break;

	case MAILBOX_WAIT_ALL:
		if ((m->expected & m->state) == m->expected)
			fire = 1;
		break;
	}

	if (fire)
		m->mode = MAILBOX_WAIT_NONE;
	thr_mutex_unlock(&m->lock);

	if (fire)
		runq_task_exec(&m->task, m->task.func);
}

void mailbox_wait(struct mailbox *m, mailbox_flags_t set, mailbox_func_t cb)
{
	int fire = 0;

	thr_mutex_lock(&m->lock);
	m->task.func = (runq_task_func_t)cb;
	m->expected = set;

	if (m->state & set)
		fire = 1;
	else
		m->mode = MAILBOX_WAIT_ANY;
	thr_mutex_unlock(&m->lock);

	if (fire)
		runq_task_exec(&m->task, m->task.func);
}

void mailbox_wait_all(struct mailbox *m, mailbox_flags_t set,
		      mailbox_func_t cb)
{
	int fire = 0;

	thr_mutex_lock(&m->lock);
	m->task.func = (runq_task_func_t)cb;
	m->expected = set;

	if ((m->state & set) == set)
		fire = 1;
	else
		m->mode = MAILBOX_WAIT_ALL;
	thr_mutex_unlock(&m->lock);

	if (fire)
		runq_task_exec(&m->task, m->task.func);
}
