#!/bin/bash

# Public Domain.

CFLAGS="-Wall  -Wextra  -std=gnu11 -Wno-unused-variable -Wno-parentheses"

gcc -o parser_gen parser_gen.c ../sti.c \
	-lm \
	$CFLAGS \
	-O0 -ggdb -DLINUX -std=gnu11

