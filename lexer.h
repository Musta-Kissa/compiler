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

    AMPERSAND,

    OPEN_PARENT,
    CLOSE_PARENT,
    OPEN_CURRLY_PARENT,
    CLOSE_CURRLY_PARENT,
    DOT,
    EOF_TOKEN,

    IF,
    ELSE,
    WHILE,
    FOR,
    RETURN,

    STRUCT,
    ENUM,
    UNION,

    FN,
    ARROW,

    ASSIGN,
    SEMICOLON,
    COLON,
    COMMA,
    PLUS,
    STAR,
    DIVITION,
    LESS_THEN,
    MORE_THEN,
    SUBSCRIPT_OPEN,
    SUBSCRIPT_CLOSE,
    PLUS_PLUS,
    MINUS_MINUS,

    MINUS,
    EQUAL,
    NOT,
    NOT_EQUAL,
    LESS_EQUAL,
    MORE_EQUAL,

} TokenKind ;

typedef struct Token {
    TokenKind kind;
    char* value;
} Token;

typedef struct Lexer {
    int idx;
    Token* tokens;
} Lexer;


Token Lexer_peek_n(Lexer* lexer, int n);
Token Lexer_next(Lexer* lexer);
Token Lexer_peek(Lexer* lexer);
Token Lexer_curr(Lexer* lexer);
Token Lexer_peek_back(Lexer* lexer);

int is_terminal(char c);
Lexer lex_file(String string);
const char* format_enum(Token k);

#endif
