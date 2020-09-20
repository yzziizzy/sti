#!/bin/bash

# Public Domain.

./gen_parser.sh ./scripting_tokens.txt

CFLAGS="-Wall  -Wextra "

gcc -o ass assembler.c lexer.c ../sti.c \
	-lm \
	$CFLAGS \
	-O0 -ggdb -DLINUX -std=gnu11

