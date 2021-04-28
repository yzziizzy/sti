#!/bin/bash

# Public Domain.

DISABLE_DUMB_WARNINGS="\
	-Wno-switch \
	-Wno-unused-but-set-variable \
	-Wno-maybe-uninitialized \
	-Wno-unused-variable \
	"

CFLAGS="-Wall "

gcc -o arith arithmetic.c ../vec.c ../hash.c ../rpn.c ../hash_fns/MurmurHash3.c \
	-lm \
	$CFLAGS $DISABLE_DUMB_WARNINGS \
	-O0 -ggdb -DLINUX -std=gnu11

