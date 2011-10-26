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

struct index_key {
	const char	*text;
	unsigned int	length;
};

static hash_code_t key_hash(const void *k)
{
	const struct index_key *ik = (const struct index_key *)k;
	hash_code_t code = 0;
	int i;

	for (i = 0; i < ik->length; i++)
		code = code * 33 + ik->text[i];

	return code;
}

static int key_compare(const void *k, const struct hash_node *n)
{
	const struct index_key *ik = (const struct index_key *)k;
	const struct istr_desc *d = (const struct istr_desc *)n;

	if (ik->length != d->length)
		return 1;

	return memcmp(d->owner->text.text + d->offset, ik->text, ik->length);
}

void istr_pool_init(struct istr_pool *p)
{
	strbuf_init(&p->text);
	hash_init(&p->index, key_hash, key_compare);
	slab_init(&p->descs, sizeof(struct istr_desc));

	p->gc_threshold = 128;
}

void istr_pool_destroy(struct istr_pool *p)
{
	strbuf_destroy(&p->text);
	hash_destroy(&p->index);
	slab_free_all(&p->descs);
}

istr_t istr_pool_alloc(struct istr_pool *p, const char *text, int length)
{
	struct index_key ik;
	struct hash_node *n;
	struct istr_desc *d;

	if (length < 0)
		length = strlen(text);

	if (p->index.count >= p->gc_threshold) {
		istr_pool_gc(p);
		p->gc_threshold = p->index.count * 4;
		if (p->gc_threshold < 128)
			p->gc_threshold = 128;
	}

	ik.text = text;
	ik.length = length;
	n = hash_find(&p->index, &ik);

	if (n) {
		d = (struct istr_desc *)n;

		d->refcnt++;
		return d;
	}

	d = slab_alloc(&p->descs);
	if (!d)
		return NULL;

	d->owner = p;
	d->offset = p->text.length;
	d->length = length;
	d->refcnt = 1;

	if (strbuf_add_string(&p->text, text, length) < 0 ||
	    strbuf_add_char(&p->text, 0) < 0) {
		slab_free(&p->descs, d);
		return NULL;
	}

	if (hash_insert(&p->index, &ik, &d->node,
			NULL, HASH_INSERT_UNIQUE) < 0) {
		slab_free(&p->descs, d);
		return NULL;
	}

	return d;
}

static int gc_index(struct istr_pool *p)
{
	struct hash new_index;
	int i;

	hash_init(&new_index, key_hash, key_compare);

	if (hash_capacity_hint(&new_index, p->index.count) < 0) {
		hash_destroy(&new_index);
		return -1;
	}

	for (i = 0; i < p->index.size; i++) {
		struct hash_node *n = p->index.table[i];

		while (n) {
			struct hash_node *next = n->next;
			struct istr_desc *d = (struct istr_desc *)n;

			if (d->refcnt)
				hash_insert(&new_index, NULL, n, NULL,
					    HASH_INSERT_PREHASHED |
					    HASH_INSERT_UNIQUE);
			else
				slab_free(&p->descs, d);

			n = next;
		}
	}

	hash_capacity_hint(&new_index, new_index.count);
	hash_destroy(&p->index);
	memcpy(&p->index, &new_index, sizeof(p->index));

	return 0;
}

static int gc_text(struct istr_pool *p)
{
	struct strbuf new_buf;
	int i;

	strbuf_init(&new_buf);

	if (strbuf_capacity_hint(&new_buf, p->text.length) < 0) {
		strbuf_destroy(&new_buf);
		return -1;
	}

	for (i = 0; i < p->index.size; i++) {
		struct hash_node *n;

		for (n = p->index.table[i]; n; n = n->next) {
			struct istr_desc *d = (struct istr_desc *)n;
			int new_offset = new_buf.length;

			strbuf_add_string(&new_buf, istr_text(d),
					  d->length + 1);
			d->offset = new_offset;
		}
	}

	strbuf_capacity_hint(&new_buf, new_buf.length);
	strbuf_destroy(&p->text);
	memcpy(&p->text, &new_buf, sizeof(p->text));

	return 0;
}

int istr_pool_gc(struct istr_pool *p)
{
	if (gc_index(p) < 0 || gc_text(p) < 0)
		return -1;

	return 0;
}

int istr_equal(istr_t a, istr_t b)
{
	if (a->owner == b->owner)
		return istr_eq(a, b);

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
