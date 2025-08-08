#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}

void print_ast(AstExpr* expr) {
    switch(expr->type) {
        case AST_NUMBER:
            printf(" %s ",expr->number.token.value.string);
            return;
        case AST_IDENTIFIER:
            printf(" %s ",expr->identifier.token.value.string);
            return;
    }
    if (expr->type == AST_BINARY_OPERATION) {
        printf("(");
        switch (expr->binary_operation.opp_token.kind) {
            case ADDITION:
                printf("+ "); break;
            case DIVITION:
                printf("/ "); break;
            case MULTIPLICATION:
                printf("* "); break;
            case LESS_THEN:
                printf("< "); break;
            case MORE_THEN:
                printf("> "); break;
        }
        print_ast(expr->binary_operation.left);
        print_ast(expr->binary_operation.right);
        printf(")");
    } else {
        exit(69);
    }
}

char* read_file(FILE* file) {
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buff = (char*)malloc(filesize+1);
    fread(buff,1,filesize,file);
    fseek(file, 0, SEEK_SET);
    buff[filesize+1] = '\0';
    return buff;
}

int main(int argc, char* argv[]) {
    //for(int n = 0; n < argc; n++) {
        //printf("arg%d %s\n",n,argv[n]);
    //}
    FILE* f = fopen("./input2.txt","r");

    char* source = read_file(f);
    printf("source: \n%s",source);

    Lexer lexer = lex_file(f);

    for( int n = 0; lexer.tokens[n-1].kind != EOF_TOKEN ; n++) {
        Token t = lexer.tokens[n];
        printf("%s ",format_enum(t.kind));
        switch(t.kind) {
            case IDENT: 
            case NUMBER:
            case STRING:
                printf("val: %s",t.value.string);
        }
        printf("\n");
    }

    AstExpr* expr = parse_expr(&lexer,0);
    printf("=========================\n");
    print_ast(expr);
    printf("\n=========================\n");
    //float result = compute_ast(expr);
    //printf("result %f\n",result);
}
