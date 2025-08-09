#include "lexer.h"

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}

const char* format_enum(TokenKind k) {
    switch(k) {
        case IDENT:                 return "IDENT";
        case NUMBER:                return "NUMBER";
        case STRING:                return "STRING";
        case INT:                   return "INT";
        case FLOAT:                 return "FLOAT";
        case OPEN_PARENT:           return "OPEN_PARENT";
        case CLOSE_PARENT:          return "CLOSE_PARENT";
        case OPEN_CURRLY_PARENT:    return "OPEN_CURRLY_PARENT";
        case CLOSE_CURRLY_PARENT:   return "CLOSE_CURRLY_PARENT";
        case DOT:                   return "DOT";
        case EOF_TOKEN:             return "EOF_TOKEN";
        case ASSIGN:                return "ASSIGN";
        case SEMICOLON:             return "SEMICOLON";
        case ADDITION:              return "ADDITION";
        case MULTIPLICATION:        return "MULTIPLICATION";
        case DIVITION:              return "DIVITION";
        case LESS_THEN:             return "LESS_THEN";
        case MORE_THEN:             return "MORE_THEN";
        case SUBSCRIPT_OPEN:        return "SUBSCRIPT_OPEN";
        case SUBSCRIPT_CLOSE:       return "SUBSCRIPT_CLOSE";
    }
}


int is_terminal(char c) {
    const char terminals[] = {'.','[', ']', '(', '{', ')', '}', '=', '+', '*', '/', '<', '>', ';', ' ', '\n','\"'};
    const int len = sizeof(terminals) / sizeof(terminals[0]);

    for ( int i = 0; i <= len; i++) {
        if ( c == terminals[i])
            return 1;
    }

    return 0;
}

Lexer Lexer_new(Token* tokens) {
    return (Lexer){.tokens = tokens, .idx = -1};
}

Token Lexer_next(Lexer* lexer) {
    Token next = lexer->tokens[++lexer->idx];
    if( next.kind == EOF_TOKEN) {
        lexer->idx--;
    }
    return next;
}

Token Lexer_peek(Lexer* lexer) {
    return lexer->tokens[lexer->idx+1];
}

Token Lexer_peek_back(Lexer* lexer) {
    return lexer->tokens[lexer->idx-1];
}

Lexer lex_file(FILE* f) {
    char c;
    char tmp[100];
    int tmp_idx = 0;
    Token* tokens = (Token*)malloc(sizeof(Token)*100);
    int tokens_idx = 0;

    while((c = fgetc(f)) != EOF ) {
        switch(c) {
            case '(': tokens[tokens_idx++] = (Token){ .kind=OPEN_PARENT };          continue;
            case '{': tokens[tokens_idx++] = (Token){ .kind=OPEN_CURRLY_PARENT };   continue;
            case ')': tokens[tokens_idx++] = (Token){ .kind=CLOSE_PARENT };         continue;
            case '}': tokens[tokens_idx++] = (Token){ .kind=CLOSE_CURRLY_PARENT };  continue;
            case '=': tokens[tokens_idx++] = (Token){ .kind=ASSIGN };               continue;
            case '+': tokens[tokens_idx++] = (Token){ .kind=ADDITION };             continue;
            case '*': tokens[tokens_idx++] = (Token){ .kind=MULTIPLICATION };       continue;
            case '/': tokens[tokens_idx++] = (Token){ .kind=DIVITION };             continue;
            case ';': tokens[tokens_idx++] = (Token){ .kind=SEMICOLON };            continue;
            case '.': tokens[tokens_idx++] = (Token){ .kind=DOT };                  continue;
            case '<': tokens[tokens_idx++] = (Token){ .kind=LESS_THEN };            continue;
            case '>': tokens[tokens_idx++] = (Token){ .kind=MORE_THEN };            continue;
            case '[': tokens[tokens_idx++] = (Token){ .kind=SUBSCRIPT_OPEN };       continue;
            case ']': tokens[tokens_idx++] = (Token){ .kind=SUBSCRIPT_CLOSE };      continue;
        }
        switch(c) {
            case '\t':
            case '\n':
            case ' ':
                continue;
        }

        if( c >= '0' && c <= '9' ) {
            do {
                tmp[tmp_idx++] = c; 
                c = fgetc(f);
            } while( c >= '0' && c <= '9' );
            ungetc(c,f);

            tmp[tmp_idx++] = '\0'; tmp_idx = 0;

            Token t = (Token){ .kind=NUMBER, .value.string = (char*)malloc(sizeof(char)*100) };
            strncpy(t.value.string,tmp,100);
            tokens[tokens_idx++] = t;

        } else if( c == '\"') {
            c = fgetc(f);
            while( c != '\"' || c == EOF) {
                tmp[tmp_idx++] = c; 
                c = fgetc(f);
            }

            tmp[tmp_idx++] = '\0'; tmp_idx = 0;

            Token t = (Token){ .kind=STRING, .value.string = (char*)malloc(sizeof(char)*100) };
            strncpy(t.value.string,tmp,100);
            tokens[tokens_idx++] = t;
        } else {
            while(1) {
                tmp[tmp_idx++] = c; 

                c = fgetc(f);
                if( is_terminal(c) ) {
                    break;
                }
            }

            ungetc(c,f);
            tmp[tmp_idx++] = '\0'; tmp_idx = 0;

            if( strcmp(tmp,"int") == 0 ) {
                tokens[tokens_idx++] = (Token){ .kind=INT };
            } 
            else if( strcmp(tmp,"float") == 0 ) {
                tokens[tokens_idx++] = (Token){ .kind=FLOAT };
            } 
            else {
                Token t = (Token){ .kind=IDENT, .value.string = (char*)malloc(sizeof(char)*100) };
                strncpy(t.value.string,tmp,100);
                tokens[tokens_idx++] = t;
            }
        }
    }
    tokens[tokens_idx++] = (Token){ .kind = EOF_TOKEN };
    return Lexer_new(tokens);
}
