#include <stdio.h>
#include <string.h>
#include "codegen.h"

/* small helper: write indentation spaces */
static void emit_indent(FILE* f, int indent_spaces) {
    for (int i = 0; i < indent_spaces; i++) fputc(' ', f);
}

/* small helper: emit a valid C string literal from raw text in node->text
   - wraps in double quotes
   - escapes any embedded " characters
   - preserves backslashes as-is (so \n, \t, etc. in text keep working)
   - converts literal newlines (if present) to \n
*/
static void emit_c_string_literal(FILE* f, const char* s) {
    fputc('"', f);
    for (const char* p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '"') {
            fputs("\\\"", f);
        } else if (c == '\n') {
            fputs("\\n", f);
        } else {
            fputc(c, f);
        }
    }
    fputc('"', f);
}

/* forward decl */
static void generate_node(FILE* f, ASTNode* node, int indent);

/* generate the top-level program: just walk children */
static void generate_program(FILE* f, ASTNode* node) {
    for (int i = 0; i < node->child_count; i++) {
        generate_node(f, node->children[i], 0);
        fputc('\n', f);
    }
}

/* recursive codegen */
static void generate_node(FILE* f, ASTNode* node, int indent) {
    switch (node->type) {
        case AST_PROGRAM:
            generate_program(f, node);
            break;

        case AST_FUNCTION: {
            // For MVP we only support main with (argc, argv)
            fprintf(f, "int main(int argc, char* argv[]) {\n");
            for (int i = 0; i < node->child_count; i++) {
                generate_node(f, node->children[i], 4);
            }
            fprintf(f, "}\n");
            break;
        }

        case AST_IF: {
            // node->text holds the C condition string, e.g. "argc > 1"
            emit_indent(f, indent);
            fprintf(f, "if (%s) {\n", node->text);

            // Emit then-body (all non-ELSE children)
            int else_index = -1;
            for (int i = 0; i < node->child_count; i++) {
                if (node->children[i]->type == AST_ELSE) {
                    else_index = i;
                } else {
                    generate_node(f, node->children[i], indent + 4);
                }
            }

            emit_indent(f, indent);
            fprintf(f, "}");

            // If we have an else, print it properly as "} else { ... }"
            if (else_index >= 0) {
                fprintf(f, " else {\n");
                // In our design, the AST_ELSE node contains its statements as children
                for (int j = 0; j < node->children[else_index]->child_count; j++) {
                    generate_node(f, node->children[else_index]->children[j], indent + 4);
                }
                emit_indent(f, indent);
                fprintf(f, "}\n");
            } else {
                fputc('\n', f);
            }
            break;
        }

        case AST_ELSE:
            // Else blocks are emitted by AST_IF to ensure braces are paired correctly.
            // Nothing to do here.
            break;

        case AST_PRINTF: {
            emit_indent(f, indent);
            fputs("printf(", f);
            // format string
            emit_c_string_literal(f, node->text);
            // very simple arg inference: if the format contains "%s", pass argv[1]
            if (strstr(node->text, "%s") != NULL) {
                fputs(", argv[1]", f);
            }
            fputs(");\n", f);
            break;
        }

        case AST_RETURN: {
            emit_indent(f, indent);
            fprintf(f, "return %s;\n", node->text);
            break;
        }

        default:
            // Unknown/unsupported node type: ignore gracefully for now
            break;
    }
}

/* Main codegen entry */
int generate_c_from_ast(ASTNode* ast, const char* out_file) {
    FILE* f = fopen(out_file, "w");
    if (!f) return 1;

    fprintf(f, "#include <stdio.h>\n\n");

    // Expect AST_PROGRAM at root; still robustly handle if not.
    if (ast->type == AST_PROGRAM) {
        generate_program(f, ast);
    } else {
        generate_node(f, ast, 0);
    }

    fclose(f);
    return 0;
}

