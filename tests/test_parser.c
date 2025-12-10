#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "ast.h"

int main() {
    ASTNode* root = NULL;
    if (parse_file("dummy.co", &root) != 0) {
        printf("Parser failed\n");
        return 1;
    }

    if (root == NULL) {
        printf("Parser returned NULL root\n");
        return 1;
    }

    if (root->type != AST_PROGRAM) {
        printf("Root type mismatch. Expected %d, got %d\n", AST_PROGRAM, root->type);
        return 1;
    }

    // Check for main function
    if (root->child_count < 1 || root->children[0]->type != AST_FUNCTION) {
        printf("Missing main function\n");
        return 1;
    }

    ASTNode* main_func = root->children[0];
    if (strcmp(main_func->text, "main") != 0) {
        printf("Main function name mismatch\n");
        return 1;
    }

    printf("Parser test passed!\n");
    ast_free(root);
    return 0;
}
