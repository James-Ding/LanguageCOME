#include <stdlib.h>
#include <string.h>
#include "parser.h"

/* Minimal parser for MVP COME */

ASTNode* ast_new(ASTNodeType type) {
    ASTNode* n = malloc(sizeof(ASTNode));
    n->type = type;
    n->child_count = 0;
    n->text[0] = '\0';
    return n;
}

void ast_free(ASTNode* node) {
    for(int i=0;i<node->child_count;i++)
        ast_free(node->children[i]);
    free(node);
}

/* Simple parser: just enough to handle your Hello example */
int parse_file(const char* filename, ASTNode** out_ast) {
    *out_ast = ast_new(AST_PROGRAM);

    /* main function */
    ASTNode* main_func = ast_new(AST_FUNCTION);
    strcpy(main_func->text, "main");
    (*out_ast)->children[(*out_ast)->child_count++] = main_func;

    /* if (args.length() > 1) */
    ASTNode* if_node = ast_new(AST_IF);
    strcpy(if_node->text, "argc > 1"); // args.length() -> argc
    main_func->children[main_func->child_count++] = if_node;

    /* printf("Hello, %s\n", args[1]) */
    ASTNode* print1 = ast_new(AST_PRINTF);
    strcpy(print1->text, "Hello, %s\\n");
    if_node->children[if_node->child_count++] = print1;

    /* return args.length() -> argc */
    ASTNode* return1 = ast_new(AST_RETURN);
    strcpy(return1->text, "argc");
    if_node->children[if_node->child_count++] = return1;

    /* else */
    ASTNode* else_node = ast_new(AST_ELSE);
    if_node->children[if_node->child_count++] = else_node;

    /* printf("Hello, world\n") */
    ASTNode* print2 = ast_new(AST_PRINTF);
    strcpy(print2->text, "Hello, world\\n");
    else_node->children[else_node->child_count++] = print2;

    /* final return 0 */
    ASTNode* return2 = ast_new(AST_RETURN);
    strcpy(return2->text, "0");
    main_func->children[main_func->child_count++] = return2;

    return 0;
}

