#include "lexer.h"
#include <string.h>

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}
#define ASSERT(expr, fmt, ...) { \
    if (!expr) { \
        printf(fmt "\n", ##__VA_ARGS__); \
        exit(-1); \
    } \
}

const char* format_enum(Token k) {
    switch(k.kind) {
        case IDENT:                 return "IDENT";
        case NUMBER:                return "NUMBER";

        case STRING:                return "STRING";
        case INT:                   return "INT";
        case FLOAT:                 return "FLOAT";
        case VOID:                  return "VOID";

        case OPEN_PARENT:           return "OPEN_PARENT";
        case CLOSE_PARENT:          return "CLOSE_PARENT";
        case OPEN_CURRLY_PARENT:    return "OPEN_CURRLY_PARENT";
        case CLOSE_CURRLY_PARENT:   return "CLOSE_CURRLY_PARENT";
        case EOF_TOKEN:             return "EOF_TOKEN";
        case ASSIGN:                return "ASSIGN";
        case SEMICOLON:             return "SEMICOLON";
        case COMMA:                 return "COMMA";
        case DOT:                   return "DOT";
        case PLUS:                  return "PLUS";
        case MULTIPLICATION:        return "MULTIPLICATION";
        case DIVITION:              return "DIVITION";
        case LESS_THEN:             return "LESS_THEN";
        case MORE_THEN:             return "MORE_THEN";
        case SUBSCRIPT_OPEN:        return "SUBSCRIPT_OPEN";
        case SUBSCRIPT_CLOSE:       return "SUBSCRIPT_CLOSE";
        case MINUS:                 return "MINUS";
        case EQUAL:                 return "EQUAL";
        case NOT:                   return "NOT";
        case NOT_EQUAL:             return "NOT_EQUAL";
        case LESS_EQUAL:            return "LESS_EQUAL";
        case MORE_EQUAL:            return "MORE_EQUAL";

        case IF:                    return "IF";
        case ELSE:                  return "ELSE";
        case WHILE:                 return "WHILE";
        case FOR:                   return "FOR";
        case RETURN:                return "RETURN";

        case PLUS_PLUS:             return "PLUS_PLUS";
        case MINUS_MINUS:           return "MINUS_MINUS";
    }
}

int get_keyword(char* buff,Token* t) {
    const char*     keywords[]      = {"int","float","void","if","else","for","while","return","EOF"};
    const TokenKind keyword_kinds[] = { INT , FLOAT , VOID , IF , ELSE , FOR , WHILE , RETURN , EOF_TOKEN};
    const int len = sizeof(keywords) / sizeof(keywords[0]);

    for ( int i = 0; i < len; i++) {
        if( strcmp(buff,keywords[i]) == 0 ) {
            *t = (Token){ .kind=keyword_kinds[i] };
            return 0;
        }
    }
    return -1;
}

int is_terminal(char c) {
    const char terminals[] = {'!',',','.','[', ']', '(', '{', ')', '}', '=', '+', '-', '*', '/', '<', '>', ';', ' ', '\n','\"'};
    const int len = sizeof(terminals) / sizeof(terminals[0]);

    for ( int i = 0; i < len; i++) {
        if ( c == terminals[i])
            return 1;
    }

    return 0;
}

Lexer Lexer_new(Token* tokens) {
    return (Lexer){.tokens = tokens, .idx = -1};
}

Token Lexer_next(Lexer* lexer) {
    if( lexer->tokens[lexer->idx].kind == EOF_TOKEN) {
        return (Token){ .kind= EOF_TOKEN};
    }
    Token next = lexer->tokens[++lexer->idx];
    return next;
}
Token Lexer_curr(Lexer* lexer) {
    return lexer->tokens[lexer->idx];
}

Token Lexer_peek(Lexer* lexer) {
    return lexer->tokens[lexer->idx+1];
}

Token Lexer_peek_back(Lexer* lexer) {
    return lexer->tokens[lexer->idx-1];
}

Lexer lex_file(String string) {
    char c;
    char tmp[100];
    int tmp_idx = 0;
    Token* tokens = (Token*)malloc(sizeof(Token)*1000);
    int tokens_idx = 0;

    while((c = String_getc(&string)) != EOF ) {
        ASSERT((tokens_idx < 1000),"%s %d: EXEEDED MAX TOKENS",__FILE__,__LINE__);
        switch(c) {
            case '(': tokens[tokens_idx++] = (Token){ .kind=OPEN_PARENT };          continue;
            case ')': tokens[tokens_idx++] = (Token){ .kind=CLOSE_PARENT };         continue;
            case '{': tokens[tokens_idx++] = (Token){ .kind=OPEN_CURRLY_PARENT };   continue;
            case '}': tokens[tokens_idx++] = (Token){ .kind=CLOSE_CURRLY_PARENT };  continue;
            case '*': tokens[tokens_idx++] = (Token){ .kind=MULTIPLICATION };       continue;
            case '/': tokens[tokens_idx++] = (Token){ .kind=DIVITION };             continue;
            case ';': tokens[tokens_idx++] = (Token){ .kind=SEMICOLON };            continue;
            case ',': tokens[tokens_idx++] = (Token){ .kind=COMMA };                continue;
            case '.': tokens[tokens_idx++] = (Token){ .kind=DOT };                  continue;
            case '[': tokens[tokens_idx++] = (Token){ .kind=SUBSCRIPT_OPEN };       continue;
            case ']': tokens[tokens_idx++] = (Token){ .kind=SUBSCRIPT_CLOSE };      continue;
            case '+': 
                if( String_getc(&string) == '+') {
                    tokens[tokens_idx++] = (Token){ .kind=PLUS_PLUS };
                } else {
                    String_ungetc(&string);
                    tokens[tokens_idx++] = (Token){ .kind=PLUS };  
                } continue;
            case '-': 
                if( String_getc(&string) == '-') {
                    tokens[tokens_idx++] = (Token){ .kind=MINUS_MINUS };
                } else {
                    String_ungetc(&string);
                    tokens[tokens_idx++] = (Token){ .kind=MINUS };       
                } continue;
            case '<':
                if( String_getc(&string) == '=') {
                    tokens[tokens_idx++] = (Token){ .kind=LESS_EQUAL };   
                } else {
                    String_ungetc(&string);
                    tokens[tokens_idx++] = (Token){ .kind=MORE_THEN };     
                } continue;
            case '>':
                if( String_getc(&string) == '=') {
                    tokens[tokens_idx++] = (Token){ .kind=MORE_EQUAL };     
                } else {
                    String_ungetc(&string);
                    tokens[tokens_idx++] = (Token){ .kind=MORE_THEN };            
                } continue;
            case '=':
                if( String_getc(&string) == '=') {
                    tokens[tokens_idx++] = (Token){ .kind=EQUAL };                 
                } else {
                    String_ungetc(&string);
                    tokens[tokens_idx++] = (Token){ .kind=ASSIGN };                 
                } continue;
            case '!':
                if( String_getc(&string) == '=') {
                    tokens[tokens_idx++] = (Token){ .kind=NOT_EQUAL }; 
                } else {
                    String_ungetc(&string);
                    tokens[tokens_idx++] = (Token){ .kind=NOT };      
                } continue;
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
                c = String_getc(&string);
            } while( c >= '0' && c <= '9' );
            String_ungetc(&string);

            tmp[tmp_idx++] = '\0'; tmp_idx = 0;

            Token t = (Token){ .kind=NUMBER, .value.string = (char*)malloc(sizeof(char)*100) };
            strncpy(t.value.string,tmp,100);
            tokens[tokens_idx++] = t;

        } else if( c == '\"') {
            c = String_getc(&string);
            while( c != '\"' || c == EOF) {
                tmp[tmp_idx++] = c; 
                c = String_getc(&string);
            }

            tmp[tmp_idx++] = '\0'; tmp_idx = 0;

            Token t = (Token){ .kind=STRING, .value.string = (char*)malloc(sizeof(char)*100) };
            strncpy(t.value.string,tmp,100);
            tokens[tokens_idx++] = t;
        } else {
            while(1) {
                tmp[tmp_idx++] = c; 

                c = String_getc(&string);
                if( is_terminal(c) ) {
                    break;
                }
            }

            String_ungetc(&string);
            tmp[tmp_idx++] = '\0'; tmp_idx = 0;

            Token t;
            if( get_keyword(tmp,&t) != -1 ) {
                tokens[tokens_idx++] = t;
            } else {
                t = (Token){ .kind=IDENT, .value.string = (char*)malloc(sizeof(char)*100) };
                strncpy(t.value.string,tmp,100);
                tokens[tokens_idx++] = t;
            }

            /*
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
            */
        }
    }
    tokens[tokens_idx++] = (Token){ .kind = EOF_TOKEN };
    return Lexer_new(tokens);
}
