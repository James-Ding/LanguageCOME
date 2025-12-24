#include <stdio.h>
#include <string.h>
#include "lexer.h"

int main() {
    TokenList tokens;
    if (lex_file("examples/hello.co", &tokens)) {
        printf("Lexer failed\n");
        return 1;
    }

    int found_printf = 0;
    for(int i=0;i<tokens.count;i++){
        if(tokens.tokens[i].type == TOKEN_IDENTIFIER && strcmp(tokens.tokens[i].text, "printf") == 0) found_printf = 1;
    }

    if(found_printf) {
        printf("Lexer test passed!\n");
        return 0;
    } else {
        printf("Lexer test failed!\n");
        return 1;
    }
}

