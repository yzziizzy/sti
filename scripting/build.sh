#!/bin/bash

# Public Domain.


CFLAGS="-Wall "

gcc -o arith arithmetic.c ../vec.c ../hash.c ../rpn.c ../hash_fns/MurmurHash3.c \
	-lm \
	$CFLAGS \
	-O0 -ggdb -DLINUX -std=gnu11

