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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "hash.h"

struct record {
	struct hash_node	node;
	char			text[64];
};

#define	N	16384

static struct record words[N];
static struct hash hsh;

static int word_compare(const void *k, const struct hash_node *n)
{
	const struct record *r = (const struct record *)n;
	const char *text = (const char *)k;

	return strcmp(text, r->text);
}

static hash_code_t word_hash(const void *k)
{
	const char *text = (const char *)k;
	hash_code_t code = 0;

	while (*text)
		code = (code * 33) + *(text++);

	return code;
}

static void init_words(void)
{
	int i;

	for (i = 0; i < N; i++)
		snprintf(words[i].text, sizeof(words[i].text), "%x", i);
}

static void add_half(int start, int flags)
{
	int i;

	for (i = start; i < N; i += 2) {
		struct hash_node *old = NULL;
		int r = hash_insert(&hsh, words[i].text, &words[i].node,
				    &old, flags);

		assert(!r);
		assert(!old);
	}
}

static void remove_half(int start)
{
	int i;

	for (i = start; i < N; i += 2)
		hash_remove(&hsh, &words[i].node);
}

static void check_present(int start)
{
	int i;

	for (i = start; i < N; i += 2) {
		struct hash_node *n = hash_find(&hsh, words[i].text);

		assert(n == &words[i].node);
	}
}

static void check_not_present(int start)
{
	int i;

	for (i = start; i < N; i += 2) {
		struct hash_node *n = hash_find(&hsh, words[i].text);

		assert(!n);
	}
}

static void test_add_remove(int flags)
{
	add_half(0, flags);
	assert(hsh.count == N / 2);
	assert(hsh.size >= N / 2);
	check_present(0);
	check_not_present(1);

	hash_capacity_hint(&hsh, N);
	assert(hsh.size >= N);

	add_half(1, flags | HASH_INSERT_UNIQUE);
	assert(hsh.count == N);
	assert(hsh.size >= N);
	check_present(0);
	check_present(1);

	remove_half(0);
	assert(hsh.count == N / 2);
	assert(hsh.size >= N / 2);
	check_not_present(0);
	check_present(1);

	remove_half(1);
	check_not_present(0);
	check_not_present(1);
	assert(hsh.count == 0);
}

int main(void)
{
	init_words();

	hash_init(&hsh, word_hash, word_compare);
	test_add_remove(0);
	test_add_remove(HASH_INSERT_PREHASHED);
	hash_destroy(&hsh);

	return 0;
}
