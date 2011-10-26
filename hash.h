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

#ifndef HASH_H_
#define HASH_H_

/* This is a dynamically sized hash table. To implement a hash table for
 * a particular type, you must specify and hash and a compare function.
 *
 * The hash function should, when given a pointer to a node key, return
 * a value of type hash_code_t. For best results, the full range of the
 * type should be used.
 *
 * The comparison function should return 0 if the given key matches the
 * given hash_node. Otherwise, it should return non-zero.
 */
typedef unsigned int hash_code_t;

struct hash_node {
	hash_code_t		code;
	struct hash_node	*next;
};

typedef hash_code_t (*hash_func_t)(const void *key);
typedef int (*hash_compare_t)(const void *key, const struct hash_node *n);

/* A hash table should be initialized by calling hash_init with a pointer
 * to hashing and comparison functions. hash_destroy() must be called when
 * the hash table is no longer necessary to free memory used by the table
 * index.
 */
struct hash {
	hash_func_t		func;
	hash_compare_t		compare;
	unsigned int		size;
	unsigned int		count;
	struct hash_node	**table;
};

void hash_init(struct hash *h, hash_func_t f, hash_compare_t cmp);
void hash_destroy(struct hash *h);

/* Search for a key in a hash table. Returns a pointer to the given hash
 * node if found, or NULL if the key doesn't exist.
 */
struct hash_node *hash_find(const struct hash *h, const void *key);

/* Insert a new item into a hash table. Returns 0 on success or -1 if
 * memory couldn't be allocated. If a node already exists with the given
 * key, it can be optionally returned via the "old" argument.
 *
 * Some flags can be specified to speed up the operation if additional
 * information is known:
 *
 *     HASH_INSERT_PREHASHED: the code field is already computed
 *     HASH_INSERT_UNIQUE: the key is known to be unique
 *
 * Note that if HASH_INSERT_UNIQUE is specified, the old argument is not
 * used. If both flags are specified, then the key field is not required.
 */
#define HASH_INSERT_PREHASHED		0x01
#define HASH_INSERT_UNIQUE		0x02

int hash_insert(struct hash *h, const void *key, struct hash_node *n,
		struct hash_node **old, int flags);

/* Remove an item from a hash table. This function never fails. */
void hash_remove(struct hash *h, struct hash_node *n);

/* The hash table resizes dynamically, but if you know in advance how
 * many items you'll need to store, you can call this function to preallocate
 * memory. Returns 0 on success or -1 if memory couldn't be allocated.
 */
int hash_capacity_hint(struct hash *h, unsigned int hint);

#endif
