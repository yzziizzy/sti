#!/bin/bash


gcc -o sti_test test.c sti.c -Wall -Werror -Wextra -pedantic -O0 -ggdb -DLINUX

