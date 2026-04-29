#include "lexer.h"
#include "analyzer.h"
#include "parser.h"
#include "types.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

Type* CURR_EXPECTED_RETURN_TYPES[NESTED_FUNCTIONS];
int CURR_EXPECTED_RETURN_TYPE_IDX = 0;

bool IS_IN_EXTERN_BLOCK = false;

#define ASSERT(expr, fmt, ...) { \
    if (!expr) { \
        printf(fmt "\n", ##__VA_ARGS__); \
        exit(-1); \
    } \
}

#define PANIC(fmt, ...) { \
    printf("\033[1;31m" fmt "\033[0m" "\n", ##__VA_ARGS__); \
    __builtin_trap(); \
    exit(-1); \
}

Analyzer anlz;
int get_type_err; // 0 - OK , -1 - NOT FOUND

void Analyzer_init() {
    const Type PRIMITIVE_TYPES[] = PRIMITIVE_TYPES_ARRAY();

    Analyzer analyzer;
        analyzer.declared_vars = Stack_new();
        analyzer.types_idx = sizeof(PRIMITIVE_TYPES) / sizeof(PRIMITIVE_TYPES[0]);

    for (size_t i = 0; i < analyzer.types_idx; i++) {
        analyzer.types[i] = PRIMITIVE_TYPES[i];
    }
    anlz = analyzer;
}
Type* Analyzer_alloc_type(Type type) {
    anlz.types[anlz.types_idx] = type;
    return &anlz.types[anlz.types_idx++];
}
Type Analyzer_get_type(char* type_name,int* err) {
    for( int i = 0 ; i < anlz.types_idx ; i++ ) {
        char* curr = anlz.types[i].type_name;
        if( strcmp(type_name,curr) == 0 ) {
            *err = 0;
            return anlz.types[i];
        }
    }
    *err = -1;
    //PANIC("%s %d: Type not found in Analyzer_get_type(): %s",__FILE__,__LINE__,type_name);
}

#define MAX_SCOPE_NUMBERS 1000
#define MAX_SEEN_VARIABLES 1000
#define MAX_SEEN_VARIABLES_FRAMES 1000

typedef struct {
    int curr_scope_number;
    int numbers[MAX_SCOPE_NUMBERS];
    int numbers_pointer;
} ScopeNumbersStack;

typedef struct {
    int seen_variables_frames[MAX_SEEN_VARIABLES_FRAMES];
    int seen_variables_frames_pointer;

    char* seen_variables[MAX_SEEN_VARIABLES];
    int seen_variables_pointer;
} SeenVariablesStack;

typedef struct {
    ScopeNumbersStack numbers_stack;
    SeenVariablesStack seen_variables_stack;
} NameManglingContext;

void SeenVariablesStack_new_frame(SeenVariablesStack* stack) {
    ASSERT((stack->seen_variables_frames_pointer < MAX_SEEN_VARIABLES_FRAMES), "EXCEEDED THE MAXIMUM NUMBER OF SEEN_VARIABLES_FRAMES");
    stack->seen_variables_frames[stack->seen_variables_frames_pointer++] = stack->seen_variables_pointer;
}
void SeenVariablesStack_pop_frame(SeenVariablesStack* stack) {
    stack->seen_variables_pointer = stack->seen_variables_frames[--stack->seen_variables_frames_pointer];
}
void SeenVariablesStack_push(SeenVariablesStack* stack, char* variable) {
    ASSERT((stack->seen_variables_pointer < MAX_SEEN_VARIABLES), "EXCEEDED THE MAXIMUM NUMBER OF SEEN_VARIABLES");
    stack->seen_variables[stack->seen_variables_pointer++] = variable;
}

void ScopeNumbersStack_push(ScopeNumbersStack* stack) {
    ASSERT((stack->numbers_pointer < MAX_SCOPE_NUMBERS), "EXCEEDED THE MAXIMUM NUMBER OF SCOPE_NUMBERS");
    stack->numbers[stack->numbers_pointer++] = stack->curr_scope_number++;
}
int ScopeNumbersStack_get_number(ScopeNumbersStack* stack) {
    stack->numbers[stack->numbers_pointer-1];
}
void ScopeNumbersStack_pop(ScopeNumbersStack* stack) {
    stack->numbers_pointer--;
}

void NameManglingContext_new_scope(NameManglingContext* context) {
    SeenVariablesStack_new_frame(&context->seen_variables_stack);
    ScopeNumbersStack_push(&context->numbers_stack);
}
void NameManglingContext_pop_scope(NameManglingContext* context) {
    SeenVariablesStack_pop_frame(&context->seen_variables_stack);
    ScopeNumbersStack_pop(&context->numbers_stack);
}
void NameManglingContext_add_variable_to_current_scope(NameManglingContext* context, char* variable) {
    SeenVariablesStack_push(&context->seen_variables_stack, variable);
}

int NameManglingContext_get_scope_number_where_variable_last_seen(NameManglingContext* context, char* variable) {
    int variable_index;
    for( variable_index = context->seen_variables_stack.seen_variables_pointer - 1; ; variable_index-- ) {
        if( strcmp(variable,context->seen_variables_stack.seen_variables[variable_index]) == 0 ) {
            break;
        }
    }
    int j = context->seen_variables_stack.seen_variables_frames_pointer-1;
    while(context->seen_variables_stack.seen_variables_frames[j] > variable_index) {
        j--;
    }
    int scope_number = context->numbers_stack.numbers[j];
    return scope_number;
}

void mangle_variable(NameManglingContext* context, char** variable) {
    int scope_number = NameManglingContext_get_scope_number_where_variable_last_seen(context, *variable);
    StringBuilder sb = sb_new();
    sb_append(&sb,"scope%d_%s",scope_number,*variable);
    *variable = sb.buffer;
}

void mangle_expression(NameManglingContext* context, AstNode* stm) {
    switch(stm->type) {
        case AST_UNARY_OPERATION:
            mangle_expression(context,stm->unary_operation.right);
            return;
        case AST_BINARY_OPERATION:
            mangle_expression(context,stm->binary_operation.right);
            mangle_expression(context,stm->binary_operation.left);
            return;
        case AST_IDENTIFIER:
            mangle_variable(context,&stm->identifier.token.value);
            return;
        case AST_FUNC_CALL:
            AstNode* next = stm->function_call.args;
            while(next != NULL) {
                AstNode* arg = next->argument.value->expression_statement.expression;
                mangle_expression(context,arg);
                next = next->argument.next;
            }
            return;
        case AST_NUMBER:
        case AST_STRING:
            return;
        case AST_EXPRESSION_STATEMENT:
            mangle_expression(context,stm->expression_statement.expression);
            return;
        default:
            PANIC("NOT SUPPORTED: %s",format_ast_type(stm));
    }
}

void mangle_names_inner(NameManglingContext* context, AstNode* stm);
void mangle_names(AstNode* stm) {
    ASSERT(stm->type == AST_FUNCTION_DECLARATION, "PANICKED");

    NameManglingContext context = {0}; 

    // mangle function argument names first
    NameManglingContext_new_scope(&context);

    AstNode* arg_decl = stm->function_declaration.args;
    while(arg_decl != NULL) {
        NameManglingContext_add_variable_to_current_scope(&context,arg_decl->argument_decl.ident);
        mangle_variable(&context,&arg_decl->argument_decl.ident);

        arg_decl = arg_decl->argument_decl.next;
    }

    mangle_names_inner(&context,stm->function_declaration.body);

    NameManglingContext_pop_scope(&context);
}

void mangle_names_inner(NameManglingContext* context, AstNode* stm) {
    AstNode* next = stm;
    while( next != NULL ) {
        AstNode* curr = next;
        switch(curr->type) {
            case AST_BLOCK_STATEMENT:
                NameManglingContext_new_scope(context);
                mangle_names_inner(context, curr->block_statement.statements);
                NameManglingContext_pop_scope(context);
                next = curr->block_statement.next;
                break;
            case AST_DECLARATION:
                if( curr->declaration.value->expression_statement.expression != NULL ) {
                    mangle_expression(context,curr->declaration.value);
                }
                NameManglingContext_add_variable_to_current_scope(context,curr->declaration.name);
                mangle_variable(context,&curr->declaration.name);
                next = curr->declaration.next;
                break;
            case AST_EXPRESSION_STATEMENT:
                mangle_expression(context,curr->expression_statement.expression);
                next = next->expression_statement.next;
                break;
            case AST_RETURN_STATEMENT:
                mangle_expression(context,curr->return_statement.expression);
                next = next->return_statement.next;
                break;
            case AST_IF_STATEMENT:
                mangle_expression(context,curr->if_statement.condition);
                mangle_names_inner(context,curr->if_statement.body);
                mangle_names_inner(context,curr->if_statement.else_block);
                next = next->if_statement.next;
                break;
            case AST_FOR_STATEMENT:
                PANIC("TODO");
                next = next->for_statement.next;
                break;
            case AST_WHILE_STATEMENT:
                mangle_expression(context,curr->while_statement.condition);
                mangle_names_inner(context,curr->while_statement.body);
                next = next->while_statement.next;
                break;
            default:
                PANIC("NOT SUPPORTED: %s",format_ast_type(curr));
        }
    }
}

Variable Variable_new(Type* type, char* ident) {
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
void analyze_if(AstNode* stm) {
    if( anlz.declared_vars.frames_idx <= 1 ) {
        PANIC("'if' statement in global scope");
    }
    ASSERT( (stm->if_statement.condition->type == AST_EXPRESSION_STATEMENT), "Expression statement expected as IF condition");
    Type* condition_type = analyze_expr_statement(stm->if_statement.condition);

    if(condition_type->type_kind != BOOL_TYPE ){
        StringBuilder expr_sb = sb_new();
        print_expr_to_sb(&expr_sb,stm->if_statement.condition->expression_statement.expression);

        StringBuilder condition_type_sb = sb_new();
         Type_build_type_string(&condition_type_sb,condition_type);
        PANIC("Expression statement has to evaluate to BOOL_TYPE, got: {%s} '%s'",condition_type_sb.buffer,expr_sb.buffer);
    }

    analyze_statements(stm->if_statement.body);
    analyze_statements(stm->if_statement.else_block);
}

void analyze_function_decl(AstNode* stm) {
    char* ident = stm->function_declaration.name;

    if( anlz.declared_vars.frames_idx > 1 ) {
        PANIC("Function Declaration not in global scope: %s",ident);
    }

    analyze_type(stm->function_declaration.return_type);

    Type* func_type = Analyzer_alloc_type(Type_new(ident,FUNCTION_TYPE));
    func_type->function_type.return_type = stm->function_declaration.return_type;

    CURR_EXPECTED_RETURN_TYPES[CURR_EXPECTED_RETURN_TYPE_IDX++] = func_type->function_type.return_type;

    Variable function_var = Variable_new(func_type,ident);

    if( Stack_find_curr_frame(&anlz.declared_vars,function_var.ident) ) {
        PANIC("Redefinition of ident \"%s\" in the current scope",ident);
    }
    
    if( stm->function_declaration.args == NULL ) {
        function_var.type->function_type.arg_types = NULL;
    } else {
        TypeListNode* curr = (TypeListNode*)malloc(sizeof(TypeListNode));
        function_var.type->function_type.arg_types = curr; 
     
        AstNode* declared_arg = stm->function_declaration.args;
        while(1) { // Typing argument declarations

            char* ident = declared_arg->argument_decl.ident;

            analyze_type(declared_arg->argument_decl.type);
            Type* decl_arg_type = declared_arg->argument_decl.type;
            
            declared_arg = declared_arg->argument_decl.next;
            
            if(declared_arg == NULL) {
                *curr = (TypeListNode){.type = decl_arg_type, .next = NULL };
                break;
            } else {
                *curr = (TypeListNode){.type = decl_arg_type, .next = (TypeListNode*)malloc(sizeof(TypeListNode)) };
            }
            curr = curr->next;
        }
    }

    Stack_append(&anlz.declared_vars,function_var);
    Stack_new_frame(&anlz.declared_vars);

    AstNode*      arg           = stm->function_declaration.args;
    TypeListNode* arg_type_node = function_var.type->function_type.arg_types;

    while( arg != NULL ) { // Adding function args to the function scope
        char* ident = arg->argument_decl.ident;

        Variable var = Variable_new(arg_type_node->type,ident);
        Stack_append(&anlz.declared_vars,var);

        arg_type_node = arg_type_node->next;
        arg           = arg->argument_decl.next;
    }

    // analyze fn body
    AnalyzeStatementsReturn info = analyze_statements(stm->function_declaration.body->block_statement.statements); 
    if( !info.encountered_return && func_type->function_type.return_type->type_kind != VOID_TYPE && !IS_IN_EXTERN_BLOCK) {
        printf("WARNING: Function that has non void return type that doesnt have a return statement: %s\n",func_type->type_name);
    }
    Stack_pop_frame(&anlz.declared_vars);
    CURR_EXPECTED_RETURN_TYPE_IDX--;
}
void analyze_block(AstNode* stm) {
    Stack_new_frame(&anlz.declared_vars);
    analyze_statements(stm->block_statement.statements); 
    Stack_pop_frame(&anlz.declared_vars);
}
void analyze_decl(AstNode* stm) {
    char* var_ident      = stm->declaration.name;

    if( Stack_find_curr_frame(&anlz.declared_vars,var_ident) ) {
        PANIC("Redefinition of a var: %s",var_ident);
    }

    Type* expr_type;
    Type* declared_type;

    if( stm->declaration.type == NULL ){
        if( stm->declaration.value->expression_statement.expression == NULL ) {
            // banana := ; ??? should be impossible
            PANIC("%s %d: SHOULD BE UNREACHABLE",__FILE__,__LINE__);
            PANIC("Declaration of a variable '%s' without specified type",var_ident);
        }
            // banana := 5;
        expr_type = analyze_expr_statement(stm->declaration.value);
    } else {
        int err = analyze_type(stm->declaration.type);

        declared_type = stm->declaration.type;

        if( stm->declaration.value->expression_statement.expression == NULL ) {
            // 0 == ok, 1== arr len not specified
            ASSERT( (err == 0 ), "Array lenght has to be specified at var declaration if an expression is not provided '%s'",var_ident);
            // banana :int ;
            expr_type = declared_type;
        } else {
            // banana :int = "HELLO";
            expr_type = analyze_expr_statement(stm->declaration.value);
            // allowed 1,3
            int type_cmp_err = Type_cmp(declared_type,expr_type);
            if( type_cmp_err != 1 && type_cmp_err != 3) {
                StringBuilder expr_sb = sb_new();
                 print_expr_to_sb(&expr_sb,stm->declaration.value->expression_statement.expression);

                StringBuilder decl_type_sb = sb_new();
                 Type_build_type_string(&decl_type_sb,declared_type);
                StringBuilder expr_type_sb = sb_new();
                 Type_build_type_string(&expr_type_sb,expr_type);

                PANIC("Type specified in the declaration of var '%s' {%s} doesnt match the type of the expr provided: {%s} %s" ,
                           var_ident, 
                           decl_type_sb.buffer, 
                           expr_type_sb.buffer, 
                           expr_sb.buffer); 
            }
        }
    }

    stm->declaration.type = expr_type;

    Variable var = Variable_new(expr_type,var_ident);
    Stack_append(&anlz.declared_vars,var);
}
void analyze_for(AstNode* stm) {
    if( anlz.declared_vars.frames_idx <= 1 ) {
        PANIC("'for' statement in global scope");
    }
    Stack_new_frame(&anlz.declared_vars);

    //ASSERT((stm->for_statement.initial->type == AST_EXPRESSION_STATEMENT), "Expected Expression In For Initial");
    ASSERT((stm->for_statement.condition->type == AST_EXPRESSION_STATEMENT), "Expected Expression In For Condition");
    //ASSERT((stm->for_statement.iteration->type == AST_EXPRESSION_STATEMENT), "Expected Expression In For Iteration");

    analyze_statements(stm->for_statement.initial);
    ASSERT((analyze_expr_statement(stm->for_statement.condition)->type_kind == BOOL_TYPE), "For Condition expression has to return a bool");
    analyze_statements(stm->for_statement.iteration);

    analyze_statements(stm->for_statement.body->block_statement.statements); 
    Stack_pop_frame(&anlz.declared_vars);
}
void analyze_while(AstNode* stm) {
    if( anlz.declared_vars.frames_idx <= 1 ) {
        PANIC("'while' statement in global scope");
    }
    ASSERT((stm->while_statement.condition->type == AST_EXPRESSION_STATEMENT), "Expected an expression in while condition");
    ASSERT((analyze_expr_statement(stm->while_statement.condition)->type_kind == BOOL_TYPE),"While Condition expression has to return a bool"); 
    analyze_statements(stm->while_statement.body);
}
void analyze_return(AstNode* stm) {
    if( anlz.declared_vars.frames_idx <= 1 ) {
        PANIC("'return' statement in global scope");
    }
    Type* type;
    if( stm->return_statement.expression == NULL ) {
        type = &anlz.types[VOID_TYPE_IDX];
    } else { 
        ASSERT( (stm->return_statement.expression->type == AST_EXPRESSION_STATEMENT), "Expected expression statement in return statement");
        type = analyze_expr_statement(stm->return_statement.expression);
    }

    if( !Type_cmp(type,CURR_EXPECTED_RETURN_TYPES[CURR_EXPECTED_RETURN_TYPE_IDX-1]) ) {
        StringBuilder decl_type_sb = sb_new();
         Type_build_type_string(&decl_type_sb,CURR_EXPECTED_RETURN_TYPES[CURR_EXPECTED_RETURN_TYPE_IDX-1]);
        StringBuilder expr_type_sb = sb_new();
         Type_build_type_string(&expr_type_sb,type);
        PANIC("Wrong type in return statement {%s}, expected {%s} ",expr_type_sb.buffer, decl_type_sb.buffer);
    }
}
void analyze_extern_statement(AstNode* stm) {
    if( anlz.declared_vars.frames_idx > 1 ) {
        PANIC("Extern statement not in global scope");
    }
    if( IS_IN_EXTERN_BLOCK ) {
        PANIC("Cant have nested 'extern' blocks");
    }
    switch( stm->extern_statement.body->type ) {
        case AST_BLOCK_STATEMENT:
            IS_IN_EXTERN_BLOCK = true;
            analyze_statements(stm->extern_statement.body->block_statement.statements);
            IS_IN_EXTERN_BLOCK = false;
            break;
        default:
            PANIC("Extern statement not before block statement");
            //analyze_statements(stm->extern_statement.body);
            break;
    }
}
AnalyzeStatementsReturn analyze_statements(AstNode* stm) {
    AnalyzeStatementsReturn out = {.encountered_return = false };
    AstNode* next = stm;
    while( next != NULL ) {
        switch( next->type ) {
            case AST_FUNCTION_DECLARATION:
                analyze_function_decl(next); 
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
                out.encountered_return = true;
                analyze_return(next); 
                next = next->return_statement.next;
                break;
            /*
            case AST_STRUCT_DECLARATION:    
                analyze_struct_decl(next);
                next = next->struct_declaration.next;
                break;
            */
            case AST_EXTERN_STATEMENT:    
                analyze_extern_statement(next);
                next = next->extern_statement.next;
                break;

            default:
                PANIC("NOT SUPPORTED: %s",format_ast_type(next));
        }
    }
    return out;
}

void analyze_program_ast(AstNode* ast) {
    Analyzer_init();
    mangle_names(ast);
    analyze_statements(ast);
}

int analyze_type(Type* type) {
    int err = 0;
    int depth = 0;
    while( type->type_kind != UNKNOWN_TYPE && type->type_kind != VOID_TYPE) {
        switch( type->type_kind ) {
            case ARRAY_TYPE:
                PANIC("!!!Unreachable!!!");
                PANIC("Arrays Not supported");
            case POINTER_TYPE: 
                type = type->pointer_type.sub_type;
                break;
            default:
                PANIC("%s %d:PANICKED %s",__FILE__,__LINE__,Type_format_type_kind(*type));
        }
        depth++;
    }
    char* type_name = type->type_name;
    *type = Analyzer_get_type(type_name,&get_type_err); 
      ASSERT( ( get_type_err == 0 ), "%s %d: Type not found {%s}",__FILE__,__LINE__,type_name);
    return err;
}
// returns the type of the analyzed expr
Type* analyze_expr_statement(AstNode* stm) {
    Type* type = analyze_expr_statement_inner(stm->expression_statement.expression);
    stm->expression_statement.type = type;
    return type;
}
Type* analyze_function_call(AstNode* stm) {
    if( anlz.declared_vars.frames_idx <= 1 ) {
        PANIC("'function call' statement in global scope");
    }
    Variable var;
    char* ident = stm->function_call.identifier.value;  
    if( !Stack_find(&anlz.declared_vars, ident) ) {
        PANIC("Use of undeclered function: %s",ident);
    } else {
        var = Stack_get(&anlz.declared_vars, ident);
        if( var.type->type_kind != FUNCTION_TYPE ) {
            PANIC("Tried to call variable '%s' of type {%s} as a function",var.ident,var.type->type_name);
        }
    }
    int arg_counter = 1;
    AstNode* curr_arg = stm->function_call.args;
    TypeListNode* curr_arg_decl = var.type->function_type.arg_types;
    while(1) {
        if( curr_arg == NULL ) {
            break;
        }
        Type* arg_type      = analyze_expr_statement(curr_arg->argument.value);
        if( curr_arg_decl == NULL ) {
            PANIC("In call to function '%s' expected %d argument/s got additianal argument of type {%s}",var.ident,arg_counter,arg_type->type_name);
        }
        Type* arg_decl_type = curr_arg_decl->type;
        int type_cmp_err =Type_cmp(arg_decl_type,arg_type);
        if( type_cmp_err != 1 && type_cmp_err != 3) {
            StringBuilder expr_sb = sb_new();
            print_expr_to_sb(&expr_sb,curr_arg->argument.value->expression_statement.expression);

            StringBuilder decl_arg_type_sb = sb_new();
             Type_build_type_string(&decl_arg_type_sb,arg_decl_type);
            StringBuilder arg_type_sb = sb_new();
             Type_build_type_string(&arg_type_sb,arg_type);
            PANIC("In call to function '%s' argument number:%d doesnt match the argument declaration. Expected {%s} and got {%s} '%s'",
                  var.ident,
                  arg_counter,
                  decl_arg_type_sb.buffer,
                  arg_type_sb.buffer,
                  expr_sb.buffer
                  );
        }

        curr_arg_decl = curr_arg_decl->next;
        curr_arg = curr_arg->argument.next;
    }
    return var.type->function_type.return_type;
}

Type* analyze_expr_statement_inner(AstNode* stm) {
    switch(stm->type) {
        char* ident;
        case AST_NUMBER:
            return stm->number.type;
        case AST_IDENTIFIER:
            ident = stm->identifier.token.value;  
            if( !Stack_find(&anlz.declared_vars, ident) ) {
                PANIC("Use of undeclered var: %s",ident);
            } else {
                Variable var = Stack_get(&anlz.declared_vars, ident);
                stm->identifier.type = var.type;
                return var.type;
            }
        case AST_FUNC_CALL:
            return analyze_function_call(stm); 
        case AST_STRING:
            PANIC("strings not suported");
            /*
            Type ptr_type = Type_new(NULL,POINTER_TYPE);
            ptr_type.pointer_type.sub_type = (Type*)malloc(sizeof(Type));
            *ptr_type.pointer_type.sub_type = anlz.types[INTIGER_TYPE_IDX];
            ptr_type.pointer_type.sub_type->intiger_type.size = BITS_8;
            return ptr_type;
            */
    }
    if( stm->type == AST_UNARY_OPERATION ) {
        Type* type = analyze_expr_statement_inner(stm->unary_operation.right);
        switch( stm->unary_operation.opp_token.kind ) {
            case NOT:
                stm->unary_operation.is_lvalue = false;
                if( type->type_kind != BOOL_TYPE ) {
                    StringBuilder expr_sb = sb_new();
                     print_expr_to_sb(&expr_sb,stm);

                    StringBuilder type_sb = sb_new();
                     Type_build_type_string(&type_sb,type);
                    PANIC("attemted to NOT a type (%s) thats not a bool %s",type_sb.buffer,expr_sb.buffer);
                } 
                stm->unary_operation.op_type = &anlz.types[BOOL_TYPE_IDX];
                stm->unary_operation.return_type = &anlz.types[BOOL_TYPE_IDX];
                return &anlz.types[BOOL_TYPE_IDX];

            case MINUS:
                stm->unary_operation.is_lvalue = false;
                ASSERT( (type->type_kind == INTIGER_TYPE || type->type_kind == FLOAT_TYPE),
                       "Tried to MINIUS (negate) a (%s), thats not a number", type->type_name)

                stm->unary_operation.op_type = type;
                stm->unary_operation.return_type = type;
                return type;
            case PLUS_PLUS:
                if( !is_lvalue(stm->unary_operation.right) ){
                    StringBuilder expr_sb = sb_new();
                     print_expr_to_sb(&expr_sb,stm);

                    StringBuilder type_sb = sb_new();
                     Type_build_type_string(&type_sb,type);
                    PANIC("got {%s} but lvalue required as trying to '++' increment %s",type_sb.buffer,expr_sb.buffer);
                }

                stm->unary_operation.is_lvalue = true;
                ASSERT( (type->type_kind == INTIGER_TYPE || type->type_kind == FLOAT_TYPE),
                       "Tried to PLUS_PLUS (increment) a (%s), thats not a number", type->type_name)

                stm->unary_operation.op_type = type;
                stm->unary_operation.return_type = type;
                return type;
            case MINUS_MINUS:
                if( !is_lvalue(stm->unary_operation.right) ){
                    StringBuilder expr_sb = sb_new();
                     print_expr_to_sb(&expr_sb,stm);

                    StringBuilder type_sb = sb_new();
                     Type_build_type_string(&type_sb,type);
                    PANIC("got {%s} but lvalue required as trying to '-- decrement %s",type_sb.buffer,expr_sb.buffer);
                }

                stm->unary_operation.is_lvalue = true;
                ASSERT( (type->type_kind == INTIGER_TYPE || type->type_kind == FLOAT_TYPE),
                       "Tried to MINUS_MINUS (decrement) a (%s), thats not a number", type->type_name)

                stm->unary_operation.op_type = type;
                stm->unary_operation.return_type = type;
                return type;
            case AMPERSAND:
                if( !is_lvalue(stm->unary_operation.right) ){
                    StringBuilder expr_sb = sb_new();
                     print_expr_to_sb(&expr_sb,stm);

                    StringBuilder type_sb = sb_new();
                     Type_build_type_string(&type_sb,type);
                    PANIC("got {%s} but lvalue required as '&' operand %s",type_sb.buffer,expr_sb.buffer);
                }

                stm->unary_operation.is_lvalue = false;

                Type ptr_type = Type_new(NULL,POINTER_TYPE);
                ptr_type.pointer_type.sub_type = type;

                return Analyzer_alloc_type(ptr_type);
            case STAR:
                if( type->type_kind != POINTER_TYPE ) {
                    StringBuilder expr_sb = sb_new();
                     print_expr_to_sb(&expr_sb,stm);

                    StringBuilder type_sb = sb_new();
                     Type_build_type_string(&type_sb,type);
                    PANIC("attempted to dereference a {%s} type thats not a pointer %s",type_sb.buffer,expr_sb.buffer);
                }

                stm->unary_operation.is_lvalue = true;
                Type* derefed_type = type->pointer_type.sub_type;
                return derefed_type;
            default: 
                PANIC("%s %d: Panicked",__FILE__,__LINE__);
        }
        //return type;
    } else
    if( stm->type == AST_BINARY_OPERATION ) {
        Type* left_type  ;
        Type* right_type ;
        switch( stm->binary_operation.opp_token.kind ) {
            // same type return type
            case STAR:
            case PLUS:
            case DIVITION:
            case MINUS:
                stm->binary_operation.is_lvalue = false;
                left_type  = analyze_expr_statement_inner(stm->binary_operation.left);
                right_type = analyze_expr_statement_inner(stm->binary_operation.right);
                if( Type_cmp(left_type,right_type) != 1) {
                    PANIC("Tried to %s {%s} and {%s} witch are not the same type",
                          format_token(stm->binary_operation.opp_token),
                          Type_format_type_kind(*left_type),
                          Type_format_type_kind(*right_type));
                }
                stm->binary_operation.return_type = left_type;
                stm->binary_operation.op_type = left_type;
                return left_type;

            // same type return bool
            case EQUAL:
            case NOT_EQUAL:
            case LESS_THEN: 
            case MORE_THEN:
            case LESS_EQUAL:
            case MORE_EQUAL:
                stm->binary_operation.is_lvalue = false;

                left_type  = analyze_expr_statement_inner(stm->binary_operation.left);
                right_type = analyze_expr_statement_inner(stm->binary_operation.right);
                if( Type_cmp(left_type,right_type) != 1) {
                    StringBuilder expr_sb = sb_new();
                     print_expr_to_sb(&expr_sb,stm);

                    StringBuilder left_type_sb = sb_new();
                     Type_build_type_string(&left_type_sb,left_type);
                    StringBuilder right_type_sb = sb_new();
                     Type_build_type_string(&right_type_sb,right_type);
                    PANIC("Tried to %s {%s} and {%s} witch are not the same type %s",format_token(stm->binary_operation.opp_token),left_type_sb.buffer,right_type_sb.buffer,expr_sb.buffer);
                }
                stm->binary_operation.op_type = left_type;
                stm->binary_operation.return_type = &anlz.types[BOOL_TYPE_IDX];
                return &anlz.types[BOOL_TYPE_IDX];

            // same type and return VOID type
            // (a = a + b) ; type_of( (a = b) ) == VOID
            case ASSIGN:
                stm->binary_operation.is_lvalue = false;

                left_type  = analyze_expr_statement_inner(stm->binary_operation.left);
                right_type = analyze_expr_statement_inner(stm->binary_operation.right);

                if( !is_lvalue(stm->binary_operation.left) ){
                    PANIC("Tried to assing but left operand isnt an lvalue");
                }

                if( Type_cmp(left_type,right_type) != 1) {
                    StringBuilder expr_sb = sb_new();
                     print_expr_to_sb(&expr_sb,stm);

                    StringBuilder left_type_sb = sb_new();
                     Type_build_type_string(&left_type_sb,left_type);
                    StringBuilder right_type_sb = sb_new();
                     Type_build_type_string(&right_type_sb,right_type);
                    PANIC("Tried to ASSIGN {%s} to {%s} %s", right_type_sb.buffer, left_type_sb.buffer, expr_sb.buffer);
                }
                stm->binary_operation.return_type = &anlz.types[VOID_TYPE_IDX];
                return &anlz.types[VOID_TYPE_IDX];

            // check struct field is in the struct then return the field type
            // b.c ; type_of( b.c ) == type_of( field c )
            // b.c[10] ; type_of( b.c ) == type_of( field c )
            // right side is a name of a field,
            // left side can be any type but a STRUCT_TYPE is the only valid type
            case DOT: 
                stm->binary_operation.is_lvalue = true;

                left_type  = analyze_expr_statement_inner(stm->binary_operation.left);
                if( left_type->type_kind != STRUCT_TYPE) {
                    StringBuilder expr_sb = sb_new();
                     print_expr_to_sb(&expr_sb,stm);

                    StringBuilder left_type_sb = sb_new();
                     Type_build_type_string(&left_type_sb,left_type);
                    PANIC("Tried to use DOT operator on {%s} %s", left_type_sb.buffer, expr_sb.buffer);
                }

                AstNode* field_name_identifier = stm->binary_operation.right;
                    ASSERT( (field_name_identifier->type == AST_IDENTIFIER), "Only an identifier can be a field name", "");
                char* field_name = field_name_identifier->identifier.token.value;

                Type* field_type = Type_get_field_type(left_type,field_name);
                stm->binary_operation.return_type = field_type;
                return field_type;

            case SUBSCRIPT_OPEN: // right side has to be an intiger
                stm->binary_operation.is_lvalue = true;
                PANIC("Indexing Not Supported");
            default:
                PANIC("");
        }
    } else {
        PANIC("%s %d: Expected opp or terminal",__FILE__,__LINE__);
    }
}

bool is_lvalue(AstNode* node) {
    switch(node->type) {
        case AST_IDENTIFIER:
            return 1;
        case AST_NUMBER:
        case AST_STRING:
        case AST_FUNC_CALL:
            return 0;
        case AST_BINARY_OPERATION:
            return node->binary_operation.is_lvalue;
        case AST_UNARY_OPERATION:
            return node->unary_operation.is_lvalue;
        default:
            PANIC("")
    }
}
const char* format_ast_type(AstNode* stm) {
    switch( stm->type ) {
        case AST_EXTERN_STATEMENT:      return "AST_EXTERN_STATEMENT";
        case AST_FUNCTION_DECLARATION:  return "AST_FUNCTION_DECLARATION";
        case AST_BLOCK_STATEMENT:       return "AST_BLOCK_STATEMENT";
        case AST_DECLARATION:           return "AST_DECLARATION";
        case AST_IF_STATEMENT:          return "AST_IF_STATEMENT";
        case AST_FOR_STATEMENT:         return "AST_FOR_STATEMENT";
        case AST_WHILE_STATEMENT:       return "AST_WHILE_STATEMENT";
        case AST_RETURN_STATEMENT:      return "AST_RETURN_STATEMENT";
        case AST_EXPRESSION_STATEMENT:  return "AST_EXPRESSION_STATEMENT";
        case AST_BINARY_OPERATION:      return "AST_BINARY_OPERATION";
        case AST_STRUCT_DECLARATION:    return "AST_STRUCT_DECLARATION";
        case AST_FUNC_CALL:             return "AST_FUNC_CALL";
        default:
            PANIC("%s %d: UNKNOWN AST NODE TYPE",__FILE__,__LINE__);
        }
}
