#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "codegen.h"
#include "ast.h"

/* small helper: write indentation spaces */
static void emit_indent(FILE* f, int indent_spaces) {
    for (int i = 0; i < indent_spaces; i++) fputc(' ', f);
}

static void emit_c_string_literal(FILE* f, const char* s) {
    // s includes quotes from lexer?
    // Parser copies text. Lexer includes quotes for TOKEN_STRING_LITERAL?
    // Lexer: "foo" -> text="foo" (with quotes).
    // So just print it.
    fprintf(f, "%s", s);
}

static void generate_expression(FILE* f, ASTNode* node) {
    if (node->type == AST_STRING_LITERAL) {
        // Emit raw literal. Context handles wrapping.
        emit_c_string_literal(f, node->text);
    } else if (node->type == AST_BOOL_LITERAL) {
        fprintf(f, "%s", node->text); // true or false
    } else if (node->type == AST_NUMBER) {
        fprintf(f, "%s", node->text);
    } else if (node->type == AST_IDENTIFIER) {
        fprintf(f, "%s", node->text);
    } else if (node->type == AST_ARRAY_ACCESS) {
        // (arr)->items[index]
        fprintf(f, "(");
        generate_expression(f, node->children[0]);
        fprintf(f, ")->items[");
        generate_expression(f, node->children[1]);
        fprintf(f, "]");
    } else if (node->type == AST_METHOD_CALL) {
        char* method = node->text;
        char c_func[128];
        
        if (strcmp(method, "length") == 0) {
            strcpy(c_func, "come_string_list_len");
        } else if (strcmp(method, "len") == 0) {
            strcpy(c_func, "come_string_len");
        } else {
            sprintf(c_func, "come_string_%s", method);
        }
        
        fprintf(f, "%s(", c_func);
        
        // Handle join swap: receiver.join(list) -> join(list, receiver)
        if (strcmp(method, "join") == 0) {
             // Arg 0 (list)
             if (node->child_count > 1) generate_expression(f, node->children[1]);
             else fprintf(f, "NULL");
             fprintf(f, ", ");
             
             // Receiver (sep) - MUST WRAP if literal
             ASTNode* receiver = node->children[0];
             if (receiver->type == AST_STRING_LITERAL) {
                 fprintf(f, "come_string_new(NULL, ");
                 generate_expression(f, receiver);
                 fprintf(f, ")");
             } else {
                 generate_expression(f, receiver);
             }
        } else {
            // Normal order: receiver, args
            ASTNode* receiver = node->children[0];
            if (receiver->type == AST_STRING_LITERAL) {
                 fprintf(f, "come_string_new(NULL, ");
                 generate_expression(f, receiver);
                 fprintf(f, ")");
            } else {
                 generate_expression(f, receiver);
            }
            
            for (int i = 1; i < node->child_count; i++) {
                fputs(", ", f);
                ASTNode* arg = node->children[i];
                // Wrap args if method expects come_string_t*
                // cmp, casecmp expect come_string_t*
                if ((strcmp(method, "cmp") == 0 || strcmp(method, "casecmp") == 0) && arg->type == AST_STRING_LITERAL) {
                    fprintf(f, "come_string_new(NULL, ");
                    generate_expression(f, arg);
                    fprintf(f, ")");
                } else {
                    generate_expression(f, arg);
                }
            }
        }
        
        // Optional args
        if ((strcmp(method, "cmp") == 0 || strcmp(method, "casecmp") == 0) && node->child_count == 2) {
            fputs(", 0", f);
        }
        if (strcmp(method, "replace") == 0 && node->child_count == 3) {
            fputs(", 0", f);
        }
        if (strcmp(method, "regex_split") == 0 && node->child_count == 2) {
            fputs(", 0", f);
        }
        if (strcmp(method, "regex_replace") == 0 && node->child_count == 3) {
            fputs(", 0", f);
        }
        if ((strcmp(method, "trim") == 0 || strcmp(method, "ltrim") == 0 || strcmp(method, "rtrim") == 0) && node->child_count == 1) {
            fputs(", NULL", f);
        }
        
        fprintf(f, ")");
    } else if (node->type == AST_CALL) {
        generate_expression(f, node->children[0]);
        fprintf(f, " %s ", node->text);
        generate_expression(f, node->children[1]);
    }
}

static void generate_node(FILE* f, ASTNode* node, int indent);

static void generate_program(FILE* f, ASTNode* node) {
    for (int i = 0; i < node->child_count; i++) {
        generate_node(f, node->children[i], 0);
        fputc('\n', f);
    }
}

static void generate_node(FILE* f, ASTNode* node, int indent) {
    if (!node) return;
    switch (node->type) {
        case AST_PROGRAM:
            generate_program(f, node);
            break;

        case AST_FUNCTION: {
            fprintf(f, "int main(int argc, char* argv[]) {\n");
            emit_indent(f, indent + 4);
            fprintf(f, "TALLOC_CTX* ctx = mem_talloc_new_ctx(NULL);\n");
            
            for (int i = 0; i < node->child_count; i++) {
                generate_node(f, node->children[i], indent + 4);
            }
            
            emit_indent(f, indent + 4);
            fprintf(f, "mem_talloc_free(ctx);\n");
            emit_indent(f, indent + 4);
            fprintf(f, "return 0;\n");
            fprintf(f, "}\n");
            break;
        }

        case AST_VAR_DECL: {
            ASTNode* type_node = node->children[1];
            ASTNode* init_expr = node->children[0];
            
            emit_indent(f, indent);
            if (strcmp(type_node->text, "string") == 0) {
                fprintf(f, "come_string_t* %s = ", node->text);
                if (init_expr->type == AST_STRING_LITERAL) {
                    fprintf(f, "come_string_new(ctx, ");
                    generate_expression(f, init_expr);
                    fprintf(f, ")");
                } else {
                    generate_expression(f, init_expr);
                }
                fprintf(f, ";\n");
            } else if (strcmp(type_node->text, "string[]") == 0) {
                fprintf(f, "come_string_list_t* %s = ", node->text);
                if (init_expr->type == AST_STRING_LITERAL && strcmp(init_expr->text, "\"__ARGS__\"") == 0) {
                    fprintf(f, "come_string_list_from_argv(ctx, argc, argv)");
                } else {
                    generate_expression(f, init_expr);
                }
                fprintf(f, ";\n");
                // Mark as potentially unused to avoid warnings
                emit_indent(f, indent);
                fprintf(f, "(void)%s;\n", node->text);
            } else if (strcmp(type_node->text, "bool") == 0) {
                fprintf(f, "bool %s = ", node->text);
                generate_expression(f, init_expr);
                fprintf(f, ";\n");
            } else {
                fprintf(f, "%s %s = ", type_node->text, node->text);
                generate_expression(f, init_expr);
                fprintf(f, ";\n");
            }
            break;
        }

        case AST_PRINTF: {
            emit_indent(f, indent);
            fputs("printf(", f);
            emit_c_string_literal(f, node->text);
            
            for (int i = 0; i < node->child_count; i++) {
                fputs(", ", f);
                ASTNode* arg = node->children[i];
                
                if (arg->type == AST_STRING_LITERAL) {
                    emit_c_string_literal(f, arg->text);
                } else if (arg->type == AST_IDENTIFIER) {
                     const char* str_vars[] = {"s", "upper", "lower", "repeated", "replaced", "trimmed", "ltrimmed", "rtrimmed", "joined", "expected", "alpha", "digits", "alnum", "space", "other", "parts", "groups", "regex_replaced", "email", "text", "custom_trim"};
                     int is_str = 0;
                     for(int k=0; k<sizeof(str_vars)/sizeof(char*); k++) {
                         if (strcmp(arg->text, str_vars[k]) == 0) is_str = 1;
                     }
                     
                     if (is_str) {
                         fprintf(f, "(%s ? %s->data : \"NULL\")", arg->text, arg->text);
                     } else {
                         generate_expression(f, arg);
                     }
                } else if (arg->type == AST_METHOD_CALL) {
                    char* m = arg->text;
                    if (strcmp(m, "upper")==0 || strcmp(m, "lower")==0 || strcmp(m, "repeat")==0 || strcmp(m, "replace")==0 || strcmp(m, "trim")==0 || strcmp(m, "ltrim")==0 || strcmp(m, "rtrim")==0 || strcmp(m, "join")==0 || strcmp(m, "substr")==0 || strcmp(m, "regex_replace")==0) {
                         fprintf(f, "(");
                         generate_expression(f, arg);
                         fprintf(f, ")->data");
                    } else {
                        // Cast to int for numeric results to satisfy printf %d
                        fprintf(f, "(int)(");
                        generate_expression(f, arg);
                        fprintf(f, ")");
                    }
                } else if (arg->type == AST_ARRAY_ACCESS) {
                    // ((arr)->items[index])->data
                    fprintf(f, "(");
                    generate_expression(f, arg);
                    fprintf(f, ")->data");
                } else {
                    generate_expression(f, arg);
                }
            }
            fputs(");\n", f);
            break;
        }

        case AST_IF: {
            emit_indent(f, indent);
            fprintf(f, "if (");
            generate_expression(f, node->children[0]);
            fprintf(f, ") {\n");
            generate_node(f, node->children[1], indent + 4);
            emit_indent(f, indent);
            fprintf(f, "}");
            if (node->child_count > 2) {
                fprintf(f, " else {\n");
                generate_node(f, node->children[2], indent + 4);
                emit_indent(f, indent);
                fprintf(f, "}\n");
            } else {
                fputc('\n', f);
            }
            break;
        }
        
        case AST_ELSE: {
            // Just generate the child statement
            generate_node(f, node->children[0], indent);
            break;
        }
        
        case AST_BLOCK: {
            for (int i = 0; i < node->child_count; i++) {
                generate_node(f, node->children[i], indent);
            }
            break;
        }

        case AST_RETURN: {
            emit_indent(f, indent);
            fprintf(f, "return ");
            if (node->child_count > 0) {
                generate_expression(f, node->children[0]);
            } else {
                fprintf(f, "0");
            }
            fprintf(f, ";\n");
            break;
        }
        
        case AST_METHOD_CALL: {
            emit_indent(f, indent);
            generate_expression(f, node);
            fprintf(f, ";\n");
            break;
        }

        default:
            break;
    }
}

int generate_c_from_ast(ASTNode* ast, const char* out_file) {
    FILE* f = fopen(out_file, "w");
    if (!f) return 1;

    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <string.h>\n");
    fprintf(f, "#include <stdbool.h>\n");
    fprintf(f, "#include \"string_module.h\"\n");
    fprintf(f, "#include \"mem/talloc.h\"\n\n");

    if (ast->type == AST_PROGRAM) {
        generate_program(f, ast);
    } else {
        generate_node(f, ast, 0);
    }

    fclose(f);
    return 0;
}
