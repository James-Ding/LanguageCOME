#!/bin/bash
mkdir -p build/tests
gcc -Wall -g -Isrc/include -Isrc/core/include tests/test_lexer.c src/core/lexer.c -o build/tests/test_lexer
./build/tests/test_lexer

gcc -Wall -g -Isrc/include -Isrc/core/include tests/test_parser.c src/core/parser.c src/core/lexer.c -o build/tests/test_parser
./build/tests/test_parser

gcc -Wall -g -Isrc/include -Isrc/core/include tests/test_codegen.c src/core/parser.c src/core/lexer.c src/core/codegen.c -o build/tests/test_codegen
./build/tests/test_codegen

