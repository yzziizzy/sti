#!/bin/bash

# Public Domain.

CFLAGS="-Wall  -Wextra "

gcc -o ass assembler.c ../sti.c \
	-lm \
	$CFLAGS \
	-O0 -ggdb -DLINUX -std=gnu11

