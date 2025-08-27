#include "lexer.h"
#include "analyzer.h"
#include "parser.h"
#include "types.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

const Type PRIMITIVE_TYPES[] = PRIMITIVE_TYPES_ARRAY();

#define ASSERT(expr, fmt, ...) { \
    if (!expr) { \
        printf(fmt "\n", ##__VA_ARGS__); \
        exit(-1); \
    } \
}
//*(int*)0 = 0; \

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}

Analyzer anlz;
void Analyzer_init() {
    Analyzer analyzer;
        analyzer.declared_vars = Stack_new();
        analyzer.types_idx = sizeof(PRIMITIVE_TYPES) / sizeof(PRIMITIVE_TYPES[0]);
    for (size_t i = 0; i < analyzer.types_idx; i++) {
        analyzer.types[i] = PRIMITIVE_TYPES[i];
    }
    anlz = analyzer;
}


void Analyzer_append_type(Type type) {
    if( anlz.types_idx < 1000 ) {
        anlz.types[anlz.types_idx++] = type;
    } else {
        PANIC("Max number of types exceeded");
    }
}

int get_type_err; // 0 - OK , -1 - NOT FOUND
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

int type_is_impl(const char* type, ...) {
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
        case AST_STRUCT_DECLARATION:    return "AST_STRUCT_DECLARATION";
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

    Type func_type    = Type_new(ident,FUNCTION_TYPE);

    Type* return_type = (Type*)malloc(sizeof(Type));
    *return_type = create_type_from_ast_node(stm);

    func_type.function_type.return_type = return_type;

    Variable var = Variable_new(func_type,ident);

    if( Stack_find(&anlz.declared_vars,var.ident) ) {
        PANIC("Redefinition of ident \"%s\" as a function",ident);
    }
    
    if( stm->function_declaration.args == NULL ) {
        var.type.function_type.arg_types = NULL;
    } else {
        TypeListNode* curr = (TypeListNode*)malloc(sizeof(TypeListNode));
        var.type.function_type.arg_types = curr; 
     
        AstExpr* arg = stm->function_declaration.args;
        while(1) { // Adding function args to the function scope

            char* arg_type_name  = arg->argument_decl.type_name;
            char* ident = arg->argument_decl.ident.value;

            Type arg_type = create_type_from_ast_node(arg);

            arg = arg->argument_decl.next;
            
            if(arg == NULL) {
                *curr = (TypeListNode){.type = arg_type, .next = NULL };
                break;
            } else {
                *curr = (TypeListNode){.type = arg_type, .next = (TypeListNode*)malloc(sizeof(TypeListNode)) };
            }
            curr = curr->next;
        }
    }

    Stack_append(&anlz.declared_vars,var);
    Stack_new_frame(&anlz.declared_vars);

    AstExpr* arg = stm->function_declaration.args;
    while( arg != NULL ) { // Adding function args to the function scope

        char* type_name  = arg->argument_decl.type_name;
        char* ident = arg->argument_decl.ident.value;
        Type  type = Analyzer_get_type(type_name,&get_type_err);
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
    char* var_ident      = stm->declaration.name;
    char* var_type_name  = stm->declaration.type_name;

    if( Stack_find_curr_frame(&anlz.declared_vars,var_ident) ) {
        PANIC("Redefinition of a var: %s",var_ident);
    }

    Type expr_type;
    Type decl_var_type;

    if( var_type_name == NULL ){
        if( stm->declaration.value->expression_statement.value == NULL ) {
            // banana := ; ??? should be impossible
            PANIC("%s %d: SHOULD BE UNREACHABLE",__FILE__,__LINE__);
            PANIC("Declaration of a variable '%s' without specified type",var_ident);
        }
            // banana := 5;
        expr_type = analyze_expr_statement(stm->declaration.value);
    } else {
        decl_var_type = create_type_from_ast_node(stm);

        if( stm->declaration.value->expression_statement.value == NULL ) {
            // banana :int ;
            expr_type = decl_var_type;
        } else {
            // banana :int = "HELLO";
            expr_type = analyze_expr_statement(stm->declaration.value);
            if( !Type_cmp(&expr_type,&decl_var_type) ) {
                StringBuilder expr_sb = sb_new();
                 print_expr_to_sb(&expr_sb,stm->declaration.value->expression_statement.value);

                StringBuilder decl_type_sb = sb_new();
                 Type_build_type_string(&decl_type_sb,&decl_var_type);
                StringBuilder expr_type_sb = sb_new();
                 Type_build_type_string(&expr_type_sb,&expr_type);

                PANIC("Type specified in the declaration of var '%s' {%s} doesnt match the type of the expr provided: {%s} %s" ,
                           var_ident, 
                           decl_type_sb.buffer, 
                           expr_type_sb.buffer, 
                           expr_sb.buffer); 
            }
        }
    }

    Variable var = Variable_new(expr_type,var_ident);
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
    int arg_counter = 1;
    AstExpr* curr_arg = stm->func_call.args;
    TypeListNode* curr_arg_decl = var.type.function_type.arg_types;
    while(1) {
        if( curr_arg == NULL ) {
            break;
        }
        Type arg_type      = analyze_expr_statement(curr_arg->argument.value);
        if( curr_arg_decl == NULL ) {
            PANIC("In call to function '%s' expected %d argument/s got additianal argument of type {%s}",var.ident,arg_counter,arg_type.type_name);
        }
        Type arg_decl_type = curr_arg_decl->type;

        if( !Type_cmp(&arg_type,&arg_decl_type)) {
            StringBuilder expr_sb = sb_new();
            print_expr_to_sb(&expr_sb,curr_arg->argument.value->expression_statement.value);

            StringBuilder decl_arg_type_sb = sb_new();
             Type_build_type_string(&decl_arg_type_sb,&arg_decl_type);
            StringBuilder arg_type_sb = sb_new();
             Type_build_type_string(&arg_type_sb,&arg_type);
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
    return *var.type.function_type.return_type;
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
                if( !Type_cmp(&type,&PRIMITIVE_TYPES[BOOL_TYPE_IDX]) ) {
                    PANIC("attemted to NOT a type (%s) thats not a bool",type.type_name);
                } break;
            case MINUS:
                if( !Type_cmp(&type,&PRIMITIVE_TYPES[INT_TYPE_IDX]) && !Type_cmp(&type,&PRIMITIVE_TYPES[FLOAT_TYPE_IDX]) ) {
                    PANIC("attemted to MINUS a type (%s) thats not a bool",type.type_name);
                } break;
            case PLUS_PLUS:
                if( !Type_cmp(&type,&PRIMITIVE_TYPES[INT_TYPE_IDX]) && !Type_cmp(&type,&PRIMITIVE_TYPES[FLOAT_TYPE_IDX]) ) {
                    PANIC("attemted to PLUS_PLUS a type (%s) thats not a bool",type.type_name);
                } break;
            case MINUS_MINUS:
                if( !Type_cmp(&type,&PRIMITIVE_TYPES[INT_TYPE_IDX]) && !Type_cmp(&type,&PRIMITIVE_TYPES[FLOAT_TYPE_IDX]) ) {
                    PANIC("attemted to MINUS_MINUS a type (%s) thats not a bool",type.type_name);
                } break;
            default: 
                PANIC("panicked");
        }
        return type;
    } else
    if( stm->type == AST_BINARY_OPERATION ) {
        Type left_type  ;
        Type right_type ;
        // TODO
        switch( stm->binary_operation.opp_token.kind ) {
            // same type return type
            case STAR:
            case PLUS:
            case DIVITION:
            case MINUS:
                left_type  = analyze_expr_statement_inner(stm->binary_operation.left);
                right_type = analyze_expr_statement_inner(stm->binary_operation.right);
                if( !Type_cmp(&left_type,&right_type) ) {
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
                left_type  = analyze_expr_statement_inner(stm->binary_operation.left);
                right_type = analyze_expr_statement_inner(stm->binary_operation.right);
                if( !Type_cmp(&left_type,&right_type) ) {
                    PANIC("Tried to %s {%s} and {%s} witch are not the same type",format_enum(stm->binary_operation.opp_token),left_type.type_name,right_type.type_name);
                }
                return Type_new("bool",PRIMITIVE_TYPE);

            // same type and return VOID type
            // (a = a + b) ; type_of( (a = b) ) == VOID
            case ASSIGN:
                left_type  = analyze_expr_statement_inner(stm->binary_operation.left);
                right_type = analyze_expr_statement_inner(stm->binary_operation.right);
                if( !Type_cmp(&left_type,&right_type) ) {
                    StringBuilder expr_sb = sb_new();
                     print_expr_to_sb(&expr_sb,stm);

                    StringBuilder left_type_sb = sb_new();
                     Type_build_type_string(&left_type_sb,&left_type);
                    StringBuilder right_type_sb = sb_new();
                     Type_build_type_string(&right_type_sb,&right_type);
                    PANIC("Tried to ASSIGN {%s} to {%s} %s", left_type_sb.buffer, right_type_sb.buffer, expr_sb.buffer);
                }
                return Type_new("void",PRIMITIVE_TYPE);


            // check struct field is in the struct then return the field type
            // b.c ; type_of( b.c ) == type_of( field c )
            // b.c[10] ; type_of( b.c ) == type_of( field c )
            // right side is a name of a field,
            // left side can be any type but a STRUCT_TYPE is the only valid type
            case DOT: 
                left_type  = analyze_expr_statement_inner(stm->binary_operation.left);
                    ASSERT( (left_type.type_kind == STRUCT_TYPE), "Tried to use DOT operator on a non struct type: %s{%s}",Type_format_type_kind(left_type),left_type.type_name);
                AstExpr* field_name_identifier = stm->binary_operation.right;
                    ASSERT( (field_name_identifier->type == AST_IDENTIFIER), "Only an identifier can be a field name", "");
                char* field_name = field_name_identifier->identifier.token.value;

                Type field_type = Type_get_field_type(left_type,field_name);
                return field_type;
                PANIC("DOT operator analysis not implemented");

            // right side has to be an intiger
            case SUBSCRIPT_OPEN: 
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
void analyze_struct_decl(AstExpr* stm) {
    char* struct_name = stm->struct_declaration.name;
    Type t = Analyzer_get_type(struct_name,&get_type_err);
    if( get_type_err == 0 ) { // Type found 
        PANIC("Redefinition of type {%s} as a struct",t.type_name);
    }
    Type struct_type = Type_new(struct_name,STRUCT_TYPE);

    AstExpr* curr_field = stm->struct_declaration.body->block_statement.statements;

    if( curr_field == NULL ) {
        struct_type.struct_type.fields = NULL; 
    } else {
        FieldListNode* curr_field_node = (FieldListNode*)malloc(sizeof(FieldListNode));
        struct_type.struct_type.fields = curr_field_node; 
        while(1) {
            ASSERT( ( curr_field->type == AST_DECLARATION ),       "Only declarations allowed in struct declaration body");
            ASSERT( ( curr_field->declaration.type_name != NULL ), "Type of the field must be specified in struct declaration");

            Type curr_field_type = create_type_from_ast_node(curr_field);



            char* curr_field_name = curr_field->declaration.name;
            curr_field = curr_field->declaration.next;

            if(curr_field == NULL) {
                *curr_field_node = (FieldListNode){ .type=curr_field_type, .name = curr_field_name, .next = NULL };
                break;
            } else {
                *curr_field_node = (FieldListNode){ .type=curr_field_type, .name = curr_field_name, .next = (FieldListNode*)malloc(sizeof(FieldListNode)) };
            }
            curr_field_node = curr_field_node->next;
        }
    }

    Analyzer_append_type(struct_type);
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
            case AST_STRUCT_DECLARATION:    
                analyze_struct_decl(next);
                next = next->struct_declaration.next;
                break;

            default:
                PANIC("NOT SUPPORTED: %s",format_ast_type(next));
        }
    }
}

void analyze_program_ast(AstExpr* ast) {
    Analyzer_init();
    analyze_statements(ast);
    printf("\e[0;32manalyzed ✓\e[0m\n"); 
}

// creates a type from an AST node that should contain a type
Type create_type_from_ast_node(AstExpr* node) {
    char* type_name;
    int star_number;
    switch( node->type ) {
        case AST_DECLARATION:
            type_name  = node->declaration.type_name;
            star_number  = node->declaration.star_number;
            break;
        case AST_FUNCTION_DECLARATION:
            type_name  = node->function_declaration.return_type_name;
            star_number  = node->function_declaration.star_number;
            break;
        case AST_ARGUMENT_DECLARATION:
            type_name  = node->argument_decl.type_name;
            star_number  = node->argument_decl.star_number;
            break;
        default:
            PANIC("%s %d: Unsuported AST type",__FILE__,__LINE__);
    }

    Type type;
    if( node->declaration.star_number ) {
        type = Type_new(NULL,POINTER_TYPE);

        Type* sub_type = (Type*)malloc(sizeof(Type));
        type.pointer_type.sub_type = sub_type;

        for( int n = 1; n < node->declaration.star_number; n++ ) {
            *sub_type = Type_new(NULL,POINTER_TYPE);
            sub_type->pointer_type.sub_type = (Type*)malloc(sizeof(Type));
            sub_type = sub_type->pointer_type.sub_type;
        }
        *sub_type = Analyzer_get_type(type_name,&get_type_err); 
          ASSERT( ( get_type_err == 0 ), "%s %d: Type not found {%s}",__FILE__,__LINE__,type_name);
    } else {
        type = Analyzer_get_type(type_name,&get_type_err); 
          ASSERT( ( get_type_err == 0 ), "%s %d: Type not found {%s}",__FILE__,__LINE__,type_name);
    }
    return type;
}
