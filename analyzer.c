#include "parser.h"
#include "lexer.h"
#include "analyzer.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

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


Type Type_new(char* type_name, TypeKind type_kind) {
    return (Type){ .type_name=type_name,.type_kind=type_kind };
}

Analyzer anlz;
Type Analyzer_get_type(char* type_name) {
    for( int i = 0 ; i < anlz.types_idx ; i++ ) {
        char* curr = anlz.types[i].type_name;
        if( strcmp(type_name,curr) == 0 ) {
            return anlz.types[i];
        }
    }
    PANIC("%s %d: Type not found in Analyzer_get_type(): %s",__FILE__,__LINE__,type_name);
}

int type_is(const char* type, ...) {
    va_list args;
    va_start(args,type);

    const char* curr = va_arg(args, const char*);
    for( ; curr != NULL ; curr = va_arg(args,const char*) ) {
        if( strcmp(type,curr) == 0 ) {
            va_end(args);
            return 1;
        }
    }
    va_end(args);
    return 0;
}
#define type_is(...) type_is(__VA_ARGS__,0)

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

Variable Variable_new(Type type, char* ident) {
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

const Type primitive_types[] = {
    {PRIMITIVE_TYPE,"bool"},
    {PRIMITIVE_TYPE,"float"},
    {PRIMITIVE_TYPE,"int"},
    {PRIMITIVE_TYPE,"void"},
};
void Analyzer_init() {
    Analyzer analyzer;
        analyzer.declared_vars = Stack_new();
        analyzer.types_idx = sizeof(primitive_types) / sizeof(primitive_types[0]);
    for (size_t i = 0; i < analyzer.types_idx; i++) {
        analyzer.types[i] = primitive_types[i];
    }
    anlz = analyzer;
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
Variable Stack_get(Stack* stk, char* ident) {
    for( int i = stk->pointer - 1; i >= 0; i-- ) {
        if( strcmp(ident,stk->vars[i].ident) == 0 ) {
            return stk->vars[i]; 
        }
    }
    PANIC("%s %d: Not found in stack",__FILE__,__LINE__);
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

    char* ident = stm->function_declaration.name;
    char* return_type_name = stm->function_declaration.return_type_name;

    if( anlz.declared_vars.frames_idx > 1 ) {
        PANIC("Function Declaration not in global scope: %s",ident);
    }

    Variable var = Variable_new(Type_new(return_type_name,FUNCTION_TYPE),ident);

    if( Stack_find(&anlz.declared_vars,var.ident) ) {
        PANIC("Redefinition of ident \"%s\" as a function",ident);
    }
    Stack_append(&anlz.declared_vars,var);
    Stack_new_frame(&anlz.declared_vars);

    AstExpr* arg = stm->function_declaration.args;
    while( arg != NULL ) { // Adding function args to the function scope
        char* type_name  = arg->argument_decl.type_name;
        char* ident = arg->argument_decl.ident.value;
        Type  type = Analyzer_get_type(type_name);
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

//  banana : int = 5; 
//  banana : int;
//  banana := 5;     
void analyze_decl(AstExpr* stm) {
    char* ident      = stm->declaration.name;
    char* type_name  = stm->declaration.type_name;

    if( stm->declaration.value->expression_statement.value == NULL ) {
        if( type_name == NULL ) {
            PANIC("Declaration of a variable '%s' without specified type",ident);
        }
    } else {
        Type expr_type = analyze_expr_statement(stm->declaration.value);
        if( type_name == NULL ) {
            type_name = expr_type.type_name;
        } else {
            if( !type_is(type_name, expr_type.type_name) ) {
                PANIC("Type specified in the declaration of var '%s' {%s} doesnt match the type of the expr provided: {%s}",ident,type_name,expr_type);
            }
        }
    }

    Type type = Analyzer_get_type(type_name);
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
Type analyze_func_call(AstExpr* stm) {
    Variable var;
    char* ident = stm->func_call.identifier.value;  
    if( !Stack_find(&anlz.declared_vars, ident) ) {
        PANIC("Use of undeclered function: %s",ident);
    } else {
        var = Stack_get(&anlz.declared_vars, ident);
        if( var.type.type_kind != FUNCTION_TYPE ) {
            PANIC("Tried to call variable '%s' of type {%s} as a function",var.ident,var.type.type_name);
        }
    }
    analyze_func_call_args(stm->func_call.args);
    return var.type;
}
void analyze_func_call_args(AstExpr* stm) {
    if( stm == NULL ) {
        return;
    }
    Type type = analyze_expr_statement(stm->argument.value);
    analyze_func_call_args(stm->argument.next);
}
// returns the type of the analyzed expr
Type analyze_expr_statement(AstExpr* stm) {
    Type type = analyze_expr_statement_inner(stm->expression_statement.value);
    //stm->expression_statement.type_name = type.type_name;
    return type;
}
Type analyze_expr_statement_inner(AstExpr* stm) {
    switch(stm->type) {
        char* ident;
        case AST_NUMBER:
            return Type_new("int",PRIMITIVE_TYPE);
        case AST_IDENTIFIER:
            ident = stm->identifier.token.value;  
            if( !Stack_find(&anlz.declared_vars, ident) ) {
                PANIC("Use of undeclered var: %s",ident);
            } else {
                Variable var = Stack_get(&anlz.declared_vars, ident);
                return var.type;
            }
        case AST_FUNC_CALL:
            return analyze_func_call(stm); 
        case AST_STRING:
            return Type_new("string",PRIMITIVE_TYPE);
    }
    if( stm->type == AST_UNARY_OPERATION ) {
        Type type = analyze_expr_statement_inner(stm->unary_operation.right);
        switch( stm->unary_operation.opp_token.kind ) {
            case NOT:
                if( !type_is(type.type_name,"bool") ) {
                    PANIC("attemted to NOT a type (%s) thats not a bool",type);
                } break;
            case MINUS:
                if( !type_is(type.type_name,"int","float") ) {
                    PANIC("attemted to NOT a type (%s) thats not a bool",type);
                } break;
            case PLUS_PLUS:
                if( !type_is(type.type_name,"int","float") ) {
                    PANIC("attemted to NOT a type (%s) thats not a bool",type);
                } break;
            case MINUS_MINUS:
                if( !type_is(type.type_name,"int","float") ) {
                    PANIC("attemted to NOT a type (%s) thats not a bool",type);
                } break;
            default: 
                PANIC("panicked");
        }
        return type;
    } else
    if( stm->type == AST_BINARY_OPERATION ) {
        Type left_type  = analyze_expr_statement_inner(stm->binary_operation.right);
        Type right_type = analyze_expr_statement_inner(stm->binary_operation.left);
        // TODO
        switch( stm->binary_operation.opp_token.kind ) {
            // same type return type
            case MULTIPLICATION:
            case PLUS:
            case DIVITION:
            case MINUS:
                if( !type_is(left_type.type_name,right_type.type_name) ) {
                    PANIC("Tried to %s {%s} and {%s} witch are not the same type",format_enum(stm->binary_operation.opp_token),left_type.type_name,right_type.type_name);
                }
                return left_type;

            // same type return bool
            case EQUAL:
            case NOT_EQUAL:
            case LESS_THEN: 
            case MORE_THEN:
            case LESS_EQUAL:
            case MORE_EQUAL:
                if( !type_is(left_type.type_name,right_type.type_name) ) {
                    PANIC("Tried to %s {%s} and {%s} witch are not the same type",format_enum(stm->binary_operation.opp_token),left_type.type_name,right_type.type_name);
                }
                return Type_new("bool",PRIMITIVE_TYPE);

            // same type and return VOID type
            // (a = a + b) ; type_of( (a = b) ) == VOID
            case ASSIGN:
                if( !type_is(left_type.type_name,right_type.type_name) ) {
                    PANIC("Tried to %s %s and %s witch are not the same type",format_enum(stm->binary_operation.opp_token),left_type.type_name,right_type.type_name);
                }
                return Type_new("void",PRIMITIVE_TYPE);


            // check struct field is in the struct then return the field type
            // b.c ; type_of( b.c ) == type_of( field c )
            case DOT: 
                PANIC("DOT operator analysis not implemented");

            // right side has to be an intiger
            case SUBSCRIPT_OPEN: 
                if( !type_is(left_type.type_name,"int") ) {
                    PANIC("Tried to %s %s and %s witch are not the same type",format_enum(stm->binary_operation.opp_token),left_type.type_name,right_type.type_name);
                }
                PANIC("%s %d: Getting type from subscript not implemented",__FILE__,__LINE__);
            default:
                PANIC("");
        }
    } else {
        PANIC("%s %d: Expected opp or terminal",__FILE__,__LINE__);
    }
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
    Analyzer_init();
    analyze_statements(ast);
    printf("analyzed ✓\n"); 
}
