#!/bin/bash

# Public Domain.

./parser_gen -gecndfs $1 > parser_example_generated.h

gcc -o parser_example parser_example.c ../sti.c -lm  -Werror -O0 -ggdb -DLINUX -std=gnu11
