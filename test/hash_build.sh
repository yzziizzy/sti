#!/bin/bash


# g++ rbcompcpp.cpp  -o rbcpp -O0 -ggdb -lm 

gcc hash.c ../hash.c ../hash_fns/MurmurHash3.c -std=gnu17 -o hashtest -O3 -lm \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-format \
	-Werror=implicit-int \
	-Werror=incompatible-pointer-types

