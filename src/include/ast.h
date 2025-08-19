#ifndef AST_H
#define AST_H
typedef enum { AST_PROGRAM, AST_FUNCTION, AST_PRINTF, AST_IF, AST_RETURN } ASTNodeType;
typedef struct ASTNode {
    ASTNodeType type;
    char text[128];
    struct ASTNode* children[16];
    int child_count;
} ASTNode;
ASTNode* ast_new(ASTNodeType type);
void ast_free(ASTNode* node);
#endif
