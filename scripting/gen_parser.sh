#!/bin/bash


../parser/parser_gen ./scripting_tokens.txt -e > lexer_enums.h
../parser/parser_gen ./scripting_tokens.txt -d > lexer_enum_names.h
../parser/parser_gen ./scripting_tokens.txt -c > lexer_csets.c
../parser/parser_gen ./scripting_tokens.txt -s > lexer_switch.c

