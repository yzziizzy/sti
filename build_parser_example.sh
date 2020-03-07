#!/bin/bash

# Public Domain.

./parser_gen -e $1 > parser_example_enums.h
./parser_gen -s $1 > parser_example_switch.c
./parser_gen -c $1 > parser_example_csets.c
./parser_gen -n $1 > parser_example_enum_names.h

gcc -o parser_example parser_example.c sti.c -lm  -Werror -O0 -ggdb -DLINUX -std=gnu11
