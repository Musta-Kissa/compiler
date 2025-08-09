#ifndef MY_STRING_H
#define MY_STRING_H

typedef struct String {
    char* data;
    int len;
    int idx;
} String;

String String_readfile(FILE* file);
char String_getc(String* string);
void String_ungetc(String* string);

#endif
