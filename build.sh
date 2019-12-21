#!/bin/bash

# Public Domain.


gcc -o sti_test test.c sti.c \
	-Wall -Werror -Wextra -pedantic \
	-O0 -ggdb -DLINUX -std=gnu11
