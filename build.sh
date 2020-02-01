#!/bin/bash

# Public Domain.

CFLAGS="-Wall -Werror -Wextra -pedantic"

gcc -o sti_test test.c sti.c \
	$CFLAGS \
	-O0 -ggdb -DLINUX -std=gnu11
	
# gcc -o parser_gen parser_gen.c sti.c \
# 	$CFLAGS \
# 	-O0 -ggdb -DLINUX -std=gnu11
