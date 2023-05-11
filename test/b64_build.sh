#!/bin/bash


# g++ rbcompcpp.cpp  -o rbcpp -O0 -ggdb -lm 

gcc b64.c ../b64.c  -std=gnu17 -o b64test -O3 -lm \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-format \
	-Werror=implicit-int \
	-Werror=incompatible-pointer-types

