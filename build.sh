#!/bin/bash


gcc -o sti_test test.c fs.c sets.c vec.c misc.c -Wall -O0 -ggdb -DLINUX

