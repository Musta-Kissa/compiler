#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "my_string.h"
#include "parser.h"

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}

void print_expr(AstExpr* expr);
void print_args(AstExpr* arg) {
    if( arg == NULL) {
        printf("\b\b");
        return;
    } 
    print_expr(arg->argument.value);
    printf(", ");
    print_args(arg->argument.next);
}

void print_expr(AstExpr* expr) {
    if( expr == NULL) {
        printf("EMPTY EXPR");
        return;
    }
    switch(expr->type) {
        case AST_NUMBER:
            printf("%s",expr->number.token.value.string);
            return;
        case AST_IDENTIFIER:
            printf("%s",expr->identifier.token.value.string);
            return;
        case AST_STRING:
            printf("\"%s\"",expr->identifier.token.value.string);
            return;
    }
    if (expr->type == AST_BINARY_OPERATION) {
        printf("(");
        switch (expr->binary_operation.opp_token.kind) {
            case ADDITION:
                printf("+"); break;
            case DIVITION:
                printf("/"); break;
            case MULTIPLICATION:
                printf("*"); break;
            case LESS_THEN:
                printf("<"); break;
            case MORE_THEN:
                printf(">"); break;
            case SUBSCRIPT_OPEN:
                printf("[]"); break;
            case DOT:
                printf("."); break;
            case SUBTRACT:
                printf("-"); break;
            case EQUAL:
                printf("=="); break;
            case NOT_EQUAL:
                printf("!="); break;
            case MORE_EQUAL:
                printf(">="); break;
            case LESS_EQUAL:
                printf("<="); break;
            case NOT:
                printf("!"); break;
        }
        printf(" "); 
        print_expr(expr->binary_operation.left);
        printf(" ");
        print_expr(expr->binary_operation.right);
        printf(")");
    } 
    if( expr->type == AST_FUNC_CALL ) {
        printf("<Fn %s>{",expr->func_call.identifier.value.string);
        if( expr->func_call.args != NULL ){
            print_args(expr->func_call.args);
        }
        printf("}");
    }
}


int main(int argc, char* argv[]) {
    //for(int n = 0; n < argc; n++) {
        //printf("arg%d %s\n",n,argv[n]);
    //}
    FILE* f = fopen("./input2.txt","r");

    String source = String_readfile(f);
    printf("source: \n%s",source.data);

    Lexer lexer = lex_file(source);

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

    printf("AstExpr size: %d\n",sizeof(Token));

    while( Lexer_peek(&lexer).kind != EOF_TOKEN) {
        AstExpr* expr = parse_expr(&lexer,0);
        printf("=========================\n");
            print_expr(expr);
        printf("\n");
        Lexer_next(&lexer);
    }
}
