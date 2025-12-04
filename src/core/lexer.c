#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

int lex_file(const char* filename, TokenList* out) {
    FILE* f = fopen(filename, "r");
    if (!f) { perror("Cannot open file"); return 1; }
    out->count = 0;
    char line[256];
    while(fgets(line,sizeof(line),f)) {
        char* p = line;
        while(*p) {
            while(isspace(*p)) p++;
            if(*p=='\0') break;
            Token tok; tok.type=TOKEN_UNKNOWN; tok.text[0]='\0';
            if(strncmp(p,"import",6)==0 && isspace(p[6])) { tok.type=TOKEN_IMPORT; strcpy(tok.text,"import"); p+=6; }
            else if(strncmp(p,"main",4)==0) { tok.type=TOKEN_MAIN; strcpy(tok.text,"main"); p+=4; }
            else if(strncmp(p,"int",3)==0) { tok.type=TOKEN_INT; strcpy(tok.text,"int"); p+=3; }
            else if(strncmp(p,"string",6)==0) { tok.type=TOKEN_STRING; strcpy(tok.text,"string"); p+=6; }
            else if(strncmp(p,"std.printf",10)==0) { tok.type=TOKEN_PRINTF; strcpy(tok.text,"printf"); p+=10; }
            else if(strncmp(p,"if",2)==0) { tok.type=TOKEN_IF; strcpy(tok.text,"if"); p+=2; }
            else if(strncmp(p,"else",4)==0) { tok.type=TOKEN_ELSE; strcpy(tok.text,"else"); p+=4; }
            else if(strncmp(p,"return",6)==0) { tok.type=TOKEN_RETURN; strcpy(tok.text,"return"); p+=6; }
            else if(*p=='('){ tok.type=TOKEN_LPAREN; tok.text[0]='('; tok.text[1]='\0'; p++; }
            else if(*p==')'){ tok.type=TOKEN_RPAREN; tok.text[0]=')'; tok.text[1]='\0'; p++; }
            else if(*p=='{'){ tok.type=TOKEN_LBRACE; tok.text[0]='{'; tok.text[1]='\0'; p++; }
            else if(*p=='}'){ tok.type=TOKEN_RBRACE; tok.text[0]='}'; tok.text[1]='\0'; p++; }
            else if(*p=='.'){ tok.type=TOKEN_DOT; tok.text[0]='.'; tok.text[1]='\0'; p++; }
            else if(isdigit(*p)){ int i=0; while(isdigit(*p)) tok.text[i++]=*p++; tok.text[i]='\0'; tok.type=TOKEN_NUMBER; }
            else if(*p=='"'){ int i=0; tok.text[i++]=*p++; while(*p && *p!='"') tok.text[i++]=*p++; if(*p=='"') tok.text[i++]=*p++; tok.text[i]='\0'; tok.type=TOKEN_STRING_LITERAL; }
            else if(isalpha(*p)){ int i=0; while(isalnum(*p)||*p=='_') tok.text[i++]=*p++; tok.text[i]='\0'; tok.type=TOKEN_IDENTIFIER; }
            else { p++; continue; }
            out->tokens[out->count++]=tok;
        }
    }
    fclose(f);
    Token eof={TOKEN_EOF,""};
    out->tokens[out->count++]=eof;
    return 0;
}
