#!/bin/bash


# g++ rbcompcpp.cpp  -o rbcpp -O0 -ggdb -lm 

gcc -DDEBUG_RED_BLACK_TREE=1 rbtest.c ../sti.c -o rbtest -O3 -lm \
	-Wall \
	-Wpedantic \
	-Wno-format \
	-Werror=implicit-int \
	-Werror=incompatible-pointer-types

