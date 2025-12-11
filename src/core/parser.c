#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"
#include "lexer.h"

static TokenList tokens;
static int pos;

static Token* current() {
    if (pos >= tokens.count) return &tokens.tokens[tokens.count-1];
    return &tokens.tokens[pos];
}

static void advance() {
    if (pos < tokens.count) pos++;
}

static int match(TokenType type) {
    // printf("match(%d) vs %d\n", type, current()->type);
    if (current()->type == type) {
        advance();
        return 1;
    }
    return 0;
}

static int expect(TokenType type) {
    // printf("expect(%d) vs %d\n", type, current()->type);
    if (match(type)) return 1;
    printf("Expected token type %d, got %d ('%s')\n", type, current()->type, current()->text);
    return 0;
}

ASTNode* ast_new(ASTNodeType type) {
    ASTNode* n = malloc(sizeof(ASTNode));
    n->type = type;
    n->child_count = 0;
    n->text[0] = '\0';
    return n;
}

void ast_free(ASTNode* node) {
    if (!node) return;
    for(int i=0;i<node->child_count;i++)
        ast_free(node->children[i]);
    free(node);
}

// Forward decls
static ASTNode* parse_block();
static ASTNode* parse_statement();
static ASTNode* parse_expression();

static ASTNode* parse_expression() {
    // printf("parse_expression start, token: %d ('%s')\n", current()->type, current()->text);
    ASTNode* node = NULL;
    Token* t = current();
    
    // Handle NOT operator
    if (t->type == TOKEN_NOT) {
        advance();
        ASTNode* not_node = ast_new(AST_CALL);
        strcpy(not_node->text, "!");
        not_node->children[not_node->child_count++] = parse_expression();
        return not_node;
    }
    
    if (t->type == TOKEN_IDENTIFIER) {
        node = ast_new(AST_IDENTIFIER);
        strcpy(node->text, t->text);
        advance();
        
        // Check for method call: ident.method(...)
        if (match(TOKEN_DOT)) {
            Token* method = current();
            if (expect(TOKEN_IDENTIFIER)) {
                ASTNode* call = ast_new(AST_METHOD_CALL);
                // Child 0: object (identifier)
                call->children[call->child_count++] = node;
                // Text: method name
                strcpy(call->text, method->text);
                
                expect(TOKEN_LPAREN);
                while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) {
                    call->children[call->child_count++] = parse_expression();
                    if (!match(TOKEN_COMMA)) break;
                }
                expect(TOKEN_RPAREN);
                node = call;
                node = call;
            }
        }
        
        // Handle array access: ident[expr]
        if (match(TOKEN_LBRACKET)) {
            ASTNode* index = parse_expression();
            expect(TOKEN_RBRACKET);
            
            ASTNode* access = ast_new(AST_ARRAY_ACCESS);
            access->children[access->child_count++] = node; // Array
            access->children[access->child_count++] = index; // Index
            node = access;
        }
    } else if (t->type == TOKEN_STRING_LITERAL) {
        node = ast_new(AST_STRING_LITERAL);
        strcpy(node->text, t->text);
        advance();
        
        // Handle method call on string literal: "foo".method()
        if (match(TOKEN_DOT)) {
            Token* method = current();
            if (expect(TOKEN_IDENTIFIER)) {
                ASTNode* call = ast_new(AST_METHOD_CALL);
                call->children[call->child_count++] = node;
                strcpy(call->text, method->text);
                
                expect(TOKEN_LPAREN);
                while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) {
                    call->children[call->child_count++] = parse_expression();
                    if (!match(TOKEN_COMMA)) break;
                }
                expect(TOKEN_RPAREN);
                node = call;
            }
        }
    } else if (t->type == TOKEN_TRUE || t->type == TOKEN_FALSE) {
        node = ast_new(AST_BOOL_LITERAL);
        strcpy(node->text, t->text);
        advance();
    } else if (t->type == TOKEN_NUMBER) {
        node = ast_new(AST_NUMBER);
        strcpy(node->text, t->text);
        advance();
    } else if (match(TOKEN_LPAREN)) {
        // ( expr ) - ignored for now
    }
    
    // Check for binary ops? (e.g. > 1, == 0)
    // For MVP, if we see >, ==, etc, we might need to handle them.
    // Lexer doesn't have them yet!
    // But lexer has TOKEN_UNKNOWN.
    // Wait, `lexer.c` doesn't handle `>` or `==`.
    // I need to update lexer for operators too!
    // For now, let's assume expressions are simple.
    
    return node;
}

static ASTNode* parse_statement() {
    Token* t = current();
    // printf("parse_statement start, token: %d ('%s')\n", t->type, t->text);
    
    if (t->type == TOKEN_STRING || t->type == TOKEN_INT || t->type == TOKEN_BOOL) {
        // Variable declaration: string s = ...
        // or int x = ...
        char type_name[32];
        strcpy(type_name, t->text);
        advance();
        
        // Check for array: string args[]
        if (match(TOKEN_IDENTIFIER)) {
            char var_name[64];
            strcpy(var_name, tokens.tokens[pos-1].text);
            
            int is_array = 0;
            if (match(TOKEN_LBRACKET)) {
                expect(TOKEN_RBRACKET);
                is_array = 1;
            }
            
            if (match(TOKEN_ASSIGN)) {
                ASTNode* decl = ast_new(AST_VAR_DECL);
                strcpy(decl->text, var_name); // Var name
                // Child 0: Initializer expression
                decl->children[decl->child_count++] = parse_expression();
                // Child 1: Type
                ASTNode* type_node = ast_new(AST_IDENTIFIER);
                strcpy(type_node->text, type_name);
                if (is_array) strcat(type_node->text, "[]"); // Mark as array
                decl->children[decl->child_count++] = type_node;
                return decl;
            }
        }
    } else if (t->type == TOKEN_PRINTF) {
        advance();
        expect(TOKEN_LPAREN);
        ASTNode* node = ast_new(AST_PRINTF);
        // Format string
        if (current()->type == TOKEN_STRING_LITERAL) {
            strcpy(node->text, current()->text);
            advance();
        }
        // Args
        while (match(TOKEN_COMMA)) {
            node->children[node->child_count++] = parse_expression();
        }
        expect(TOKEN_RPAREN);
        return node;
    } else if (t->type == TOKEN_IF) {
        advance();
        expect(TOKEN_LPAREN);
        ASTNode* cond = parse_expression(); 
        // Check for binary operator
        Token* next = current();
        // printf("IF cond parsed, next token: %d ('%s')\n", next->type, next->text);
        if (next->type == TOKEN_EQ || next->type == TOKEN_NEQ || 
            next->type == TOKEN_GT || next->type == TOKEN_LT || 
            next->type == TOKEN_GE || next->type == TOKEN_LE) {
             
             char op[32];
             strcpy(op, next->text);
             advance();
             ASTNode* rhs = parse_expression();
             
             ASTNode* op_node = ast_new(AST_CALL);
             strcpy(op_node->text, op);
             op_node->children[op_node->child_count++] = cond;
             op_node->children[op_node->child_count++] = rhs;
             cond = op_node;
        }
        
        if (!match(TOKEN_RPAREN)) {
             printf("Expected RPAREN after IF condition, got %d ('%s')\n", current()->type, current()->text);
        }
        
        ASTNode* node = ast_new(AST_IF);
        node->children[node->child_count++] = cond; // Child 0 is condition
        node->children[node->child_count++] = parse_statement(); // Child 1 is then statement/block
        
        if (match(TOKEN_ELSE)) {
            ASTNode* else_node = ast_new(AST_ELSE);
            else_node->children[else_node->child_count++] = parse_statement();
            node->children[node->child_count++] = else_node;
        }
        return node;
    } else if (t->type == TOKEN_RETURN) {
        advance();
        ASTNode* node = ast_new(AST_RETURN);
        if (current()->type != TOKEN_RBRACE) { // if not end of block
             ASTNode* expr = parse_expression();
             if (expr) {
                 node->children[node->child_count++] = expr;
             }
        }
        return node;
    } else if (t->type == TOKEN_LBRACE) {
        return parse_block();
    } else if (t->type == TOKEN_IDENTIFIER) {
        // Assignment or method call as statement
        // Check lookahead
        if (tokens.tokens[pos+1].type == TOKEN_DOT) {
             // Method call
             ASTNode* expr = parse_expression();
             return expr; // Treat as statement
        }
    }
    
    advance(); // Skip unknown
    return NULL;
}

static ASTNode* parse_block() {
    expect(TOKEN_LBRACE);
    ASTNode* block = ast_new(AST_BLOCK);
    while (current()->type != TOKEN_RBRACE && current()->type != TOKEN_EOF) {
        ASTNode* stmt = parse_statement();
        if (stmt) block->children[block->child_count++] = stmt;
    }
    expect(TOKEN_RBRACE);
    return block;
}

int parse_file(const char* filename, ASTNode** out_ast) {
    if (lex_file(filename, &tokens) != 0) return 1;
    pos = 0;
    
    *out_ast = ast_new(AST_PROGRAM);
    
    while (pos < tokens.count) {
        Token* t = current();
        if (t->type == TOKEN_EOF) break;
        
        if (match(TOKEN_IMPORT)) {
            if (current()->type == TOKEN_STRING) advance();
            else expect(TOKEN_IDENTIFIER); // std or string
            // Ignore for now
        } else if (current()->type == TOKEN_MAIN) {
            // Check if it's main() or module main
            if (tokens.tokens[pos+1].type == TOKEN_LPAREN) {
                advance(); // consume main
                ASTNode* func = ast_new(AST_FUNCTION);
                strcpy(func->text, "main");
                
                expect(TOKEN_LPAREN);
                
                ASTNode* args_decl = NULL;
                if (current()->type == TOKEN_STRING) {
                    advance(); // string
                    char arg_name[64];
                    strcpy(arg_name, current()->text);
                    expect(TOKEN_IDENTIFIER);
                    expect(TOKEN_LBRACKET);
                    expect(TOKEN_RBRACKET);
                    
                    args_decl = ast_new(AST_VAR_DECL);
                    strcpy(args_decl->text, arg_name);
                    
                    ASTNode* init = ast_new(AST_STRING_LITERAL);
                    strcpy(init->text, "\"__ARGS__\"");
                    args_decl->children[args_decl->child_count++] = init;
                    
                    ASTNode* type = ast_new(AST_IDENTIFIER);
                    strcpy(type->text, "string[]");
                    args_decl->children[args_decl->child_count++] = type;
                }
                
                expect(TOKEN_RPAREN);
                
                ASTNode* body = parse_block();
                
                if (args_decl) {
                    func->children[func->child_count++] = args_decl;
                }
                
                for(int i=0; i<body->child_count; i++) {
                    func->children[func->child_count++] = body->children[i];
                }
                free(body);
                
                (*out_ast)->children[(*out_ast)->child_count++] = func;
            } else {
                advance(); // Ignore 'module main'
            }
        } else if (t->type == TOKEN_INT) { // int main(...)
            advance();
            if (expect(TOKEN_MAIN)) {
                ASTNode* func = ast_new(AST_FUNCTION);
                strcpy(func->text, "main");
                
                expect(TOKEN_LPAREN);
                
                ASTNode* args_decl = NULL;
                if (current()->type == TOKEN_STRING) {
                    advance(); // string
                    char arg_name[64];
                    strcpy(arg_name, current()->text);
                    expect(TOKEN_IDENTIFIER);
                    expect(TOKEN_LBRACKET);
                    expect(TOKEN_RBRACKET);
                    
                    args_decl = ast_new(AST_VAR_DECL);
                    strcpy(args_decl->text, arg_name);
                    
                    ASTNode* init = ast_new(AST_STRING_LITERAL);
                    strcpy(init->text, "\"__ARGS__\"");
                    args_decl->children[args_decl->child_count++] = init;
                    
                    ASTNode* type = ast_new(AST_IDENTIFIER);
                    strcpy(type->text, "string[]");
                    args_decl->children[args_decl->child_count++] = type;
                }
                
                expect(TOKEN_RPAREN);
                
                ASTNode* body = parse_block();
                
                if (args_decl) {
                    func->children[func->child_count++] = args_decl;
                }
                
                for(int i=0; i<body->child_count; i++) {
                    func->children[func->child_count++] = body->children[i];
                }
                free(body);
                
                (*out_ast)->children[(*out_ast)->child_count++] = func;
            }
        } else {
            advance();
        }
    }
    
    return 0;
}
