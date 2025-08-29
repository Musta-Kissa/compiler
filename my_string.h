#ifndef MY_STRING_H
#define MY_STRING_H

#include <stdio.h>

typedef struct String {
    char* data;
    int len;
    int idx;
} String;

String String_readfile(FILE* file);
char String_getc(String* string);
void String_ungetc(String* string);

//String Builder

typedef struct {
    char* buffer;
    size_t length;
    size_t capacity;
} StringBuilder;

StringBuilder sb_new();
void sb_reset(StringBuilder* sb);
void sb_free(StringBuilder* sb);
void sb_append(StringBuilder* sb, const char* fmt, ...);

typedef struct AstExpr AstExpr;

void print_expr_to_sb(StringBuilder* sb,AstExpr* expr);
void print_args_to_sb(StringBuilder* sb, AstExpr* arg);

#endif
