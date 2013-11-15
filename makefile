CFLAGS=-O3 -std=c99 -Wall -g

crc_rev: crc_rev.c

.PHONY: test
test:
	make -C test

.PHONY: clean
clean:
	rm crc_rev
