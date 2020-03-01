#!/bin/bash

# Public Domain.

./parser_gen -e $1 > parser_example_enums.h
./parser_gen -s $1 > parser_example_switch.c

gcc -o parser_example parser_example.c sti.c -lm  -Werror -O0 -ggdb -DLINUX -std=gnu11
