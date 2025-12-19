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
    n->source_line = (pos < tokens.count) ? tokens.tokens[pos].line : 0;
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
    
    // Handle NOT operators (logical and bitwise)
    if (t->type == TOKEN_NOT || t->type == TOKEN_TILDE) {
        TokenType op_type = t->type;  // Save before advancing
        advance();
        ASTNode* not_node = ast_new(AST_UNARY_OP);
        strcpy(not_node->text, (op_type == TOKEN_NOT) ? "!" : "~");
        not_node->children[not_node->child_count++] = parse_primary();
        return not_node;
    }
    
    if (t->type == TOKEN_IDENTIFIER) {
        node = ast_new(AST_IDENTIFIER);
        strcpy(node->text, t->text);
        advance();
        
        // Check for postfix operations: member access, method call, array access, function call
        while (1) {
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
                        // Trailing closure support
                        if (current()->type == TOKEN_LBRACE) {
                             call->children[call->child_count++] = parse_block();    
                        }
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
                strcpy(call->text, node->text); // Use previous node text as name if simple ident?
                // Wait, if node is not simple ident, this AST_CALL structure might be insufficient if it assumes text name.
                // Existing AST_CALL relies on text name for function?
                // If node is expression (arr[0]()), AST_CALL logic in codegen says: "fprintf(f, "%s(", node->text);"
                // So it assumes text is function name.
                // This means indirect calls are not fully supported by AST/codegen yet?
                // But ident(...) uses AST_CALL.
                // If I have `ident.field(...)`, it is AST_CALL? No, that's not valid syntax unless `field` is function pointer.
                // But `ident.method(...)` is AST_METHOD_CALL.
                // Let's preserve existing logic: if Identifier(...) -> Call. 
                // If I chain `ident[0](...)`, logic breaks if AST_CALL expects name.
                // For now, I'll stick to basic chaining.
                // Use "call" as text if complex? Or keep node text?
                // Let's assume chaining calls is rare or handled elsewhere.
                // But `ident(...)` in the loop:
                // If first loop iteration (node is IDENTIFIER): works.
                // If second iteration (node is Access): `arr[0](...)`.
                // Codegen for AST_CALL prints `node->text` + parens.
                // If `node` is Access, `node->text` might be empty or partial.
                // But standard C call is `expr(...)`.
                // My Codegen requires rewrite for generic call?
                // Line 288 codegen: `fprintf(f, "%s(", node->text);`
                // Yes, it assumes name.
                // So indirect calls are broken in codegen. 
                // But `examples/come_all.co` uses straightforward calls.
                // `args[1].byte_array()` is `MethodCall` on `ArrayAccess`. Codegen handles this (MethodCall uses children[0] as receiver).
                // So MethodCall works fine.
                // FunctionCall `ident(...)` works fine.
                // Indirect call not needed for this fix.
                
                // However, I need to put `AST_CALL` logic here correctly.
                // If I wrap `node` in `AST_CALL`, `node` becomes child?
                // Existing logic: `strcpy(call->text, node->text); free(node); node = call;`
                // It DISCARDS the identifier node and uses text!
                // This implies `ident(...)` is special.
                // If I have `arr[0](...)`, `node` is `arr[0]` (Access).
                // I cannot discard it.
                // But for now, let's allow it only if `node` is Identifier? 
                // Or just keep logic but warn/fix if complex.
                // Given `come_all.co` doesn't do indirect calls, I will reuse the logic but only applies to Identifier effectively?
                // But wait, `add_n_compare(i, s)` is a call.
                // `ident` is parsed. Loop starts.
                // PAREN matches.
                // `node` is IDENTIFIER `add_n_compare`.
                // We create `AST_CALL`, copy text "add_n_compare", free `node`.
                // THIS WORKS.
                // So logic is fine for direct calls.
                
                if (node->type == AST_IDENTIFIER) {
                     ASTNode* call = ast_new(AST_CALL);
                     strcpy(call->text, node->text);
                     free(node); 
                     node = call;
                     
                     while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) {
                         node->children[node->child_count++] = parse_expression();
                         if (!match(TOKEN_COMMA)) break;
                     }
                     expect(TOKEN_RPAREN);
                } else {
                     // Error or generic call?
                     // For now just error or skip to avoid complex refactor.
                     // But we must consume parens.
                     printf("Indirect call not supported yet\n");
                     expect(TOKEN_RPAREN); // consume
                }
            }
            
            // Handle array access: ident[expr]
            else if (match(TOKEN_LBRACKET)) {
                ASTNode* index = parse_expression();
                expect(TOKEN_RBRACKET);
                
                ASTNode* access = ast_new(AST_ARRAY_ACCESS);
                access->children[access->child_count++] = node; // Array
                access->children[access->child_count++] = index; // Index
                node = access;
            } else if (match(TOKEN_INC)) {
                ASTNode* inc = ast_new(AST_POST_INC);
                inc->children[inc->child_count++] = node;
                node = inc;
            } else if (match(TOKEN_DEC)) {
                ASTNode* dec = ast_new(AST_POST_DEC);
                dec->children[dec->child_count++] = node;
                node = dec;
            } else {
                break;
            }
        }
    } else if (t->type == TOKEN_STRING_LITERAL) {
        node = ast_new(AST_STRING_LITERAL);
        char combined[4096] = ""; // Large buffer for concatenated strings
        while (current()->type == TOKEN_STRING_LITERAL) {
             strcat(combined, current()->text);
             advance();
        }
        strcpy(node->text, combined);
        
        // Handle method call on string literal: "foo" "bar".method()
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
        // Map/Struct initializer: { k: v, ... } or { } or { .field = val, ... }
        node = ast_new(AST_AGGREGATE_INIT);
        strcpy(node->text, "MAP");
        while (current()->type != TOKEN_RBRACE && current()->type != TOKEN_EOF) {
             // Check for designated initializer: .field = value
             if (match(TOKEN_DOT)) {
                 if (current()->type == TOKEN_IDENTIFIER) {
                     // Create a special marker node for designated initializer
                     ASTNode* desig = ast_new(AST_IDENTIFIER);
                     // Store ".field" as text - limit to 126 chars to leave room for "." and null
                     snprintf(desig->text, sizeof(desig->text), ".%.*s", 126, current()->text);
                     advance(); // consume identifier
                     
                     if (match(TOKEN_ASSIGN)) {
                         // Now parse the value
                         ASTNode* value = parse_expression();
                         
                         // Create an assignment-like node to represent .field = value
                         ASTNode* pair = ast_new(AST_ASSIGN);
                         pair->children[pair->child_count++] = desig;
                         pair->children[pair->child_count++] = value;
                         
                         node->children[node->child_count++] = pair;
                     }
                 }
             } else {
                 node->children[node->child_count++] = parse_expression();
             }
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
        // or struct Point p = ...
        char type_name[128];
        strcpy(type_name, t->text);
        advance();
        
        // Special handling for struct: "struct Type varname"
        if (strcmp(type_name, "struct") == 0 && current()->type == TOKEN_IDENTIFIER) {
            // Consume the struct type name
            strcat(type_name, " ");
            strcat(type_name, current()->text);
            advance();
        }
        
        // Check for array type: int[] x
        while (match(TOKEN_LBRACKET)) {
             while(current()->type != TOKEN_RBRACKET && current()->type != TOKEN_EOF) advance();
             expect(TOKEN_RBRACKET);
             strcat(type_name, "[]");
        }

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
        // Check for assignments FIRST (before type declarations)
        // Check lookahead for assignment operators
        if (pos + 1 < tokens.count && 
            (tokens.tokens[pos+1].type == TOKEN_ASSIGN || 
             tokens.tokens[pos+1].type == TOKEN_PLUS_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_MINUS_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_STAR_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_SLASH_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_AND_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_OR_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_XOR_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_LSHIFT_ASSIGN ||
             tokens.tokens[pos+1].type == TOKEN_RSHIFT_ASSIGN)) {
              
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
        
        // Then check for custom type declaration: MyType x ...
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
        } else {
             // Maybe function call or expression statement?
             // Treat as expression statement
             ASTNode* expr = parse_expression();
             
             // Check for assignment after parsing expression (e.g. member/array access LHS)
             if (pos < tokens.count && 
                 (tokens.tokens[pos].type == TOKEN_ASSIGN || 
                  tokens.tokens[pos].type == TOKEN_PLUS_ASSIGN ||
                  tokens.tokens[pos].type == TOKEN_MINUS_ASSIGN ||
                  tokens.tokens[pos].type == TOKEN_STAR_ASSIGN ||
                  tokens.tokens[pos].type == TOKEN_SLASH_ASSIGN ||
                  tokens.tokens[pos].type == TOKEN_AND_ASSIGN ||
                  tokens.tokens[pos].type == TOKEN_OR_ASSIGN ||
                  tokens.tokens[pos].type == TOKEN_XOR_ASSIGN ||
                  tokens.tokens[pos].type == TOKEN_LSHIFT_ASSIGN ||
                  tokens.tokens[pos].type == TOKEN_RSHIFT_ASSIGN)) {
                  
                   ASTNode* assign = ast_new(AST_ASSIGN);
                   strcpy(assign->text, tokens.tokens[pos].text);
                   match(tokens.tokens[pos].type); // consume op
                   
                   assign->children[assign->child_count++] = expr;
                   assign->children[assign->child_count++] = parse_expression();
                   
                   if (current()->type == TOKEN_SEMICOLON) advance();
                   return assign;
             }
             
             if (current()->type == TOKEN_SEMICOLON) advance();
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
                is_array = 1;
                strcat(type_name, "[");
                if (current()->type != TOKEN_RBRACKET) {
                    strcat(type_name, current()->text);
                    while (current()->type != TOKEN_RBRACKET && current()->type != TOKEN_EOF) advance();
                }
                strcat(type_name, "]");
                expect(TOKEN_RBRACKET);
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
                      
                      char type_text[128];
                      strcpy(type_text, tokens.tokens[pos-1].text);
                      
                      // Special case: if type is "struct", also consume the struct name
                      if (strcmp(type_text, "struct") == 0 && current()->type == TOKEN_IDENTIFIER) {
                          strcat(type_text, " ");
                          strcat(type_text, current()->text);
                          advance();
                      }
                      
                      ASTNode* node = ast_new(AST_TYPE_ALIAS);
                      strcpy(node->text, alias_name);
                      
                      ASTNode* target = ast_new(AST_IDENTIFIER);
                      strcpy(target->text, type_text);
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

static void parse_import(ASTNode* program) {
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

static void parse_export(ASTNode* program) {
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

static void parse_const(ASTNode* program) {
     // const ( ... ) OR const X = ...
     advance();
     if (match(TOKEN_LPAREN)) {
         ASTNode* group = ast_new(AST_CONST_GROUP);
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
                 group->children[group->child_count++] = node;
                 match(TOKEN_COMMA);
             } else {
                 advance(); // skip unknown in block
             }
         }
         expect(TOKEN_RPAREN);
         program->children[program->child_count++] = group;
     } else {
         // const X = ...
         if (expect(TOKEN_IDENTIFIER)) {
             ASTNode* node = ast_new(AST_CONST_DECL);
             strcpy(node->text, tokens.tokens[pos-1].text);
             if (match(TOKEN_ASSIGN)) {
                 node->children[node->child_count++] = parse_expression();
             }
             program->children[program->child_count++] = node;
         }
     }
}

static void parse_union(ASTNode* program) {
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
        program->children[program->child_count++] = node;
    }
}

static void parse_struct(ASTNode* program) {
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
            program->children[program->child_count++] = node;
         }
    }
}

static int is_type_token(TokenType type) {
    return (type == TOKEN_INT || type == TOKEN_VOID || type == TOKEN_STRING || 
            type == TOKEN_BOOL || type == TOKEN_FLOAT || type == TOKEN_DOUBLE ||
            type == TOKEN_BYTE || type == TOKEN_UBYTE ||
            type == TOKEN_SHORT || type == TOKEN_USHORT ||
            type == TOKEN_LONG || type == TOKEN_ULONG ||
            type == TOKEN_WCHAR || type == TOKEN_MAP || type == TOKEN_VAR);
}

static void parse_alias(ASTNode* program) {
    advance(); // alias
    // alias X = Y or alias X(args) = Y
    if (expect(TOKEN_IDENTIFIER)) {
        char name[256];
        strcpy(name, tokens.tokens[pos-1].text);

        if (match(TOKEN_LPAREN)) {
            // Macro alias: alias SQUARE(x) = ...
             while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) advance(); // skip args
             expect(TOKEN_RPAREN);
             // Assume macro, skip for now or treat as const?
             // Not creating AST yet to avoid codegen issues with macros
             if (match(TOKEN_ASSIGN)) parse_expression();
        }
        else if (match(TOKEN_ASSIGN)) {
             // alias X = Y
             // Check if Y is type
             if (is_type_token(current()->type) || current()->type == TOKEN_STRUCT || current()->type == TOKEN_UNION) {
                  printf("DEBUG: Parsing Type Alias %s\n", name);
                  // Type Alias
                  ASTNode* node = ast_new(AST_TYPE_ALIAS);
                  strcpy(node->text, name);
                  
                  ASTNode* typeNode = ast_new(AST_IDENTIFIER);
                  
                  if (current()->type == TOKEN_STRUCT) {
                      advance();
                      char t[256];
                      snprintf(t, sizeof(t), "struct %s", current()->text);
                      strcpy(typeNode->text, t);
                      advance();
                  } else if (current()->type == TOKEN_UNION) {
                      advance();
                      char t[256];
                      snprintf(t, sizeof(t), "union %s", current()->text);
                      strcpy(typeNode->text, t);
                      advance();
                  } else {
                      strcpy(typeNode->text, current()->text);
                      advance();
                  }
                  
                  node->children[0] = typeNode;
                  node->child_count = 1;
                  program->children[program->child_count++] = node;
             } else {
                  // Constant/Expression Alias
                  // alias MAX = 5
                  ASTNode* node = ast_new(AST_CONST_DECL);
                  strcpy(node->text, name);
                  node->children[0] = parse_expression();
                  node->child_count = 1;
                  program->children[program->child_count++] = node;
             }
        }
    }
}

static void parse_top_level_decl(ASTNode* program) {
    Token* t = current();
    
    char type_name[256] = {0};
    int is_method = 0;
    int implicit_type = 0;

    // Variable or Function declaration
    // Check if it starts with a type OR is an implicit function definition (e.g. main() or myfunc())
    if (is_type_token(t->type) || t->type == TOKEN_MAIN || 
        (t->type == TOKEN_IDENTIFIER && tokens.tokens[pos+1].type == TOKEN_LPAREN)) {
             
         // Parse type info
         // int is_struct = 0; // UNUSED

         if (t->type == TOKEN_LPAREN) {
              // Parse tuple type: (int, string)
              advance(); // (
              strcpy(type_name, "(");
              while (current()->type != TOKEN_RPAREN && current()->type != TOKEN_EOF) {
                  strcat(type_name, current()->text);
                  advance();
                  if (match(TOKEN_COMMA)) strcat(type_name, ",");
                  else break;
              }
              expect(TOKEN_RPAREN);
              strcat(type_name, ")");
         } else if (t->type == TOKEN_STRUCT) {
             // is_struct = 1;
             advance();
             if (current()->type == TOKEN_IDENTIFIER) {
                 sprintf(type_name, "struct %s", current()->text);
                 advance();
             } else {
                 // struct { ... } ?
                 strcpy(type_name, "struct");
             }
         } else if (t->type == TOKEN_MAIN || (t->type == TOKEN_IDENTIFIER && tokens.tokens[pos+1].type == TOKEN_LPAREN)) {
             // "main()" or "func()" -> Implicit return type
             strcpy(type_name, (strcmp(t->text, "main")==0) ? "int" : "void");
             implicit_type = 1;
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
         
         // Handle explicit vs implicit flow
         char name[256];
         int is_func_def = 0;
         
         if (implicit_type) {
             strcpy(name, t->text);
             advance();
             is_func_def = 1; 
         } else {
             if (current()->type == TOKEN_IDENTIFIER || current()->type == TOKEN_MAIN) {
                 strcpy(name, current()->text);
                 advance();
                 
                 // Check for "Struct.Method" syntax
                 if (current()->type == TOKEN_DOT) {
                     advance(); // consume DOT
                     if (expect(TOKEN_IDENTIFIER)) {
                         char method_name[64];
                         char struct_name[64];
                         
                         strcpy(method_name, tokens.tokens[pos-1].text);
                         
                         // Determine Struct Name (it's in 'name' currently)
                         strcpy(struct_name, name);
                         
                         // Mangled Name: Struct_Method
                         sprintf(name, "%s_%s", struct_name, method_name);
                         is_method = 1;
                         
                         // We need to inject 'self' argument. 
                         // But we are outside the func parsing block.
                         // We can abuse 'implicit_type' flag or add a new one?
                         // Let's add 'is_method_def' to the loop scope? 
                         // No, variables must be declared at top of block in C90 (if strictly followed, but here we can mix).
                         // We need to pass this info to the arg parsing section.
                         // Since we can't easily add a variable to outer scope without refactoring, 
                         // let's assume if name has underscore and looks like struct method, we might want to check?
                         // Better: Refactor `parse_top_level_decl` variable declarations.
                     }
                 }

                 is_func_def = 1; 
             }
         }
         
         if (is_func_def) {
             if (current()->type == TOKEN_LPAREN) {
                 // Function definition: Type Name(...) { ... }
                 // OR Prototype: Type Name(...);
                 ASTNode* func = ast_new(AST_FUNCTION);
                 strcpy(func->text, name); // Function name
                 
                 // Child 0: Return Type
                 ASTNode* ret_node = ast_new(AST_IDENTIFIER);
                 strcpy(ret_node->text, type_name);
                 func->children[func->child_count++] = ret_node;
                 
                 expect(TOKEN_LPAREN);
                 
                 // Inject 'self' argument if method
                 if (is_method) {
                      // Extract struct name from mangled name or re-derive?
                      // Name is "Struct_Method". We need "Struct".
                      char struct_name[64];
                      char* underscore;
                      ASTNode* self_arg;
                      ASTNode* type_node;

                      strcpy(struct_name, name);
                      underscore = strrchr(struct_name, '_');
                      if (underscore) *underscore = 0;
                      
                      self_arg = ast_new(AST_VAR_DECL);
                      strcpy(self_arg->text, "self");
                      self_arg->children[self_arg->child_count++] = NULL; // No init
                      
                      type_node = ast_new(AST_IDENTIFIER);
                      sprintf(type_node->text, "struct %s*", struct_name); // Pointer to struct
                      self_arg->children[self_arg->child_count++] = type_node;
                      
                      func->children[func->child_count++] = self_arg;
                      
                      // Check for comma if there are more args
                      if (current()->type != TOKEN_RPAREN) {
                          // We don't need to check comma here because the loop below expects args?
                          // But wait, "byte TCP_ADDR.nport()". No args in parens.
                          // parser loop: while (current != RPAREN).
                          // So it will skip loop.
                          // But if there ARE args: "func(a)".
                          // We injected self. So effectively "func(self, a)".
                          // The loop handles "a".
                          // Does loop require comma at start?
                          // Loop: if (COMMA) advance.
                          // So if user wrote "func(a)", we injected self.
                          // Next token is "a". Loop sees "a". Logic parses "a".
                          // Correct.
                          // But what if user wrote "func()"?
                          // injected self. Next is RPAREN. Loop terminates.
                          // Correct.
                      }
                 }

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
                     program->children[program->child_count++] = func;
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
                 
                 if (implicit_type && current()->type != TOKEN_LPAREN) {
                      printf("Error: Implicit type only supported for functions (e.g. 'main()'). Got '%s' after '%s'\n", current()->text, name);
                 }

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
                 program->children[program->child_count++] = var;
                 match(TOKEN_SEMICOLON); // optional ;
             }
         }
     } else {
         advance(); // unknown top level
     }
}

int parse_file(const char* filename, ASTNode** out_ast) {
    if (lex_file(filename, &tokens) != 0) return 1;
    pos = 0;
    
    *out_ast = ast_new(AST_PROGRAM);
    
    while (pos < tokens.count) {
        Token* t = current();
        if (t->type == TOKEN_EOF) break;
        
        switch (t->type) {
            case TOKEN_MODULE:
                advance(); // module
                if (current()->type == TOKEN_MAIN) advance();
                else if (current()->type == TOKEN_IDENTIFIER) advance();
                // ignore module decl for AST
                break;
            case TOKEN_IMPORT:
                parse_import(*out_ast);
                break;
            case TOKEN_EXPORT:
                parse_export(*out_ast);
                break;
            case TOKEN_CONST:
                parse_const(*out_ast);
                break;
            case TOKEN_UNION:
                parse_union(*out_ast);
                break;
            case TOKEN_STRUCT:
                if (pos + 2 < tokens.count && tokens.tokens[pos+1].type == TOKEN_IDENTIFIER && tokens.tokens[pos+2].type == TOKEN_LBRACE) {
                    parse_struct(*out_ast);
                } else {
                    parse_top_level_decl(*out_ast);
                }
                break;
            case TOKEN_ALIAS:
                parse_alias(*out_ast);
                break;
            default:
                parse_top_level_decl(*out_ast);
                break;
        }
    }
    
    return 0;
}
