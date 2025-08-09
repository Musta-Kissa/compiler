#include<stdio.h>
#include <stdlib.h>
#include "my_string.h"

String String_readfile(FILE* file) {
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buff = (char*)malloc(filesize+1);
    fread(buff,1,filesize,file);
    fseek(file, 0, SEEK_SET);
    buff[filesize+1] = '\0';
    String string;
    string.data = buff;
    string.len = filesize;
    string.idx = 0;
    return string;
}

char String_getc(String* string) {
    if( string->idx < string->len ) {
        return string->data[string->idx++];
    } else {
        return EOF;
    }
}

void String_ungetc(String* string) {
    string->idx--;
}
