#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "my_string.h"
#include "parser.h"

/*
    AstExpr* parse_expr(Lexer* lexer, int curr_bp); Done
    AstExpr* parse_arg_decl(Lexer* lexer); Done
    AstExpr* parse_decl(Lexer* lexer);
    AstExpr* parse_func_decl(Lexer* lexer,Token type,Token ident);
    AstExpr* parse_program(Lexer* lexer);
    AstExpr* parse_statements(Lexer* lexer);
    AstExpr* parse_function_call(Lexer* lexer,Token ident);
    AstExpr* parse_unary(Lexer* lexer, Token opp);
*/

void print_statements(AstExpr* stm);
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
    if( expr->type == AST_UNARY_OPERATION ) {
        printf("(");
        switch (expr->unary_operation.opp_token.kind) {
            case NOT:
                printf("!"); break;
            case MINUS:
                printf("-"); break;
        }
        printf(" "); 
        print_expr(expr->unary_operation.right);
        printf(")");
    }
    if( expr->type == AST_BINARY_OPERATION ) {
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
            case MINUS:
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

void print_arg_decl(AstExpr* arg) {
    AstExpr* next = arg;
    while( next != NULL ) {
        printf("\targ: type= {%s} name= {%s}\n",format_enum(next->argument_decl.type),next->argument_decl.ident.value.string);
        next = next->argument_decl.next;
    }
}

void print_func_decl(AstExpr* node) {
    printf("func: name= {%s} return_type= {%s}\n",
            node->function_declaration.name.value.string,
            format_enum(node->function_declaration.return_type));
    print_arg_decl(node->function_declaration.args);
    // print body
    print_statements(node->function_declaration.body);
    printf("\n");
}

void print_decl(AstExpr* node) {
    printf("decl: name= {%s} type= {%s} value= ",
            node->declaration.name.value.string,
            format_enum(node->declaration.type));
    print_expr(node->declaration.value);
    printf("\n");
}

void print_if(AstExpr* node) {
    printf("if: condition= ");
    print_expr(node->if_statement.condition);
    printf(" body= \n");
    print_statements(node->if_statement.body);
    printf("\n");
}

void print_statements(AstExpr* stm) {
    AstExpr* next = stm;
    while( next != NULL ) {
        switch( next->type ) {
            case AST_FUNCTION_DECLARATION:
                print_func_decl(next); 
                next = next->function_declaration.next;
                break;
            case AST_DECLARATION:
                print_decl(next); 
                next = next->declaration.next;
                break;
            case AST_IF_STATEMENT:
                print_if(next); 
                next = next->declaration.next;
                break;
            default:
                PANIC("NOT SUPPORTED");
        }
    }
}

void print_program_ast(AstExpr* ast) {
    print_statements(ast);
    printf("\n");
}
