#!/bin/bash

# Public Domain.

CFLAGS="-Wall -Werror -Wextra -pedantic"
SANE_CFLAGS="-Wno-unused-function" # ...because this is a library...

gcc -o sti_test test.c sti.c \
	-lm \
	$CFLAGS $SANE_CFLAGS \
	-O0 -ggdb -DLINUX -std=gnu11


#gcc -o parser_gen parser_gen.c sti.c \
#	-lm \
#	$CFLAGS \
#	-O0 -ggdb -DLINUX -std=gnu11

