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
#include <stdlib.h>

#include "hash.h"

void hash_init(struct hash *h, hash_func_t f, hash_compare_t cmp)
{
	memset(h, 0, sizeof(*h));
	h->func = f;
	h->compare = cmp;
}

void hash_destroy(struct hash *h)
{
	if (h->table)
		free(h->table);

	memset(h, 0, sizeof(*h));
}

int hash_capacity_hint(struct hash *h, unsigned int hint)
{
	struct hash_node **new_table;
	unsigned int i;
	unsigned int new_size = 32;

	if (hint < h->count)
		return 0;

	while (new_size < hint)
		new_size <<= 1;

	for (;;) {
		int d;

		for (d = 2; d * d <= new_size; d++)
			if (!(new_size % d))
				goto not_prime;

		break;
not_prime:
		new_size++;
	}

	if (h->size == new_size)
		return 0;

	new_table = malloc(sizeof(new_table[0]) * new_size);
	if (!new_table)
		return -1;

	memset(new_table, 0, sizeof(new_table[0]) * new_size);

	for (i = 0; i < h->size; i++) {
		struct hash_node *n = h->table[i];

		while (n) {
			struct hash_node *next = n->next;
			unsigned int new_index = n->code % new_size;

			n->next = new_table[new_index];
			new_table[new_index] = n;

			n = next;
		}
	}

	if (h->table)
		free(h->table);

	h->table = new_table;
	h->size = new_size;

	return 0;
}

struct hash_node *hash_find(const struct hash *h, const void *key)
{
	if (h->size) {
		const hash_code_t code = h->func(key);
		const unsigned int index = code % h->size;
		struct hash_node *n = h->table[index];

		while (n) {
			if (!h->compare(key, n))
				return n;

			n = n->next;
		}
	}

	return NULL;
}

int hash_insert(struct hash *h, const void *key, struct hash_node *ins,
		struct hash_node **old_ret, int flags)
{
	unsigned int index;
	struct hash_node *n;
	struct hash_node *old = NULL;

	if (h->count >= h->size &&
	    hash_capacity_hint(h, h->count + 1) < 0)
		return -1;

	ins->next = NULL;

	if (!(flags & HASH_INSERT_PREHASHED))
		ins->code = h->func(key);

	index = ins->code % h->size;

	if (flags & HASH_INSERT_UNIQUE) {
		ins->next = h->table[index];
		h->table[index] = ins;
		h->count++;
		return 0;
	}

	n = h->table[index];

	while (n) {
		struct hash_node *next = n->next;

		if (h->compare(key, n)) {
			n->next = ins;
			ins = n;
		} else {
			old = n;
		}

		n = next;
	}

	h->table[index] = ins;
	if (!old)
		h->count++;

	if (old_ret)
		*old_ret = old;

	return 0;
}

void hash_remove(struct hash *h, struct hash_node *n)
{
	const unsigned int index = n->code % h->size;

	if (h->table[index] == n) {
		h->table[index] = n->next;
		h->count--;
	} else {
		struct hash_node *s = h->table[index];

		while (s && s->next != n)
			s = s->next;

		if (s) {
			s->next = n->next;
			h->count--;
		}
	}

	if (h->count * 4 < h->size)
		hash_capacity_hint(h, h->count);
}
