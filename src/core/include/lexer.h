#ifndef LEXER_H
#define LEXER_H
typedef enum { TOKEN_EOF, TOKEN_IMPORT, TOKEN_MAIN, TOKEN_INT,
               TOKEN_STRING,
               TOKEN_BOOL,
               TOKEN_TRUE,
               TOKEN_FALSE,
               TOKEN_IDENTIFIER,
               TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_DOT,
               TOKEN_PRINTF, TOKEN_IF, TOKEN_ELSE, TOKEN_RETURN, TOKEN_NUMBER, TOKEN_STRING_LITERAL,
               TOKEN_ASSIGN, TOKEN_COMMA, TOKEN_LBRACKET, TOKEN_RBRACKET,
               TOKEN_EQ, TOKEN_NEQ, TOKEN_GT, TOKEN_LT, TOKEN_GE, TOKEN_LE,
               TOKEN_NOT, TOKEN_CHAR_LITERAL,
               TOKEN_UNKNOWN } TokenType;

typedef struct { TokenType type; char text[128]; } Token;
typedef struct { Token tokens[4096]; int count; } TokenList;
int lex_file(const char* filename, TokenList* out);
#endif
