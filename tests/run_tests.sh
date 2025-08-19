#!/bin/bash
mkdir -p build/tests
gcc -Wall -g -Isrc/include tests/test_lexer.c src/lexer.c -o build/tests/test_lexer
./build/tests/test_lexer

