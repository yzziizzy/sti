#!/bin/bash

# Public Domain.

CFLAGS="-Wall -Werror -Wextra -pedantic"

gcc -o parser_gen parser_gen.c ../sti.c \
	-lm \
	$CFLAGS \
	-O0 -ggdb -DLINUX -std=gnu11

