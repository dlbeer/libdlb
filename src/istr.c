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

#include <string.h>
#include "istr.h"

void istr_pool_init(struct istr_pool *p)
{
	strbuf_init(&p->text);
	slab_init(&p->descs, sizeof(struct istr_desc));

	p->all = NULL;
	p->desc_count = 0;
	p->gc_threshold = 128;
}

void istr_pool_destroy(struct istr_pool *p)
{
	strbuf_destroy(&p->text);
	slab_free_all(&p->descs);
}

istr_t istr_pool_alloc(struct istr_pool *p, const char *text, int length)
{
	struct istr_desc *d;

	if (length < 0)
		length = strlen(text);

	/* Allocate a descriptor */
	d = slab_alloc(&p->descs);
	if (!d)
		return NULL;

	d->owner = p;
	d->offset = strbuf_len(&p->text);
	d->length = length;
	d->refcnt = 1;

	/* Allocate string data */
	if (strbuf_add_string(&p->text, text, length) < 0 ||
	    strbuf_add_char(&p->text, 0) < 0) {
		slab_free(&p->descs, d);
		return NULL;
	}

	/* Add the descriptor to the list */
	p->desc_count++;
	d->next = p->all;
	p->all = d;

	/* Give the garbage collector a chance to run */
	if (p->desc_count >= p->gc_threshold) {
		istr_pool_gc(p);

		p->gc_threshold = p->desc_count * 4;
		if (p->gc_threshold < 128)
			p->gc_threshold = 128;
	}

	return d;
}

static void gc_desc(struct istr_pool *p)
{
	struct istr_desc *new_list = NULL;

	/* Filter the descriptor list, throwing away those with no
	 * references.
	 */
	while (p->all) {
		struct istr_desc *d = p->all;

		p->all = d->next;

		if (d->refcnt) {
			d->next = new_list;
			new_list = d;
		} else {
			slab_free(&p->descs, d);
		}
	}

	/* Reverse and count the filtered list */
	p->desc_count = 0;
	while (new_list) {
		struct istr_desc *d = new_list;

		new_list = d->next;

		d->next = p->all;
		p->all = d;

		p->desc_count++;
	}
}

static int gc_text(struct istr_pool *p)
{
	struct istr_desc *d;
	struct strbuf new_buf;

	strbuf_init(&new_buf);

	if (strbuf_capacity_hint(&new_buf, p->text.length) < 0) {
		strbuf_destroy(&new_buf);
		return -1;
	}

	for (d = p->all; d; d = d->next) {
		int new_offset = new_buf.length;

		strbuf_add_string(&new_buf, istr_text(d),
				  d->length + 1);
		d->offset = new_offset;
	}

	strbuf_capacity_hint(&new_buf, new_buf.length);
	strbuf_destroy(&p->text);
	memcpy(&p->text, &new_buf, sizeof(p->text));

	return 0;
}

int istr_pool_gc(struct istr_pool *p)
{
	gc_desc(p);

	return gc_text(p);
}

int istr_equal(istr_t a, istr_t b)
{
	if (a->length != b->length)
		return 0;

	return !memcmp(istr_text(a), istr_text(b), a->length);
}

int istr_compare(istr_t a, istr_t b)
{
	const char *ta = istr_text(a);
	const char *tb = istr_text(b);
	const int la = istr_length(a);
	const int lb = istr_length(b);

	if (ta == tb)
		return 0;

	if (la < lb) {
		int r = memcmp(ta, tb, la);

		if (!r)
			return -1;

		return r;
	}

	if (la > lb) {
		int r = memcmp(ta, tb, lb);

		if (!r)
			return 1;

		return r;
	}

	return memcmp(ta, tb, la);
}
