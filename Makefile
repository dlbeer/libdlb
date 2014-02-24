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

ifeq ($(OS),Windows_NT)
    OS_CFLAGS = -D__Windows__
    TEST = .exe
    LIB_NET = -lws2_32
else
    TEST = .test
    LIB_RT = -lrt
    LIB_PTHREAD = -lpthread
endif

TESTS = \
    tests/containers$(TEST) \
    tests/protothread$(TEST) \
    tests/bytes$(TEST) \
    tests/list$(TEST) \
    tests/vector$(TEST) \
    tests/rbt$(TEST) \
    tests/rbt_iter$(TEST) \
    tests/rbt_range$(TEST) \
    tests/slab$(TEST) \
    tests/hash$(TEST) \
    tests/strbuf$(TEST) \
    tests/istr$(TEST) \
    tests/bint$(TEST) \
    tests/slist$(TEST) \
    tests/cbuf$(TEST) \
    tests/strlcpy$(TEST) \
    tests/arena$(TEST) \
    tests/syserr$(TEST) \
    tests/clock$(TEST) \
    tests/thr$(TEST) \
    tests/runq$(TEST) \
    tests/waitq$(TEST) \
    tests/ioq$(TEST) \
    tests/mailbox$(TEST) \
    tests/afile$(TEST) \
    tests/net$(TEST) \
    tests/adns$(TEST) \
    tests/asock$(TEST)

CFLAGS = -O1 -Wall -ggdb -Isrc -Iio -Inet $(OS_CFLAGS)
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
	rm -f tests/*$(TEST)

tests/containers$(TEST): tests/test_containers.o
	$(CC) -o $@ $^

tests/protothread$(TEST): tests/test_protothread.o
	$(CC) -o $@ $^

tests/bytes$(TEST): tests/test_bytes.o
	$(CC) -o $@ $^

tests/list$(TEST): tests/test_list.o src/list.o
	$(CC) -o $@ $^

tests/vector$(TEST): tests/test_vector.o src/vector.o
	$(CC) -o $@ $^

tests/rbt$(TEST): tests/test_rbt.o src/rbt.o
	$(CC) -o $@ $^

tests/rbt_iter$(TEST): tests/test_rbt_iter.o src/rbt.o src/rbt_iter.o
	$(CC) -o $@ $^

tests/rbt_range$(TEST): tests/test_rbt_range.o src/rbt.o src/rbt_range.o
	$(CC) -o $@ $^

tests/slab$(TEST): tests/test_slab.o src/slab.o src/list.o
	$(CC) -o $@ $^

tests/hash$(TEST): tests/test_hash.o src/hash.o
	$(CC) -o $@ $^

tests/strbuf$(TEST): tests/test_strbuf.o src/strbuf.o
	$(CC) -o $@ $^

tests/istr$(TEST): tests/test_istr.o src/istr.o src/strbuf.o src/hash.o \
		 src/slab.o src/list.o
	$(CC) -o $@ $^

tests/bint$(TEST): tests/test_bint.o src/bint.o
	$(CC) -o $@ $^

tests/slist$(TEST): tests/test_slist.o src/slist.o
	$(CC) -o $@ $^

tests/cbuf$(TEST): tests/test_cbuf.o src/cbuf.o
	$(CC) -o $@ $^

tests/strlcpy$(TEST): tests/test_strlcpy.o src/strlcpy.o
	$(CC) -o $@ $^

tests/arena$(TEST): tests/test_arena.o src/arena.o
	$(CC) -o $@ $^

tests/syserr$(TEST): tests/test_syserr.o
	$(CC) -o $@ $^

tests/clock$(TEST): tests/test_clock.o io/clock.o
	$(CC) -o $@ $^ $(LIB_RT)

tests/thr$(TEST): tests/test_thr.o io/thr.o io/clock.o
	$(CC) -o $@ $^ $(LIB_PTHREAD) $(LIB_RT)

tests/runq$(TEST): tests/test_runq.o io/runq.o io/thr.o \
		    src/slist.o io/clock.o
	$(CC) -o $@ $^ $(LIB_PTHREAD) $(LIB_RT)

tests/waitq$(TEST): tests/test_waitq.o io/waitq.o io/runq.o \
		  io/thr.o io/clock.o src/slist.o src/rbt.o \
		  src/rbt_iter.o
	$(CC) -o $@ $^ $(LIB_PTHREAD) $(LIB_RT)

tests/ioq$(TEST): tests/test_ioq.o io/ioq.o io/waitq.o \
		io/runq.o io/thr.o io/clock.o src/slist.o \
		src/rbt.o src/rbt_iter.o
	$(CC) -o $@ $^ $(LIB_PTHREAD) $(LIB_RT)

tests/afile$(TEST): tests/test_afile.o io/ioq.o io/waitq.o \
		  io/runq.o io/thr.o io/clock.o src/slist.o \
		  src/rbt.o src/rbt_iter.o io/afile.o
	$(CC) -o $@ $^ $(LIB_PTHREAD) $(LIB_RT)

tests/mailbox$(TEST): tests/test_mailbox.o io/mailbox.o io/runq.o \
		    io/thr.o src/slist.o
	$(CC) -o $@ $^ $(LIB_PTHREAD)

tests/net$(TEST): tests/test_net.o io/net.o
	$(CC) -o $@ $^ $(LIB_NET)

tests/adns$(TEST): tests/test_adns.o io/adns.o io/runq.o src/list.o \
		   src/slist.o io/thr.o io/net.o
	$(CC) -o $@ $^ $(LIB_PTHREAD) $(LIB_NET)

tests/asock$(TEST): tests/test_asock.o io/ioq.o io/waitq.o \
		    io/runq.o io/thr.o io/clock.o src/slist.o \
		    src/rbt.o src/rbt_iter.o io/asock.o io/net.o
	$(CC) -o $@ $^ $(LIB_PTHREAD) $(LIB_RT) $(LIB_NET)

%.o: %.c
	$(CC) $(CFLAGS) -o $*.o -c $*.c
