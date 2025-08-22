#include "parser.h"
#include "lexer.h"
#include "analyzer.h"
#include "dyn_arr.h"

#define ASSERT(expr, fmt, ...) { \
    if (!expr) { \
        printf(fmt "\n", ##__VA_ARGS__); \
        exit(-1); \
    } \
}

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}


Analyzer anlz;

const char* format_ast_type(AstExpr* stm) {
    switch( stm->type ) {
        case AST_FUNCTION_DECLARATION:  return "AST_FUNCTION_DECLARATION";
        case AST_BLOCK_STATEMENT:       return "AST_BLOCK_STATEMENT";
        case AST_DECLARATION:           return "AST_DECLARATION";
        case AST_IF_STATEMENT:          return "AST_IF_STATEMENT";
        case AST_FOR_STATEMENT:         return "AST_FOR_STATEMENT";
        case AST_WHILE_STATEMENT:       return "AST_WHILE_STATEMENT";
        case AST_RETURN_STATEMENT:      return "AST_RETURN_STATEMENT";
        case AST_EXPRESSION_STATEMENT:  return "AST_EXPRESSION_STATEMENT";
        case AST_BINARY_OPERATION:      return "AST_BINARY_OPERATION";
        default:
            PANIC("UNKNOWKN AST NODE TYPE");
        }
}

Variable Variable_new(char* type, char* ident) {
    Variable var;
        var.ident = ident;
        var.type  = type;
    return var;
}

Stack Stack_new() {
    Stack stk;
        stk.pointer = 0;
        stk.frames[0] = 0;
        stk.frames_idx = 1;
    return stk;
}

Analyzer Analyzer_new() {
    Analyzer analyzer;
        analyzer.declared_vars = Stack_new();
        analyzer.types_idx = 0;
    return analyzer;
}

void Stack_new_frame(Stack* stk) {
    ASSERT( (stk->frames_idx < FRAMES_NUM ) , "TO MANY FRAMES: frame ptr: %d",stk->frames_idx);
    stk->frames[stk->frames_idx++] = stk->pointer;
}
void Stack_pop_frame(Stack* stk) {
    if( stk->pointer == 0 ) {
        PANIC("NO FRAME TO POP");
    } 
    stk->pointer = stk->frames[--stk->frames_idx];
}
void Stack_append(Stack* stk, Variable var) {
    ASSERT( (stk->pointer < VARS_NUM ) , "TO MANY VARS: stk ptr: %d", stk->pointer);
    stk->vars[stk->pointer++] = var;
}
int Stack_find(Stack* stk, char* ident) {
    for( int i = stk->pointer - 1; i >= 0; i-- ) {
        if( strcmp(ident,stk->vars[i].ident) == 0 ) {
            return 1;
        }
    }
    return 0;
}
int Stack_curr_frame(Stack* stk) {
    return stk->frames[stk->frames_idx -1];
}
int Stack_find_curr_frame(Stack* stk, char* ident) {
    for( int i = stk->pointer - 1; i >= Stack_curr_frame(stk); i-- ) {
        if( strcmp(ident,stk->vars[i].ident) == 0 ) {
            return 1;
        }
    }
    return 0;
}

// ===================================================================

void analyze_func_decl(AstExpr* stm) {

    char* ident = stm->function_declaration.name.value;
    char* return_type = stm->function_declaration.return_type.value;

    if( anlz.declared_vars.frames_idx > 1 ) {
        PANIC("Function Declaration not in global scope: %s",ident);
    }

    Variable var = Variable_new(return_type,ident);

    if( Stack_find(&anlz.declared_vars,var.ident) ) {
        PANIC("Redefinition of ident \"%s\" as a function",ident);
    }
    Stack_append(&anlz.declared_vars,var);
    Stack_new_frame(&anlz.declared_vars);

    AstExpr* arg = stm->function_declaration.args;
    while( arg != NULL ) { // Adding function args to the function scope
        char* type  = arg->argument_decl.type.value;
        char* ident = arg->argument_decl.ident.value;
        Variable var = Variable_new(type,ident);
        Stack_append(&anlz.declared_vars,var);
        arg = arg->argument_decl.next;
    }
    // analyze fn body
    analyze_statements(stm->function_declaration.body->block_statement.statements); 
    Stack_pop_frame(&anlz.declared_vars);
}
void analyze_block(AstExpr* stm) {
    Stack_new_frame(&anlz.declared_vars);
    analyze_statements(stm->block_statement.statements); 
    Stack_pop_frame(&anlz.declared_vars);
}

void analyze_decl(AstExpr* stm) {
    char* ident = stm->declaration.name.value;
    char* type = stm->declaration.type.value;

    Variable var = Variable_new(type,ident);

    if( Stack_find_curr_frame(&anlz.declared_vars,var.ident) ) {
        PANIC("Redefinition of a var: %s",ident);
    }
    Stack_append(&anlz.declared_vars,var);
}
void analyze_if(AstExpr* stm) {
    if( anlz.declared_vars.frames_idx <= 1 ) {
        PANIC("'if' statement in global scope");
    }
    analyze_statements(stm->if_statement.condition);
    analyze_statements(stm->if_statement.body);
}
void analyze_expr(AstExpr* stm) {
    switch(stm->type) {
        char* ident;
        case AST_NUMBER:
            return;
        case AST_IDENTIFIER:
            ident = stm->identifier.token.value;  
            if( !Stack_find(&anlz.declared_vars, ident) ) {
                PANIC("Use of undeclered var: %s",ident);
            } return;
        case AST_STRING:
            return;
    }
    if( stm->type == AST_UNARY_OPERATION ) {
        analyze_expr(stm->unary_operation.right);
    } else
    if( stm->type == AST_BINARY_OPERATION ) {
        analyze_expr(stm->binary_operation.right);
        analyze_expr(stm->binary_operation.left);
    } else {
        PANIC("%s %d: Expected opp or terminal",__FILE__,__LINE__);
    }
}
void analyze_expr_statement(AstExpr* stm) {
    AstExpr* expr = stm->expression_statement.value;
    analyze_expr(expr);
}
void analyze_for(AstExpr* stm) {
    if( anlz.declared_vars.frames_idx <= 1 ) {
        PANIC("'for' statement in global scope");
    }
    Stack_new_frame(&anlz.declared_vars);

    analyze_statements(stm->for_statement.initial);
    analyze_statements(stm->for_statement.condition);
    analyze_statements(stm->for_statement.iteration);

    analyze_statements(stm->for_statement.body->block_statement.statements); 
    Stack_pop_frame(&anlz.declared_vars);
}

void analyze_while(AstExpr* stm) {
    if( anlz.declared_vars.frames_idx <= 1 ) {
        PANIC("'while' statement in global scope");
    }
    analyze_statements(stm->while_statement.condition);
    analyze_statements(stm->while_statement.body);
}
void analyze_return(AstExpr* stm) {
    if( anlz.declared_vars.frames_idx <= 1 ) {
        PANIC("'return' statement in global scope");
    }
    analyze_statements(stm->return_statement.expression);
}

void analyze_statements(AstExpr* stm) {
    AstExpr* next = stm;
    while( next != NULL ) {
        switch( next->type ) {
            case AST_FUNCTION_DECLARATION:
                analyze_func_decl(next); 
                next = next->function_declaration.next;
                break;
            case AST_BLOCK_STATEMENT:
                analyze_block(next); 
                next = next->block_statement.next;
                break;
            case AST_DECLARATION:
                analyze_decl(next); 
                next = next->declaration.next;
                break;
            case AST_IF_STATEMENT:
                analyze_if(next); 
                next = next->if_statement.next;
                break;
            case AST_EXPRESSION_STATEMENT:
                analyze_expr_statement(next); 
                next = next->expression_statement.next;
                break;
            case AST_FOR_STATEMENT:
                analyze_for(next); 
                next = next->for_statement.next;
                break;
            case AST_WHILE_STATEMENT:
                analyze_while(next); 
                next = next->while_statement.next;
                break;
            case AST_RETURN_STATEMENT:
                analyze_return(next); 
                next = next->return_statement.next;
                break;
            default:
                PANIC("NOT SUPPORTED: %s",format_ast_type(next));
        }
    }
}

void analyze_program_ast(AstExpr* ast) {
    anlz = Analyzer_new();
    analyze_statements(ast);
    printf("analyzed ✓\n"); 
}
