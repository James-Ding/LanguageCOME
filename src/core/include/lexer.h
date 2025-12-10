#ifndef LEXER_H
#define LEXER_H
typedef enum { TOKEN_EOF, TOKEN_IMPORT, TOKEN_MAIN, TOKEN_INT, TOKEN_STRING, TOKEN_IDENTIFIER,
               TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_DOT,
               TOKEN_PRINTF, TOKEN_IF, TOKEN_ELSE, TOKEN_RETURN, TOKEN_NUMBER, TOKEN_STRING_LITERAL,
               TOKEN_UNKNOWN } TokenType;

typedef struct { TokenType type; char text[128]; } Token;
typedef struct { Token tokens[1024]; int count; } TokenList;
int lex_file(const char* filename, TokenList* out);
#endif
