# libdlb - data structures and utilities library
# Copyright (C) 2011 Daniel Beer <dlbeer@gmail.com>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

TESTS = \
    tests/containers.test \
    tests/list.test \
    tests/vector.test \
    tests/rbt.test \
    tests/rbt_iter.test
CFLAGS = -O1 -Wall -ggdb -I.
CC = gcc

all: $(TESTS)

test: $(TESTS)
	@@for x in $(TESTS); \
	do \
	    echo $$x; \
	    ./$$x > /dev/null || exit 255; \
	done

clean:
	rm -f *.o
	rm -f tests/*.o
	rm -f tests/*.test

tests/containers.test: tests/test_containers.o
	$(CC) -o $@ $^

tests/list.test: tests/test_list.o list.o
	$(CC) -o $@ $^

tests/vector.test: tests/test_vector.o vector.o
	$(CC) -o $@ $^

tests/rbt.test: tests/test_rbt.o rbt.o
	$(CC) -o $@ $^

tests/rbt_iter.test: tests/test_rbt_iter.o rbt.o rbt_iter.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $*.o -c $*.c
