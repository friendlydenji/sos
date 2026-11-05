CC := gcc
CFLAGS := -Wall -Wextra -O2

.PHONY: all buffered_io clean

all: buffered_io

buffered_io: buffered_io.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) buffered_io