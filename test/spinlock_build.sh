#!/bin/bash



gcc -std=gnu11 -O0 -ggdb -S spinlock.c 

gcc -std=gnu11 -O0 -ggdb spinlock.c ../spinlock.c -o spinlock \
&& ./spinlock $@




