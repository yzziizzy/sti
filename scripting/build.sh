#!/bin/bash

# Public Domain.

DISABLE_DUMB_WARNINGS="\
	-Wno-switch \
	-Wno-unused-but-set-variable \
	-Wno-maybe-uninitialized \
	-Wno-unused-variable \
	-Wno-unused-function \
	"

CFLAGS="-Wall -O0 -ggdb -std=gnu11 "

gcc -o arith arithmetic.c ../vec.c ../hash.c ../rpn.c ../hash_fns/MurmurHash3.c \
	-lm \
	$CFLAGS $DISABLE_DUMB_WARNINGS \
	-DLINUX 

