#!/bin/bash

gcc -lutil _build.c -o ._build -ggdb \
	&& ./._build \
	&& gdb -ex r ./lextest
