#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_string.h"

typedef enum TokenKind {
    IDENT,
    NUMBER,
    STRING,

    INT,
    FLOAT,

    OPEN_PARENT,
    CLOSE_PARENT,
    OPEN_CURRLY_PARENT,
    CLOSE_CURRLY_PARENT,
    DOT,
    EOF_TOKEN,

    ASSIGN,
    SEMICOLON,
    COMMA,
    ADDITION,
    MULTIPLICATION,
    DIVITION,
    LESS_THEN,
    MORE_THEN,
    SUBSCRIPT_OPEN,
    SUBSCRIPT_CLOSE,

    SUBTRACT,
    EQUAL,
    NOT,
    NOT_EQUAL,
    LESS_EQUAL,
    MORE_EQUAL,

} TokenKind ;

typedef struct Token {
    TokenKind kind;
    union {
        int number;
        char* string;
    } value;
} Token;

typedef struct Lexer {
    int idx;
    Token* tokens;
} Lexer;


Token Lexer_next(Lexer* lexer);
Token Lexer_peek(Lexer* lexer);
Token Lexer_peek_back(Lexer* lexer);

int is_terminal(char c);
Lexer lex_file(String string);
const char* format_enum(TokenKind k);

#endif
