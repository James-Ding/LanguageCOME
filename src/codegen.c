#include <stdio.h>
#include "codegen.h"

int codegen(ASTNode* ast, const char* out_file) {
    FILE* f = fopen(out_file, "w");
    if (!f) return 1;

    fprintf(f, "#include <stdio.h>\n\n");

    for (int i = 0; i < ast->child_count; i++) {
        ASTNode* fn = ast->children[i];
        if (fn->type != AST_FUNCTION) continue;

        fprintf(f, "int %s(int argc, char* argv[]) {\n", fn->text);

        for (int j = 0; j < fn->child_count; j++) {
            ASTNode* stmt = fn->children[j];
            if (stmt->type == AST_PRINTF) {
                fprintf(f, "    printf(\"%s\\n\");\n", stmt->text);
            }
        }

        fprintf(f, "    return 0;\n}\n\n");
    }

    fclose(f);
    return 0;
}

