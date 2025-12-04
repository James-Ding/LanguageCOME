#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "codegen.h"
#include "ast.h"

int main() {
    ASTNode* root = NULL;
    // parse_file currently returns a hardcoded AST, so the input file doesn't matter much
    // but we need it to succeed.
    if (parse_file("tests/test_files/hello.co", &root) != 0) {
        printf("Parser failed\n");
        return 1;
    }

    const char* out_file = "build/tests/test_output.c";
    if (generate_c_from_ast(root, out_file) != 0) {
        printf("Codegen failed\n");
        return 1;
    }

    // Verify output file exists and has some content
    FILE* f = fopen(out_file, "r");
    if (!f) {
        printf("Output file not created\n");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);

    if (size == 0) {
        printf("Output file is empty\n");
        return 1;
    }

    printf("Codegen test passed!\n");
    ast_free(root);
    return 0;
}
