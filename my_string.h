#ifndef MY_STRING_H
#define MY_STRING_H

#include <stdio.h>

typedef struct String {
    char* data;
    int len;
    int idx;
} String;

ssize_t strscpy(char *dest, const char *src, size_t count);

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

typedef struct AstNode AstNode;

void print_expr_to_sb(StringBuilder* sb,AstNode* expr);
void print_args_to_sb(StringBuilder* sb, AstNode* arg);

#endif
