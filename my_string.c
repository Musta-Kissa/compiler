#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "panic_macros.h"
#include "my_string.h"
#include "parser.h"

ssize_t strscpy(char *dest, const char *src, size_t count) {
    if( count == 0 )
        return -E2BIG;

    size_t i = 0;
    for(; i < count-1; i++) {
        char c = src[i];
        dest[i] = c;
        if( c == '\0' )
            return i;
    }
	dest[i] = '\0';

    if( src[i] != '\0')
        return -E2BIG;

	return i;
}

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

#include <string.h>
#include <stdarg.h>

StringBuilder sb_new() {
    StringBuilder sb;
        sb.capacity = 128;
        sb.length = 0;
        sb.buffer = malloc(sb.capacity);
        if (sb.buffer) {
            sb.buffer[0] = '\0';
        } else {
            PANIC("MALLOC ERROR");
        }
    return sb;
}

void sb_reset(StringBuilder* sb) {
    sb->length = 0;
    sb->buffer[0] = '\0';
}

void sb_free(StringBuilder* sb) {
    free(sb->buffer);
}

void sb_append(StringBuilder* sb, const char* fmt, ...) {
    va_list args;

    // Try to print to a temporary buffer to get required size
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (sb->length + needed + 1 > sb->capacity) {
        // Resize buffer
        while (sb->length + needed + 1 > sb->capacity) {
            sb->capacity *= 2;
        }
        sb->buffer = realloc(sb->buffer, sb->capacity);
    }

    // Append the formatted string
    va_start(args, fmt);
    vsprintf(sb->buffer + sb->length, fmt, args);
    va_end(args);

    sb->length += needed;
}

void print_expr_to_sb(StringBuilder* sb,AstNode* expr);

void print_args_to_sb(StringBuilder* sb, AstNode* arg) {
    if( arg == NULL) {
        // TODO change to subtract from the sb instead of printing \b
        sb_append(sb,"\b\b");
        return;
    } 
    print_expr_to_sb(sb,arg->argument.value);
    sb_append(sb,", ");
    print_args_to_sb(sb,arg->argument.next);
}

void print_expr_to_sb(StringBuilder* sb,AstNode* expr) {
    if( expr == NULL) {
        sb_append(sb,"EMPTY EXPR");
        return;
    }
    switch(expr->type) {
        case AST_NUMBER:
            sb_append(sb,"%s",expr->number.value);
            return;
        case AST_IDENTIFIER:
            sb_append(sb,"%s",expr->identifier.token.value);
            return;
        case AST_STRING:
            sb_append(sb,"\"%s\"",expr->identifier.token.value);
            return;
    }
    if( expr->type == AST_UNARY_OPERATION ) {
        sb_append(sb,"(");
        switch (expr->unary_operation.opp_token.kind) {
            case NOT:
                sb_append(sb,"!"); break;
            case MINUS:
                sb_append(sb,"-"); break;
            case PLUS_PLUS:
                sb_append(sb,"++"); break;
            case MINUS_MINUS:
                sb_append(sb,"--"); break;
            case AMPERSAND:
                sb_append(sb,"&"); break;
        }
        sb_append(sb," "); 
        print_expr_to_sb(sb,expr->unary_operation.right);
        sb_append(sb,")");
    }
    if( expr->type == AST_BINARY_OPERATION ) {
        sb_append(sb,"(");
        switch (expr->binary_operation.opp_token.kind) {
            case PLUS:
                sb_append(sb,"+"); break;
            case DIVITION:
                sb_append(sb,"/"); break;
            case STAR:
                sb_append(sb,"*"); break;
            case LESS_THEN:
                sb_append(sb,"<"); break;
            case MORE_THEN:
                sb_append(sb,">"); break;
            case SUBSCRIPT_OPEN:
                sb_append(sb,"[]"); break;
            case DOT:
                sb_append(sb,"."); break;
            case MINUS:
                sb_append(sb,"-"); break;
            case EQUAL:
                sb_append(sb,"=="); break;
            case NOT_EQUAL:
                sb_append(sb,"!="); break;
            case MORE_EQUAL:
                sb_append(sb,">="); break;
            case LESS_EQUAL:
                sb_append(sb,"<="); break;
            case ASSIGN:
                sb_append(sb,"="); break;
            case LOGIC_AND:
                sb_append(sb,"&&"); break;
            case LOGIC_OR:
                sb_append(sb,"||"); break;
            default: 
                PANIC("%s %d:Oparation printing not supported",__FILE__,__LINE__);
        }
        sb_append(sb," "); 
        print_expr_to_sb(sb,expr->binary_operation.left);
        sb_append(sb," ");
        print_expr_to_sb(sb,expr->binary_operation.right);
        sb_append(sb,")");
    } 
    if( expr->type == AST_FUNC_CALL ) {
        sb_append(sb,"<Fn %s>{",expr->function_call.identifier.value);
        if( expr->function_call.args != NULL ){
            print_args_to_sb(sb,expr->function_call.args);
        }
        sb_append(sb,"}");
    }
}
