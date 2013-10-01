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
    tests/rbt_iter.test \
    tests/rbt_range.test \
    tests/slab.test \
    tests/hash.test \
    tests/strbuf.test \
    tests/istr.test \
    tests/bint.test \
    tests/slist.test \
    tests/cbuf.test \
    tests/syserr.test \
    tests/clock.test \
    tests/thr.test \
    tests/runq.test \
    tests/waitq.test \
    tests/ioq.test \
    tests/mailbox.test \
    tests/afile.test
CFLAGS = -O1 -Wall -ggdb -Isrc -Iio
CC = gcc

all: $(TESTS)

test: $(TESTS)
	@@for x in $(TESTS); \
	do \
	    echo $$x; \
	    ./$$x > /dev/null || exit 255; \
	done

clean:
	rm -f */*.o
	rm -f tests/*.test

tests/containers.test: tests/test_containers.o
	$(CC) -o $@ $^

tests/list.test: tests/test_list.o src/list.o
	$(CC) -o $@ $^

tests/vector.test: tests/test_vector.o src/vector.o
	$(CC) -o $@ $^

tests/rbt.test: tests/test_rbt.o src/rbt.o
	$(CC) -o $@ $^

tests/rbt_iter.test: tests/test_rbt_iter.o src/rbt.o src/rbt_iter.o
	$(CC) -o $@ $^

tests/rbt_range.test: tests/test_rbt_range.o src/rbt.o src/rbt_range.o
	$(CC) -o $@ $^

tests/slab.test: tests/test_slab.o src/slab.o src/list.o
	$(CC) -o $@ $^

tests/hash.test: tests/test_hash.o src/hash.o
	$(CC) -o $@ $^

tests/strbuf.test: tests/test_strbuf.o src/strbuf.o
	$(CC) -o $@ $^

tests/istr.test: tests/test_istr.o src/istr.o src/strbuf.o src/hash.o \
		 src/slab.o src/list.o
	$(CC) -o $@ $^

tests/bint.test: tests/test_bint.o src/bint.o
	$(CC) -o $@ $^

tests/slist.test: tests/test_slist.o src/slist.o
	$(CC) -o $@ $^

tests/cbuf.test: tests/test_cbuf.o src/cbuf.o
	$(CC) -o $@ $^

tests/syserr.test: tests/test_syserr.o
	$(CC) -o $@ $^

tests/clock.test: tests/test_clock.o io/clock.o
	$(CC) -o $@ $^ -lrt

tests/thr.test: tests/test_thr.o io/thr.o io/clock.o
	$(CC) -o $@ $^ -lpthread -lrt

tests/runq.test: tests/test_runq.o io/runq.o io/thr.o \
		    src/slist.o io/clock.o
	$(CC) -o $@ $^ -lpthread -lrt

tests/waitq.test: tests/test_waitq.o io/waitq.o io/runq.o \
		  io/thr.o io/clock.o src/slist.o src/rbt.o \
		  src/rbt_iter.o
	$(CC) -o $@ $^ -lpthread -lrt

tests/ioq.test: tests/test_ioq.o io/ioq.o io/waitq.o \
		io/runq.o io/thr.o io/clock.o src/slist.o \
		src/rbt.o src/rbt_iter.o
	$(CC) -o $@ $^ -lpthread -lrt

tests/afile.test: tests/test_afile.o io/ioq.o io/waitq.o \
		  io/runq.o io/thr.o io/clock.o src/slist.o \
		  src/rbt.o src/rbt_iter.o io/afile.o
	$(CC) -o $@ $^ -lpthread -lrt

tests/mailbox.test: tests/test_mailbox.o io/mailbox.o io/runq.o \
		    io/thr.o src/slist.o
	$(CC) -o $@ $^ -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -o $*.o -c $*.c
