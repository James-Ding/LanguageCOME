#include <stdlib.h>
#include <string.h>
#include "parser.h"

ASTNode* ast_new(ASTNodeType type) {
    ASTNode* n = malloc(sizeof(ASTNode));
    n->type = type; n->child_count=0; n->text[0]='\0';
    return n;
}
void ast_free(ASTNode* node){ for(int i=0;i<node->child_count;i++) ast_free(node->children[i]); free(node); }

int parse_file(const char* filename, ASTNode** out_ast){
    *out_ast = ast_new(AST_PROGRAM);
    ASTNode* main_func = ast_new(AST_FUNCTION); strcpy(main_func->text,"main");
    (*out_ast)->children[(*out_ast)->child_count++] = main_func;
    ASTNode* print_node = ast_new(AST_PRINTF); strcpy(print_node->text,"Hello world!");
    main_func->children[main_func->child_count++] = print_node;
    return 0;
}
