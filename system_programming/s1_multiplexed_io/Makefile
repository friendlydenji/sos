CC := gcc
CFLAGS := -Wall -Wextra -O2

.PHONY: all buffered_io clean

all: server client

server: server.c
	$(CC) $(CFLAGS) -o $@ $<

client: client.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) server client