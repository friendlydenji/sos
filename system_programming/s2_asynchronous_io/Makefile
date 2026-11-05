CC := gcc
CFLAGS := -Wall -Wextra -O2

.PHONY: all aio_gnu worker_thread_pool libaio_native liburing_native clean

all: aio_gnu worker_thread_pool libaio_native liburing_native 

aio_gnu: aio.c
	$(CC) $(CFLAGS) -o $@ $<

worker_thread_pool: worker_thread_pool.c
	$(CC) $(CFLAGS) -o $@ $<

libaio_native: libaio_native.c
	$(CC) $(CFLAGS) -o $@ $<

liburing_native: liburing_native.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) aio_gnu worker_thread_pool libaio_native liburing_native