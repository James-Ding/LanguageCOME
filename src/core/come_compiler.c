// src/come_compiler.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"

/* ---------- small utilities (local, no external deps) ---------- */

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

static int ends_with(const char *s, const char *suffix) {
    size_t ls = strlen(s), lt = strlen(suffix);
    return (ls >= lt) && memcmp(s + ls - lt, suffix, lt) == 0;
}

static void strip_suffix(char *s, const char *suffix) {
    size_t ls = strlen(s), lt = strlen(suffix);
    if (ls >= lt && memcmp(s + ls - lt, suffix, lt) == 0) {
        s[ls - lt] = '\0';
    }
}

/* dirname-like helper: copy directory part into out (no trailing slash unless root). */
static void path_dirname(const char *path, char *out, size_t outsz) {
    const char *slash = strrchr(path, '/');
    if (!slash) {
        snprintf(out, outsz, ".");
        return;
    }
    size_t n = (size_t)(slash - path);
    if (n >= outsz) n = outsz - 1;
    memcpy(out, path, n);
    out[n] = '\0';
}


/* create directory tree best-effort for POSIX. On Windows, no-op fallback. */
static void mkdir_p_for_file(const char *filepath) {
    char dir[1024];
    path_dirname(filepath, dir, sizeof(dir));
    if (strcmp(dir, ".") == 0 || strcmp(dir, "/") == 0) return;

    // Build path piece by piece
    char tmp[1024];
    tmp[0] = '\0';
    const char *p = dir;
    while (*p) {
        // copy up to next '/'
        const char *q = strchr(p, '/');
        size_t chunk = q ? (size_t)(q - p) : strlen(p);
        if (strlen(tmp) + (tmp[0] ? 1 : 0) + chunk + 1 >= sizeof(tmp)) break;

        if (tmp[0]) strcat(tmp, "/");
        strncat(tmp, p, chunk);

        if (mkdir(tmp, 0777) != 0 && errno != EEXIST) {
            // best effort only; ignore errors
            break;
        }
        if (!q) break;
        p = q + 1;
    }
}

/* run a shell command; return non-zero if failed */
static int run_cmd(const char *fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    // fprintf(stderr, "[CMD] %s\n", buf); // uncomment for debug
    int rc = system(buf);
    return rc;
}

/* ---------- CLI parsing ---------- */

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s build [-c] [-o <output>] <file.co>\n"
        "Options:\n"
        "  -c           Generate C code only (.co.c), do not compile\n"
        "  -o <output>  Output executable path/name (ignored with -c)\n",
        prog);
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "build") != 0) {
        usage(argv[0]);
        return 1;
    }

    int generate_only = 0;       // -c
    const char *out_path = NULL; // -o <file>
    const char *co_file = NULL;

    // Parse options: come build [-c] [-o out] file.co
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            generate_only = 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                die("Error: -o requires an output path");
            }
            out_path = argv[++i];
        } else if (argv[i][0] == '-') {
            die("Unknown option: %s", argv[i]);
        } else {
            co_file = argv[i];
        }
    }

    if (!co_file) {
        usage(argv[0]);
        return 1;
    }
    if (!ends_with(co_file, ".co")) {
        die("Input must be a .co file: %s", co_file);
    }

    /* Prepare filenames */
    char c_file[1024];
    char bin_file[1024];

    // Generated C file: <file>.co.c (same directory as input)
    snprintf(c_file, sizeof(c_file), "%s.c", co_file);

    // Default binary: remove ".co"
    if (out_path && !generate_only) {
        snprintf(bin_file, sizeof(bin_file), "%s", out_path);
    } else {
        snprintf(bin_file, sizeof(bin_file), "%s", co_file);
        strip_suffix(bin_file, ".co"); // from .../foo.co -> .../foo
    }

	ASTNode *ast = NULL;
	if (parse_file(co_file, &ast) != 0 || !ast) {
	    die("Parsing failed: %s", co_file);
	}

    /* 3) Codegen -> C file */
    // Ensure directory for c_file exists (same dir as .co; usually does)
    if (generate_c_from_ast(ast, c_file) != 0) {
        ast_free(ast);
        die("Code generation failed: %s", c_file);
    }

    if (generate_only) {
        printf("Generated C code: %s\n", c_file);
        ast_free(ast);
        return 0;
    }

    /* 4) Compile C -> executable */
    // Ensure directory for output exists if -o specified
    if (out_path) {
        mkdir_p_for_file(out_path);
    }

    // Compile with gcc
    // Note: you may want to add -I paths or link libs here later
    if (run_cmd("gcc -Wall -g \"%s\" -o \"%s\"", c_file, bin_file) != 0) {
        ast_free(ast);
        die("GCC compilation failed");
    }

    /* 5) Cleanup and finish */
    // Remove intermediate C file to keep tree clean
    // (If you want to keep it, comment out the line below.)
    if (remove(c_file) != 0) {
        // Not fatal; just warn.
        // fprintf(stderr, "Warning: failed to remove %s\n", c_file);
    }

    ast_free(ast);
    printf("Built executable: %s\n", bin_file);
    return 0;
}

