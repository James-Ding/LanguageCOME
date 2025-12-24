#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "ast.h"

int main() {
    ASTNode* root = NULL;
    if (parse_file("examples/hello.co", &root) != 0) {
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
    int found_main = 0;
    for (int i = 0; i < root->child_count; i++) {
        ASTNode* node = root->children[i];
        if (node->type == AST_FUNCTION && strcmp(node->text, "main") == 0) {
            found_main = 1;
            break;
        }
    }

    if (!found_main) {
        printf("Missing main function\n");
        return 1;
    }

    printf("Parser test passed!\n");
    ast_free(root);
    return 0;
}
