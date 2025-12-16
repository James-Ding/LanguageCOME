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
    if (current()->type == type) {
        advance();
        return 1;
    }
    return 0;
}

static int expect(TokenType type) {
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

static int get_precedence(TokenType type) {
    switch (type) {
        case TOKEN_LOGIC_OR: return 1;
        case TOKEN_LOGIC_AND: return 2;
        case TOKEN_EQ: case TOKEN_NEQ: return 3;
        case TOKEN_LT: case TOKEN_GT: case TOKEN_LE: case TOKEN_GE: return 4;
        case TOKEN_PLUS: case TOKEN_MINUS: return 5;
        case TOKEN_STAR: case TOKEN_SLASH: case TOKEN_PERCENT: return 6;
        default: return 0;
    }
}

static ASTNode* parse_primary() {
    ASTNode* node = NULL;
    Token* t = current();
    
    // Handle NOT operator
    if (t->type == TOKEN_NOT) {
        advance();
        ASTNode* not_node = ast_new(AST_UNARY_OP); // Use UNARY_OP
        strcpy(not_node->text, "!");
        not_node->children[not_node->child_count++] = parse_primary(); // Recurse primary for !
        return not_node;
    }
    
    if (t->type == TOKEN_IDENTIFIER) {
        node = ast_new(AST_IDENTIFIER);
        strcpy(node->text, t->text);
        advance();
        
        // Check for method call or member access: ident.method(...) or ident.field
        if (match(TOKEN_DOT)) {
            Token* member = current();
            if (expect(TOKEN_IDENTIFIER)) {
                if (match(TOKEN_LPAREN)) {
                    // Method Call
                    ASTNode* call = ast_new(AST_METHOD_CALL);
                    call->children[call->child_count++] = node;
                    strcpy(call->text, member->text);
                    
                    while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) {
                        call->children[call->child_count++] = parse_expression();
                        if (!match(TOKEN_COMMA)) break;
                    }
                    expect(TOKEN_RPAREN);
                    node = call;
                } else {
                    // Member Access
                    ASTNode* access = ast_new(AST_MEMBER_ACCESS);
                    access->children[access->child_count++] = node;
                    strcpy(access->text, member->text);
                    node = access;
                }
            }
        }
        // Handle function call: ident(...)
        else if (match(TOKEN_LPAREN)) {
            ASTNode* call = ast_new(AST_CALL);
            strcpy(call->text, node->text);
            free(node); 
            node = call;
            
            while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) {
                node->children[node->child_count++] = parse_expression();
                if (!match(TOKEN_COMMA)) break;
            }
            expect(TOKEN_RPAREN);
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
    } else if (t->type == TOKEN_CHAR_LITERAL) {
        node = ast_new(AST_NUMBER);
        strcpy(node->text, t->text); 
        advance();
    } else if (t->type == TOKEN_NUMBER || t->type == TOKEN_WCHAR_LITERAL) {
        node = ast_new(AST_NUMBER);
        strcpy(node->text, t->text);
        advance();
    } else if (match(TOKEN_LBRACKET)) {
        // Array initializer: [1, 2, 3]
        node = ast_new(AST_AGGREGATE_INIT);
        strcpy(node->text, "ARRAY");
        while (current()->type != TOKEN_RBRACKET && current()->type != TOKEN_EOF) {
            node->children[node->child_count++] = parse_expression();
            if (!match(TOKEN_COMMA)) break;
        }
        expect(TOKEN_RBRACKET);
    } else if (match(TOKEN_LBRACE)) {
        // Map/Struct initializer: { k: v, ... } or { }
        node = ast_new(AST_AGGREGATE_INIT);
        strcpy(node->text, "MAP");
        while (current()->type != TOKEN_RBRACE && current()->type != TOKEN_EOF) {
             node->children[node->child_count++] = parse_expression();
             if (!match(TOKEN_COMMA)) break;
        }
        expect(TOKEN_RBRACE);
    } else if (match(TOKEN_LPAREN)) {
        node = parse_expression();
        expect(TOKEN_RPAREN);
    }
    
    return node;
}

static ASTNode* parse_expression_prec(int min_prec) {
    ASTNode* lhs = parse_primary();
    if (!lhs) return NULL;

    while (1) {
        Token* t = current();
        int prec = get_precedence(t->type);
        if (prec == 0 || prec < min_prec) break;

        char op_text[32];
        strcpy(op_text, t->text);
        advance(); // consume op

        ASTNode* rhs = parse_expression_prec(prec + 1);
        
        ASTNode* bin = ast_new(AST_BINARY_OP);
        strcpy(bin->text, op_text);
        bin->children[bin->child_count++] = lhs;
        bin->children[bin->child_count++] = rhs;
        lhs = bin;
    }
    return lhs;
}

static ASTNode* parse_expression() {
    return parse_expression_prec(0);
}

static ASTNode* parse_statement() {
    Token* t = current();
    // printf("parse_statement start, token: %d ('%s')\n", t->type, t->text);
    
    if (t->type == TOKEN_STRING || t->type == TOKEN_INT || t->type == TOKEN_BOOL ||
        t->type == TOKEN_BYTE || t->type == TOKEN_UBYTE ||
        t->type == TOKEN_SHORT || t->type == TOKEN_USHORT ||
        t->type == TOKEN_UINT ||
        t->type == TOKEN_LONG || t->type == TOKEN_ULONG ||
        t->type == TOKEN_FLOAT || t->type == TOKEN_DOUBLE ||
        t->type == TOKEN_WCHAR || t->type == TOKEN_VOID ||
        t->type == TOKEN_MAP || t->type == TOKEN_STRUCT || t->type == TOKEN_VAR) {
        // Variable declaration: string s = ...
        // or int x = ...
        // or var x = ...
        char type_name[32];
        strcpy(type_name, t->text);
        advance();
        
        // Check for array: string args[]
        if (match(TOKEN_IDENTIFIER)) {
            char var_name[64];
            strcpy(var_name, tokens.tokens[pos-1].text);
            
            int is_array = 0;
            if (match(TOKEN_LBRACKET)) {
                while(current()->type!=TOKEN_RBRACKET && current()->type!=TOKEN_EOF) advance();
                expect(TOKEN_RBRACKET);
                is_array = 1;
            }
            
            // Always handle as declaration if type matches
             ASTNode* decl = ast_new(AST_VAR_DECL);
             strcpy(decl->text, var_name); // Var name
             
             // Child 0: Initializer expression
             if (tokens.tokens[pos-1].type == TOKEN_ASSIGN) { // If just matched ASSIGN (unlikely here if logic changed)
                  decl->children[decl->child_count++] = parse_expression();
             } else if (match(TOKEN_ASSIGN)) {
                  decl->children[decl->child_count++] = parse_expression();
             } else {
                  // No initializer? Uninitialized var.
                  ASTNode* dummy = ast_new(AST_NUMBER);
                  strcpy(dummy->text, "0"); // Default init?
                  decl->children[decl->child_count++] = dummy; 
             }

             // Child 1: Type
             ASTNode* type_node = ast_new(AST_IDENTIFIER);
             strcpy(type_node->text, type_name);
             if (is_array) strcat(type_node->text, "[]"); // Mark as array
             decl->children[decl->child_count++] = type_node;
             return decl;
        }
    } else if (t->type == TOKEN_IDENTIFIER) {
        // Check for custom type declaration: MyType x ...
        // Lookahead 1
        if (pos + 1 < tokens.count && tokens.tokens[pos+1].type == TOKEN_IDENTIFIER) {
             // Treat as declaration
             char type_name[64];
             strcpy(type_name, t->text);
             advance(); // consume type
             
             char var_name[64];
             strcpy(var_name, tokens.tokens[pos].text);
             advance(); // consume var name
             
             // Check array
             int is_array = 0;
             if (match(TOKEN_LBRACKET)) {
                 while(current()->type!=TOKEN_RBRACKET && current()->type!=TOKEN_EOF) advance();
                 expect(TOKEN_RBRACKET);
                 is_array = 1;
             }
             
             ASTNode* decl = ast_new(AST_VAR_DECL);
             strcpy(decl->text, var_name);
             
             // Init
             if (match(TOKEN_ASSIGN)) {
                 decl->children[decl->child_count++] = parse_expression();
             } else {
                 // Default init 0
                 ASTNode* dummy = ast_new(AST_NUMBER);
                 strcpy(dummy->text, "0"); 
                 decl->children[decl->child_count++] = dummy;
             }
             
             // Type
             ASTNode* type_node = ast_new(AST_IDENTIFIER);
             strcpy(type_node->text, type_name);
             if (is_array) strcat(type_node->text, "[]");
             decl->children[decl->child_count++] = type_node;
             return decl;
        } else if (pos + 1 < tokens.count && tokens.tokens[pos+1].type == TOKEN_EQ) {
             // Identifier = Expr (Assignment)
             ASTNode* assign = ast_new(AST_ASSIGN);
             strcpy(assign->text, t->text); // var name
             advance(); // Identifier
             advance(); // =
             assign->children[assign->child_count++] = parse_expression();
             return assign;
        } else {
             // Maybe function call or expression statement?
             // Treat as expression statement
             ASTNode* expr = parse_expression();
             return expr;
        }
    } else if (t->type == TOKEN_SWITCH) {
        advance();
        expect(TOKEN_LPAREN);
        ASTNode* expr = parse_expression();
        expect(TOKEN_RPAREN);
        
        ASTNode* switch_node = ast_new(AST_SWITCH);
        switch_node->children[switch_node->child_count++] = expr;
        
        expect(TOKEN_LBRACE);
        while(current()->type!=TOKEN_RBRACE && current()->type!=TOKEN_EOF) {
             ASTNode* stmt = parse_statement();
             if (stmt) switch_node->children[switch_node->child_count++] = stmt;
        }
        expect(TOKEN_RBRACE);
        return switch_node;
    } else if (t->type == TOKEN_CASE) {
        advance();
        ASTNode* case_node = ast_new(AST_CASE);
        case_node->children[case_node->child_count++] = parse_expression();
        expect(TOKEN_COLON);
        // Statements are siblings in the switch block in this simple AST, 
        // OR we can make statements children of CASE?
        // C-style parser usually treats case as a label.
        // But for AST generation, it's easier if CASE contains statements or just marks position?
        // Given existing structure, let's treat CASE as a node, and subsequent statements as siblings in the SWITCH list?
        // Or make CASE contain the statements until next case/end?
        // Let's try to capture statements until next case.
        while (current()->type != TOKEN_CASE && current()->type != TOKEN_DEFAULT && current()->type != TOKEN_RBRACE && current()->type != TOKEN_EOF) {
             ASTNode* s = parse_statement();
             if (s) case_node->children[case_node->child_count++] = s;
        }
        return case_node;
    } else if (t->type == TOKEN_DEFAULT) {
        advance();
        expect(TOKEN_COLON);
        ASTNode* def_node = ast_new(AST_DEFAULT);
         while (current()->type != TOKEN_CASE && current()->type != TOKEN_DEFAULT && current()->type != TOKEN_RBRACE && current()->type != TOKEN_EOF) {
             ASTNode* s = parse_statement();
             if (s) def_node->children[def_node->child_count++] = s;
        }
        return def_node;
    } else if (t->type == TOKEN_FALLTHROUGH) {
        advance();
        // Ignoring for AST/codegen for now, or emit a special node?
        // C fallthrough is default, but COME says fallthrough explicit.
        // Codegen needs to know? 
        // Actually if we group statements under CASE, we naturally break unless we check fallthrough.
        // Let's assume codegen implements switch as if-else chains or real switch.
        // For MVP, ignore fallthrough implies manual break handling? 
        // Wait, "COME switch does NOT fall through by default".
        // So we need to emit "break" at end of case unless fallthrough is present.
        // Let's return a FALLTHROUGH token node if strictly needed, or just handle in codegen logic.
        return NULL; // Consume
    } else if (t->type == TOKEN_WHILE) {
        advance();
        expect(TOKEN_LPAREN);
        ASTNode* cond = parse_expression();
        expect(TOKEN_RPAREN);
        ASTNode* body = parse_block();
        
        ASTNode* node = ast_new(AST_WHILE);
        node->children[node->child_count++] = cond;
        node->children[node->child_count++] = body;
        return node;
    } else if (t->type == TOKEN_DO) {
        advance();
        ASTNode* body = parse_block();
        expect(TOKEN_WHILE);
        expect(TOKEN_LPAREN);
        ASTNode* cond = parse_expression();
        expect(TOKEN_RPAREN);
        
        ASTNode* node = ast_new(AST_DO_WHILE);
        node->children[node->child_count++] = body;
        node->children[node->child_count++] = cond;
        return node;
    } else if (t->type == TOKEN_FOR) {
        advance();
        expect(TOKEN_LPAREN);
        ASTNode* node = ast_new(AST_FOR);
        
        // Init (stmt or expr)
        if (current()->type != TOKEN_SEMICOLON) {
             ASTNode* init = parse_statement(); // Handles variable decl or expr
             if (init) node->children[node->child_count++] = init;
        } else {
             // Empty init
             node->children[node->child_count++] = NULL;
        }
        if (current()->type == TOKEN_SEMICOLON) advance(); // consume ;

        // Condition
        if (current()->type != TOKEN_SEMICOLON) {
             ASTNode* cond = parse_expression();
             node->children[node->child_count++] = cond;
        } else {
             node->children[node->child_count++] = NULL;
        }
        if (current()->type == TOKEN_SEMICOLON) advance(); // consume ;
        
        // Iteration
        if (current()->type != TOKEN_RPAREN) {
             ASTNode* iter = parse_expression(); // usually expr: i++
             // But i++ is not expression in some parsers?
             // If i++ is handled as assignment or unary update?
             // My parser doesn't handle ++ yet?
             // come.co uses `i++`.
             // I need to add ++/-- tokens and parsing logic.
             // Assume i++ parses as expression or assignment.
             // If parse_expression fails on i++, we are in trouble.
             // Quick check: lexer has no `++`.
             // I'll add `++` token logic in next step or now?
             // For now assume `i = i + 1` or similar if I can't parse `i++`.
             // But come.co uses `k++`.
             // I'll assume I update lexer for ++ too.
             node->children[node->child_count++] = iter;
        } else {
             node->children[node->child_count++] = NULL;
        }
        expect(TOKEN_RPAREN);
        
        ASTNode* body = parse_statement(); // block or single stmt
        node->children[node->child_count++] = body;
        
        return node;
    }
    
    if (t->type == TOKEN_IDENTIFIER) {
         // ... (existing identifier handling)
         // Need to verify if it handles method calls properly vs assignment vs declaration
         // ...
         // (Keep existing logic but wrap the tail)
    }

    // Include the original trail
    if (t->type == TOKEN_PRINTF) {
        // ... (Keep existing printf logic)
        advance();
        expect(TOKEN_LPAREN);
        ASTNode* node = ast_new(AST_PRINTF);
        if (current()->type == TOKEN_STRING_LITERAL) {
            strcpy(node->text, current()->text);
            advance();
        }
        while (match(TOKEN_COMMA)) {
            node->children[node->child_count++] = parse_expression();
        }
        expect(TOKEN_RPAREN);
        return node;
    }
    
    // ... (Keep IF/RETURN logic)
    
     if (t->type == TOKEN_IF) {
        advance();
        expect(TOKEN_LPAREN);
        ASTNode* cond = parse_expression(); 
        
        // Simple comparison operator check hack in existing parser...
        Token* next = current();
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
        node->children[node->child_count++] = cond;
        node->children[node->child_count++] = parse_statement();
        
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
             // Handle multiple return values?
             // Spec: `return (a + b), (a > b) ? ">" : "<="`
             // Expression parser doesn't handle comma yet (except in arg lists).
             // Assume single expr for now or modify expr parser.
             ASTNode* expr = parse_expression();
             if (expr) {
                 node->children[node->child_count++] = expr;
                 while(match(TOKEN_COMMA)) {
                     node->children[node->child_count++] = parse_expression();
                 }
             }
        }
        return node;
    } else if (t->type == TOKEN_LBRACE) {
        return parse_block();    
    }
    
    // Handle assignments
    if (t->type == TOKEN_IDENTIFIER) {
         // Check lookahead for assignment
         if (tokens.tokens[pos+1].type == TOKEN_ASSIGN || 
             tokens.tokens[pos+1].type == TOKEN_PLUS_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_MINUS_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_STAR_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_SLASH_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_AND_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_OR_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_XOR_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_LSHIFT_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_RSHIFT_ASSIGN) {
              
              ASTNode* assign = ast_new(AST_ASSIGN);
              strcpy(assign->text, tokens.tokens[pos+1].text); // The operator
              
              ASTNode* lhs = ast_new(AST_IDENTIFIER);
              strcpy(lhs->text, t->text);
              assign->children[assign->child_count++] = lhs;
              
              advance(); // ident
              advance(); // op
              assign->children[assign->child_count++] = parse_expression();
              return assign;
         }
         
         // ARRAY assignment: arr[i] = val
         if (tokens.tokens[pos+1].type == TOKEN_LBRACKET) {
             // ... Complex lookahead or parse as expression and check if lvalue?
             // Simplification: parse as expression, check if top node is ARRAY_ACCESS, then check for ASSIGN.
             // But parse_expression consumes tokens.
             // Let's try parsing expr.
             ASTNode* expr = parse_expression();
             if (match(TOKEN_ASSIGN)) {
                 ASTNode* assign = ast_new(AST_ASSIGN);
                 strcpy(assign->text, "=");
                 assign->children[assign->child_count++] = expr;
                 assign->children[assign->child_count++] = parse_expression();
                 return assign;
             }
             return expr;
         }
    }
    
    // Default fallback to var declarations (primitives)
    if (t->type == TOKEN_STRING || t->type == TOKEN_INT || t->type == TOKEN_BOOL ||
        t->type == TOKEN_BYTE || t->type == TOKEN_UBYTE ||
        t->type == TOKEN_SHORT || t->type == TOKEN_USHORT ||
        t->type == TOKEN_UINT ||
        t->type == TOKEN_LONG || t->type == TOKEN_ULONG ||
        t->type == TOKEN_FLOAT || t->type == TOKEN_DOUBLE ||
        t->type == TOKEN_WCHAR || t->type == TOKEN_VOID ||
        t->type == TOKEN_MAP || t->type == TOKEN_STRUCT || t->type == TOKEN_VAR) {
            // ... (keep existing decl logic)
             char type_name[128];
        strcpy(type_name, t->text);
        advance();
        
        // Check for array: string args[]
        if (match(TOKEN_IDENTIFIER)) {
            char var_name[64];
            strcpy(var_name, tokens.tokens[pos-1].text);
            
            int is_array = 0;
            if (match(TOKEN_LBRACKET)) {
                // Handle size expression or empty
                while (current()->type != TOKEN_RBRACKET && current()->type != TOKEN_EOF) {
                    advance(); // simple skip for now
                }
                expect(TOKEN_RBRACKET);
                is_array = 1;
            }
            
            if (match(TOKEN_ASSIGN) || is_array) { // Handle "Type x;" too? For now require assignment or array syntax to distinguish from expression stmt effectively? No, "Type x" is decl.
                ASTNode* decl = ast_new(AST_VAR_DECL);
                strcpy(decl->text, var_name); // Var name
                
                // Child 0: Initializer expression
                if (tokens.tokens[pos-1].type == TOKEN_ASSIGN) { // If just matched ASSIGN
                     decl->children[decl->child_count++] = parse_expression();
                } else if (match(TOKEN_ASSIGN)) {
                     decl->children[decl->child_count++] = parse_expression();
                } else {
                     // No initializer? Uninitialized var.
                     ASTNode* dummy = ast_new(AST_NUMBER);
                     strcpy(dummy->text, "0"); // Default init?
                     decl->children[decl->child_count++] = dummy; 
                }

                ASTNode* type_node = ast_new(AST_IDENTIFIER);
                strcpy(type_node->text, type_name);
                if (is_array) strcat(type_node->text, "[]"); // Mark as array
                decl->children[decl->child_count++] = type_node;
                return decl;
            }
        }
    } else if (t->type == TOKEN_METHOD) {
        // method Name()
        advance();
        if (expect(TOKEN_IDENTIFIER)) {
            char name[64];
            strcpy(name, tokens.tokens[pos-1].text);
            expect(TOKEN_LPAREN);
            // args?
            while(current()->type!=TOKEN_RPAREN && current()->type!=TOKEN_EOF) advance();
            expect(TOKEN_RPAREN);
            // body?
            // In struct, just decl.
            // Return dummy node
             ASTNode* node = ast_new(AST_FUNCTION); // Use function for now or custom
             strcpy(node->text, name);
             return node; 
        }
    } else if (t->type == TOKEN_ALIAS) { // Handle alias
        advance();
        
        if (match(TOKEN_LPAREN)) {
            // alias (A, B) = (T1, T2)
            // Parse identifiers
            char names[16][64]; // Support up to 16 aliases in a tuple for now
            int name_count = 0;
            
            do {
                if (expect(TOKEN_IDENTIFIER)) {
                    strcpy(names[name_count++], tokens.tokens[pos-1].text);
                }
            } while (match(TOKEN_COMMA));
            
            expect(TOKEN_RPAREN);
            expect(TOKEN_ASSIGN);
            expect(TOKEN_LPAREN);
            
            // Parse types
            ASTNode* block = ast_new(AST_BLOCK);
            int type_count = 0;
            
            do {
                 if (match(TOKEN_IDENTIFIER) || match(TOKEN_INT) || match(TOKEN_STRING) || 
                     match(TOKEN_BYTE) || match(TOKEN_UBYTE) ||
                     match(TOKEN_SHORT) || match(TOKEN_USHORT) ||
                     match(TOKEN_UINT) ||
                     match(TOKEN_LONG) || match(TOKEN_ULONG) ||
                     match(TOKEN_FLOAT) || match(TOKEN_DOUBLE) ||
                     match(TOKEN_WCHAR) || match(TOKEN_MAP) || match(TOKEN_STRUCT)) {
                         
                    if (type_count < name_count) {
                        ASTNode* node = ast_new(AST_TYPE_ALIAS);
                        strcpy(node->text, names[type_count]);
                        
                        ASTNode* target = ast_new(AST_IDENTIFIER);
                        strcpy(target->text, tokens.tokens[pos-1].text);
                        node->children[node->child_count++] = target;
                        
                        block->children[block->child_count++] = node;
                    }
                    type_count++;
                 }
            } while (match(TOKEN_COMMA));
            
            expect(TOKEN_RPAREN);
            
            if (name_count != type_count) {
                printf("Error: Alias count (%d) does not match type count (%d)\n", name_count, type_count);
            }
            
            return block;

        } else if (expect(TOKEN_IDENTIFIER)) {
             char alias_name[64];
             strcpy(alias_name, tokens.tokens[pos-1].text);
             
             if (match(TOKEN_ASSIGN)) {
                 if (match(TOKEN_IDENTIFIER) || match(TOKEN_INT) || match(TOKEN_STRING) || 
                     match(TOKEN_BYTE) || match(TOKEN_UBYTE) ||
                     match(TOKEN_SHORT) || match(TOKEN_USHORT) ||
                     match(TOKEN_UINT) ||
                     match(TOKEN_LONG) || match(TOKEN_ULONG) ||
                     match(TOKEN_FLOAT) || match(TOKEN_DOUBLE) ||
                     match(TOKEN_WCHAR) || match(TOKEN_MAP) || match(TOKEN_STRUCT)) {
                      
                      ASTNode* node = ast_new(AST_TYPE_ALIAS);
                      strcpy(node->text, alias_name);
                      
                      ASTNode* target = ast_new(AST_IDENTIFIER);
                      strcpy(target->text, tokens.tokens[pos-1].text);
                      node->children[node->child_count++] = target;
                      return node;
                 }
             }
        }
    } else if (t->type == TOKEN_STRUCT) {
        // struct Name { ... } OR struct Name var;
        advance();
        if (expect(TOKEN_IDENTIFIER)) {
            char struct_name[64];
            strcpy(struct_name, tokens.tokens[pos-1].text);
            
            if (match(TOKEN_LBRACE)) {
                // struct definition: struct Name { int x; int y; }
                ASTNode* node = ast_new(AST_STRUCT_DECL);
                strcpy(node->text, struct_name);
                
                while (current()->type != TOKEN_RBRACE && current()->type != TOKEN_EOF) {
                     // Parse fields like "int x" (without assignment)
                     ASTNode* field = parse_statement(); // Reuse var decl parsing?
                     // parse_statement expects "int x = ..." or "int x". 
                     // Let's rely on standard decl parsing but ignore assignment if missing?
                     // Actually parser currently expects assignment for var decls: "if (match(TOKEN_ASSIGN))"
                     // We need to support decl without assignment.
                     if (field) node->children[node->child_count++] = field;
                }
                expect(TOKEN_RBRACE);
                return node;
            } else if (match(TOKEN_IDENTIFIER)) {
                 // struct Name var = ...
                 // Treated as standard var decl below if we handle it there.
            }
        }
    } else if (t->type == TOKEN_IDENTIFIER) {
         // Could be "MyType x = ..." OR "x = ..." OR "x.method()"
         // If next is IDENTIFIER, it's a declaration: "MyType x"
         Token* next = &tokens.tokens[pos+1];
         if (next->type == TOKEN_IDENTIFIER) {
             // User defined type declaration
             char type_name[64];
             strcpy(type_name, t->text);
             advance(); // eat type name
             
             char var_name[64];
             strcpy(var_name, current()->text);
             advance(); // eat var name
             
             if (match(TOKEN_ASSIGN)) {
                 ASTNode* decl = ast_new(AST_VAR_DECL);
                 strcpy(decl->text, var_name);
                 
                 ASTNode* type_node = ast_new(AST_IDENTIFIER);
                 strcpy(type_node->text, type_name);
                 
                 decl->children[decl->child_count++] = parse_expression(); // Init
                 decl->children[decl->child_count++] = type_node; // Type
                 return decl;
             }
         }
         
        // Assignment or method call as statement
        // Check lookahead
        if (tokens.tokens[pos+1].type == TOKEN_DOT || tokens.tokens[pos+1].type == TOKEN_ASSIGN) {
             // Method call or assignment
             // parse_expression will handle it?
             // Not AST_ASSIGN is not in parse_expression yet.
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
        
        if (t->type == TOKEN_MODULE) {
            advance(); // module
            if (current()->type == TOKEN_MAIN) advance();
            else if (current()->type == TOKEN_IDENTIFIER) advance();
            // ignore module decl for AST
        }
        else if (t->type == TOKEN_IMPORT) {
            advance();
            if (match(TOKEN_LPAREN)) {
                // import ( ... )
                while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) {
                    advance(); 
                }
                expect(TOKEN_RPAREN);
            } else {
                // import std or import "pkg"
                advance(); 
                while(match(TOKEN_COMMA)) advance(); // import a, b
            }
        }
        else if (t->type == TOKEN_EXPORT) {
            advance();
            if (match(TOKEN_LPAREN)) {
                while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) {
                    advance();
                }
                expect(TOKEN_RPAREN);
            } else {
                advance(); // export symbol
            }
        }
        else if (t->type == TOKEN_CONST) {
             // const ( ... ) OR const X = ...
             advance();
             if (match(TOKEN_LPAREN)) {
                 while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) {
                     // Ident [= val] [, or newline]
                     if (current()->type == TOKEN_IDENTIFIER) {
                         ASTNode* node = ast_new(AST_CONST_DECL);
                         strcpy(node->text, current()->text);
                         advance();
                         
                         // Check for = val
                         if (match(TOKEN_ASSIGN)) {
                             // Check enum
                             if (match(TOKEN_ENUM)) {
                                 ASTNode* en = ast_new(AST_ENUM_DECL);
                                 if (match(TOKEN_LPAREN)) {
                                      // enum(start)
                                      en->children[en->child_count++] = parse_expression();
                                      expect(TOKEN_RPAREN);
                                 }
                                 node->children[node->child_count++] = en;
                             } else {
                                 node->children[node->child_count++] = parse_expression();
                             }
                         } else {
                             // Implicit enum or just declaration?
                             // Assume enum decl in const block if no value 
                             ASTNode* en = ast_new(AST_ENUM_DECL);
                             node->children[node->child_count++] = en;
                         }
                         (*out_ast)->children[(*out_ast)->child_count++] = node;
                         match(TOKEN_COMMA);
                     } else {
                         advance(); // skip unknown in block
                     }
                 }
                 expect(TOKEN_RPAREN);
             } else {
                 // const X = ...
                 if (expect(TOKEN_IDENTIFIER)) {
                     ASTNode* node = ast_new(AST_CONST_DECL);
                     strcpy(node->text, tokens.tokens[pos-1].text);
                     if (match(TOKEN_ASSIGN)) {
                         node->children[node->child_count++] = parse_expression();
                     }
                     (*out_ast)->children[(*out_ast)->child_count++] = node;
                 }
             }
        } 
        else if (t->type == TOKEN_UNION) {
            advance();
            if (expect(TOKEN_IDENTIFIER)) {
                ASTNode* node = ast_new(AST_UNION_DECL);
                strcpy(node->text, tokens.tokens[pos-1].text);
                expect(TOKEN_LBRACE);
                while(current()->type!=TOKEN_RBRACE && current()->type!=TOKEN_EOF) {
                     ASTNode* field = parse_statement(); // Reusing var parsing
                     if (field) node->children[node->child_count++] = field;
                }
                expect(TOKEN_RBRACE);
                (*out_ast)->children[(*out_ast)->child_count++] = node;
            }
        }
        else if (t->type == TOKEN_STRUCT) {
            advance();
            if (expect(TOKEN_IDENTIFIER)) {
                ASTNode* node = ast_new(AST_STRUCT_DECL);
                strcpy(node->text, tokens.tokens[pos-1].text);
                
                if (match(TOKEN_LBRACE)) {
                    while(current()->type!=TOKEN_RBRACE && current()->type!=TOKEN_EOF) {
                         // Check for method decl inside struct: method name()
                         if (match(TOKEN_METHOD)) {
                             // method ident()
                             if (expect(TOKEN_IDENTIFIER)) {
                                 // Add method decl to struct?
                                 // For C codegen, maybe just ignore or handle as func pointer?
                                 // Spec says methods are functions with 'self'.
                                 // Here just declaration.
                                 if (match(TOKEN_LPAREN)) expect(TOKEN_RPAREN);
                             }
                         } else {
                             ASTNode* field = parse_statement();
                             if (field) node->children[node->child_count++] = field;
                         }
                    }
                    expect(TOKEN_RBRACE);
                    // Handle trailing optional semicolon
                    match(TOKEN_SEMICOLON);
                    (*out_ast)->children[(*out_ast)->child_count++] = node;
                } else {
                     // struct Name varName ...
                     // It's a variable declaration. "struct Point p1 = ..."
                     // Rewind and let generic decl handler handle it?
                     pos -= 2; // un-consume ID and STRUCT
                     goto parse_decl;
                }
            }
        }
        else if (t->type == TOKEN_ALIAS) {
             ASTNode* stmt = parse_statement();
             if (stmt) (*out_ast)->children[(*out_ast)->child_count++] = stmt;
        }
        else {
parse_decl:;
            // Variable or Function declaration
            // int x; void f();
            // Check if it starts with a type
             if (t->type == TOKEN_INT || t->type == TOKEN_VOID || t->type == TOKEN_STRING || 
                 t->type == TOKEN_BOOL || t->type == TOKEN_FLOAT || t->type == TOKEN_DOUBLE ||
                 t->type == TOKEN_BYTE || t->type == TOKEN_UBYTE ||
                 t->type == TOKEN_SHORT || t->type == TOKEN_USHORT ||
                 t->type == TOKEN_UINT || t->type == TOKEN_LONG || t->type == TOKEN_ULONG ||
                 t->type == TOKEN_WCHAR || t->type == TOKEN_MAP || t->type == TOKEN_STRUCT || 
                 t->type == TOKEN_VAR || t->type == TOKEN_IDENTIFIER) {
                     
                 // Parse type info
                 char type_name[256];
                 // int is_struct = 0; // UNUSED
                 if (t->type == TOKEN_STRUCT) {
                     // is_struct = 1;
                     advance();
                     if (current()->type == TOKEN_IDENTIFIER) {
                         sprintf(type_name, "struct %s", current()->text);
                         advance();
                     } else {
                         // struct { ... } ?
                         strcpy(type_name, "struct");
                     }
                 } else {
                     strcpy(type_name, t->text);
                     advance();
                     // Check array [] in type? "int[] x" or "int[16] x"
                     if (match(TOKEN_LBRACKET)) {
                         while(current()->type!=TOKEN_RBRACKET && current()->type!=TOKEN_EOF) advance();
                         expect(TOKEN_RBRACKET);
                         strcat(type_name, "[]");
                     }
                 }
                 
                 if (current()->type == TOKEN_IDENTIFIER) {
                     char name[64];
                     strcpy(name, current()->text);
                     advance();
                     
                     if (current()->type == TOKEN_LPAREN) {
                         // Function definition: Type Name(...) { ... }
                         // OR Prototype: Type Name(...);
                         ASTNode* func = ast_new(AST_FUNCTION);
                         strcpy(func->text, name); // Function name
                         
                         // Add return type? AST_FUNCTION usually implies int or void in old codegen?
                         // Codegen assumes "int main" if AST_FUNCTION?
                         // I need to update codegen for generic functions.
                         // Let's store return type as first child? 
                         // Or proper structure. 
                         // Current AST_FUNCTION just has children (body).
                         // I should add return type and args as children.
                         
                         // Child 0: Return Type
                         ASTNode* ret_node = ast_new(AST_IDENTIFIER);
                         strcpy(ret_node->text, type_name);
                         func->children[func->child_count++] = ret_node;
                         
                         expect(TOKEN_LPAREN);
                         // Parse Args
                         // arg: Type Name
                         while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) {
                              if (current()->type == TOKEN_COMMA) { advance(); continue; }
                              if (current()->type == TOKEN_CONST) advance(); // skip const in args for now
                              
                              char arg_type[256];
                              if (current()->type == TOKEN_STRUCT) {
                                  advance();
                                  sprintf(arg_type, "struct %s", current()->text);
                                  advance();
                              } else {
                                  strcpy(arg_type, current()->text);
                                  advance();
                              }
                              // brackets?
                              if (match(TOKEN_LBRACKET)) { 
                                  while(current()->type!=TOKEN_RBRACKET && current()->type!=TOKEN_EOF) advance();
                                  expect(TOKEN_RBRACKET); 
                                  strcat(arg_type, "[]"); 
                              }
                              
                              if (current()->type == TOKEN_IDENTIFIER) {
                                  ASTNode* lb = match(TOKEN_LBRACKET) ? ast_new(AST_NUMBER) : NULL; // check array after name?
                                  if (lb) match(TOKEN_RBRACKET);
                                  
                                  ASTNode* arg = ast_new(AST_VAR_DECL);
                                  strcpy(arg->text, current()->text);
                                  advance();
                                  
                                  // Add type to arg
                                  ASTNode* at = ast_new(AST_IDENTIFIER);
                                  strcpy(at->text, arg_type);
                                  
                                  // Check array after name
                                  int is_arr = 0;
                                  if (match(TOKEN_LBRACKET)) {
                                      while(current()->type!=TOKEN_RBRACKET && current()->type!=TOKEN_EOF) advance();
                                      expect(TOKEN_RBRACKET); 
                                      is_arr = 1;
                                  }
                                  
                                  if (is_arr) strcat(at->text, "[]"); // array param
                                  arg->children[arg->child_count++] = NULL; // No init
                                  arg->children[arg->child_count++] = at;
                                  
                                  func->children[func->child_count++] = arg;
                              }
                         }
                         expect(TOKEN_RPAREN);
                         
                         if (current()->type == TOKEN_LBRACE) {
                             ASTNode* body = parse_block();
                             func->children[func->child_count++] = body;
                             (*out_ast)->children[(*out_ast)->child_count++] = func;
                         } else {
                             // Prototype (semicolon or newline)
                             // Ignore prototypes for AST? Or emit decl?
                             // Emit decl.
                             // Add dummy body?
                             // Mark as prototype?
                             // For now skip prototypes.
                         }
                     } else {
                         // Variable Declaration: Type Name [= ...]
                         ASTNode* var = ast_new(AST_VAR_DECL);
                         strcpy(var->text, name);
                         ASTNode* init = NULL;
                         if (match(TOKEN_ASSIGN)) {
                             init = parse_expression();
                         } else {
                             init = ast_new(AST_NUMBER);
                             strcpy(init->text, "0");
                         }
                         var->children[var->child_count++] = init;
                         ASTNode* type_node = ast_new(AST_IDENTIFIER);
                         strcpy(type_node->text, type_name);
                         // check array
                         if (match(TOKEN_LBRACKET)) {
                             if (current()->type != TOKEN_RBRACKET) {
                                  // int arr[10]
                                  // size?
                                  while(current()->type!=TOKEN_RBRACKET && current()->type!=TOKEN_EOF) advance();
                             }
                             expect(TOKEN_RBRACKET);
                             strcat(type_node->text, "[]");
                         }
                         var->children[var->child_count++] = type_node;
                         (*out_ast)->children[(*out_ast)->child_count++] = var;
                         match(TOKEN_SEMICOLON); // optional ;
                     }
                 } else {
                    // Just type?
                    advance(); 
                 }
             } else {
                 advance(); // unknown top level
             }
        }
    }
    
    return 0;
}
