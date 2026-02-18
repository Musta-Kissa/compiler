#include "backend.h"
#include "lexer.h"
#include "dyn_arrays_macro.h"

#define ASSERT(expr, fmt, ...) { \
    if (!expr) { \
        printf(fmt "\n", ##__VA_ARGS__); \
        exit(-1); \
    } \
}
#define PANIC(fmt, ...) { \
    printf("\033[1;31m" fmt "\033[0m" "\n", ##__VA_ARGS__); \
    *(int*)0=0;\
    exit(-1); \
}

char* get_location_str(VariableLocation location) {
    static char buffer[256];
    switch( location.type ) {
        case NOT_ASSIGNED:
            PANIC("%s %d: PANICKED",__FILE__,__LINE__);
        case REGISTER:
            return get_register_str(location.register_location.register_); 
        case STACK:
            snprintf(buffer, sizeof(buffer), "[rbp%+d]", location.stack_location.base_offset);
            return buffer;
    }
}

const VariableLocation get_location_of_variable(const VariableInfoList variables_list, const char* variable_identifier) {
    for(int idx = 0; idx < variables_list.count; idx++) {
        if( strcmp(variable_identifier,variables_list.items[idx].identifier) == 0 ) {
            return variables_list.items[idx].location;
        }
    }
    PANIC("%s %d: PANICKED",__FILE__,__LINE__);
}

VariableInfo* find_var(const VariableInfoList list, const char* name) {
    for(int idx = 0; idx < list.count; idx++) {
        if( strcmp(name,list.items[idx].identifier) == 0 ) {
            return &list.items[idx];
        }
    }
    return NULL;
}

void set_location_for_variable(VariableInfoList variables_list, char* name, VariableLocation location) {
    VariableInfo* variable = find_var(variables_list, name);
    variable->location = location;
}

bool is_value_ast(AstNode* stm) {
    switch(stm->type) {
        case AST_FUNC_CALL:
        case AST_IDENTIFIER:
        case AST_NUMBER:
        case AST_STRING:
            return true;
        case AST_EXPRESSION_STATEMENT:
            return is_value_ast(stm->expression_statement.expression);
        default:
            return false;
    }
}
void update_vars(VariableInfoList* variables_list, AstNode* stm, int current_line) {
    const char* name;
    Type* type;

    switch( stm->type ) {
        case AST_IDENTIFIER:
            name = stm->identifier.token.value;
            type = stm->identifier.type;

            VariableInfo* variable = find_var(*variables_list,name);
            variable->last_line_used = current_line;
            break;
        case AST_DECLARATION:
            name = stm->declaration.name;
            type = stm->declaration.type;

            int arg_size = Type_size_of(type);
            VariableInfo info = {.identifier = name, 
                                 .first_line_used = current_line, 
                                 .last_line_used = current_line, 
                                 .location = NOT_ASSIGNED };
            da_append_ref(variables_list,info);
            break;
        default:
            PANIC("%s %d: PANICKED",__FILE__,__LINE__);
    }
}
void update_variable_info_with_expression(VariableInfoList* variables_list, AstNode* stm,int current_line) {
    switch(stm->type) {
        case AST_UNARY_OPERATION:
            update_variable_info_with_expression(variables_list,stm->unary_operation.right,current_line);
            return;
        case AST_BINARY_OPERATION:
            update_variable_info_with_expression(variables_list,stm->binary_operation.right,current_line);
            update_variable_info_with_expression(variables_list,stm->binary_operation.left,current_line);
            return;
        case AST_IDENTIFIER:
            update_vars(variables_list,stm,current_line);
            return;
        case AST_FUNC_CALL:
            AstNode* next = stm->function_call.args;
            while(next != NULL) {
                AstNode* arg = next->argument.value->expression_statement.expression;
                update_variable_info_with_expression(variables_list,arg,current_line);
                next = next->argument.next;
            }
            return;
        case AST_NUMBER:
        case AST_STRING:
            return;
        default:
            PANIC("%s %d: NOT SUPPORTED: %s",__FILE__,__LINE__,format_ast_type(stm));
  }
}
int analyze_variable_lifetimes(VariableInfoList* variables_list, AstNode* stm, int current_line) {
    AstNode* next = stm;
    while( next != NULL ) {
        AstNode* curr = next;
        switch(curr->type) {
            case AST_BLOCK_STATEMENT:
                current_line = analyze_variable_lifetimes(variables_list,curr->block_statement.statements,current_line);
                next = curr->block_statement.next;
                break;
            case AST_DECLARATION:
                update_vars(variables_list,curr,current_line);
                update_variable_info_with_expression(variables_list,curr->declaration.value->expression_statement.expression,current_line);
                current_line++;
                next = curr->declaration.next;
                break;
            case AST_EXPRESSION_STATEMENT:
                update_variable_info_with_expression(variables_list,curr->expression_statement.expression,current_line);
                current_line++;
                next = next->expression_statement.next;
                break;
            case AST_RETURN_STATEMENT:
                update_variable_info_with_expression(variables_list,curr->return_statement.expression->expression_statement.expression,current_line);
                current_line++;
                next = next->return_statement.next;
                break;
            case AST_IF_STATEMENT:
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                next = next->if_statement.next;
                break;
            case AST_FOR_STATEMENT:
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                next = next->for_statement.next;
                break;
            case AST_WHILE_STATEMENT:
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                next = next->while_statement.next;
                break;
            /*
            AST_BINARY_OPERATION,   
            AST_UNARY_OPERATION,    
            AST_FUNC_CALL,          
            */
            default:
                PANIC("%s %d: NOT SUPPORTED: %s",__FILE__,__LINE__,format_ast_type(curr));
        }
    }
    return current_line;
}
void generate_asm_for_hanging_expression(StringBuilder* sb, AstNode* stm, FunctionContext* context ) {
    //PANIC("%s %d: TODO",__FILE__,__LINE__);
    if( stm == NULL ) {
        PANIC("%s %d: NULL AST_NODE",__FILE__,__LINE__);
    }

    switch(stm->type) {
        case AST_EXPRESSION_STATEMENT:
            generate_asm_for_hanging_expression(sb, stm->expression_statement.expression, context);
            break;
        case AST_BINARY_OPERATION:
            switch( stm->binary_operation.opp_token.kind ) {
                case ASSIGN:
                    generate_asm_for_expression(sb, stm, context, 0);
                    break;
                default:
                    generate_asm_for_hanging_expression(sb,stm->binary_operation.right,context);
                    generate_asm_for_hanging_expression(sb,stm->binary_operation.left,context);
                    break;
            }
            break;
        case AST_UNARY_OPERATION:
            generate_asm_for_hanging_expression(sb,stm->unary_operation.right,context);
            break;
        case AST_FUNC_CALL:
            generate_asm_for_function_call(sb,stm,context);
            break;

        case AST_IDENTIFIER:
        case AST_NUMBER:
        case AST_STRING:
            break;
        default:
            PANIC("%s %d: NOT SUPPORTED: %s",__FILE__,__LINE__,format_ast_type(stm));
    }
}

void generate_asm_for_function_call(StringBuilder* sb, AstNode* stm, FunctionContext* context) {
    // have to push the args in reverse order so the first argument has the smallest (closer to zero) negative ofset
    char* identifier = stm->function_call.identifier.value;

    FunctionArgumentsList arguments_list = {0};
    int used_stack_bytes = 0;

    AstNode* curr_arg = stm->function_call.args;
    while( curr_arg != NULL ) {
        ASSERT( (curr_arg->argument.value->type == AST_EXPRESSION_STATEMENT), "Argument to a funtion call isnt an expression statement");

        da_append(arguments_list,curr_arg);
        curr_arg = curr_arg->argument.next;
    }

    for( int i = arguments_list.count-1; i >= 0; i-- ){

        AstNode* curr_expr = arguments_list.items[i]->argument.value->expression_statement.expression;
        switch( curr_expr->type ) {
            case AST_BINARY_OPERATION:
            case AST_UNARY_OPERATION:
                Register target_register = take_next_available_register(&context->available_registers);
                if( target_register == 0 ) {
                    PANIC("%s %d: RUN OUT OF REGISTERS",__FILE__,__LINE__);
                }

                add_register(&context->touched_registers,target_register);
                // this puts the value of the expression in the target_register
                generate_asm_for_expression(sb, curr_expr, context, target_register);
                // push target_register
                sb_append(sb,"push %s\n",get_register_str(target_register));

                remove_register(&context->available_registers,target_register);
                break;
            case AST_FUNC_CALL:
                generate_asm_for_function_call(sb, curr_expr, context);
                sb_append(sb,"push r15\n");
                break;

            case AST_STRING:
                PANIC("%s %d: STRING NOT SUPPORTED",__FILE__,__LINE__);
            case AST_NUMBER:
                sb_append(sb,"push %s\n",curr_expr->number.value);
                break;
            case AST_IDENTIFIER:
                VariableLocation location = get_location_of_variable(context->variables_list,curr_expr->identifier.token.value);
                sb_append(sb,"push %s\n",get_location_str(location));
                break;
        }
    } 

    sb_append(sb,"call %s\n",identifier);
    //PANIC("%s %d: TODO: CLEAR THE PUSHED ARGS FROM THE STACK (sub rsp, {size of all args})",__FILE__,__LINE__);
    sb_append(sb,"sub rsp, %d\n",arguments_list.count*8);
}

Register generate_asm_for_expression(StringBuilder* sb, AstNode* stm, FunctionContext* context, Register target_register) {
    if( stm == NULL ) {
        PANIC("%s %d: NULL AST_NODE",__FILE__,__LINE__);
    }

    switch(stm->type) {
        case AST_BINARY_OPERATION: {
            // check which side is a value, call recursively on the side thats not, 
            // after that "apply" the value to the register that was passed to the value side
            // if both sides arent values then get a new register and pass it on the right side then 
            // "apply" the new register's value to the old one and "free" the new register
            // if no new register is available fallback to the stack
            bool right_is_value = is_value_ast(stm->binary_operation.right);
            bool left_is_value = is_value_ast(stm->binary_operation.left);
            AstNode* right = stm->binary_operation.right;
            AstNode* left = stm->binary_operation.left;

            TokenKind opp_kind = stm->binary_operation.opp_token.kind;
            char* opp;
            switch( opp_kind ) {
                case PLUS: 
                    opp = "add";
                    break;
                case MINUS:
                    opp = "sub";
                    break;
                case STAR:
                    opp = "imul";
                    break;
                case ASSIGN:
                    PANIC("%s %d: TODO",__FILE__,__LINE__);
                default:
                    PANIC("%s %d: OPERATION NOT SUPPORTED: %s",__FILE__,__LINE__,format_token_kind(stm->binary_operation.opp_token));
            }

            if(!right_is_value && !left_is_value) { // have to allocate a new register
                Register new_register = take_next_available_register(&context->available_registers);
                if( new_register == 0 ) {
                    PANIC("%s %d: RUN OUT OF REGISTERS",__FILE__,__LINE__);
                }
                add_register(&context->touched_registers,new_register);

                generate_asm_for_expression(sb,left,context, new_register);
                generate_asm_for_expression(sb,right,context, target_register);

                sb_append(sb,"%s %s, %s\n",opp,get_register_str(target_register),get_register_str(new_register));
                remove_register(&context->available_registers,new_register);

            } else if ( right_is_value && left_is_value ) { // bottom of the tree
                switch(left->type) {
                    case AST_FUNC_CALL:
                        generate_asm_for_function_call(sb,left,context);
                        sb_append(sb,"mov %s, r15\n",get_register_str(target_register));
                        break;
                    case AST_IDENTIFIER:
                        VariableLocation location = get_location_of_variable(context->variables_list,left->identifier.token.value);
                        sb_append(sb,"mov %s, %s\n",get_register_str(target_register),get_location_str(location));
                        break;
                    case AST_NUMBER:
                        sb_append(sb,"mov %s, %s\n",get_register_str(target_register),left->number.value);
                        break;
                    case AST_STRING:
                        PANIC("%s %d: STRING NOT SUPPORTED",__FILE__,__LINE__);
                }

                switch(right->type) {
                    case AST_FUNC_CALL:
                        generate_asm_for_function_call(sb,right,context);
                        sb_append(sb,"%s %s, r15\n",opp, get_register_str(target_register));
                        break;
                    case AST_IDENTIFIER:
                        VariableLocation location = get_location_of_variable(context->variables_list,right->identifier.token.value);
                        sb_append(sb,"%s %s, %s\n",opp, get_register_str(target_register),get_location_str(location));
                        break;
                    case AST_NUMBER:
                        sb_append(sb,"%s %s, %s\n",opp, get_register_str(target_register),right->number.value);
                        break;
                    case AST_STRING:
                        PANIC("%s %d: STRING NOT SUPPORTED",__FILE__,__LINE__);
                }
            } else if ( right_is_value ) { 
                generate_asm_for_expression(sb,left,context, target_register);

                switch(right->type) {
                    case AST_FUNC_CALL:
                        generate_asm_for_function_call(sb,right,context);
                        sb_append(sb,"%s %s, r15\n",opp,get_register_str(target_register));
                        break;
                    case AST_IDENTIFIER:
                        VariableLocation location = get_location_of_variable(context->variables_list,right->identifier.token.value);
                        sb_append(sb,"%s %s, %s\n",opp,get_register_str(target_register),get_location_str(location));
                        break;
                    case AST_NUMBER:
                        sb_append(sb,"%s %s, %s\n",opp,get_register_str(target_register),right->number.value);
                        break;
                    case AST_STRING:
                        PANIC("%s %d: STRING NOT SUPPORTED",__FILE__,__LINE__);
                }

            } else { // left is value
                generate_asm_for_expression(sb,right,context, target_register);

                switch(left->type) {
                    case AST_FUNC_CALL:
                        generate_asm_for_function_call(sb,left,context);
                        sb_append(sb,"%s %s, r15\n",opp,get_register_str(target_register));
                        break;
                    case AST_IDENTIFIER:
                        VariableLocation location = get_location_of_variable(context->variables_list,left->identifier.token.value);
                        sb_append(sb,"%s %s, %s\n",opp,get_register_str(target_register),get_location_str(location));
                        break;
                    case AST_NUMBER:
                        sb_append(sb,"%s %s, %s\n",opp,get_register_str(target_register),left->number.value);
                        break;
                    case AST_STRING:
                        PANIC("%s %d: STRING NOT SUPPORTED",__FILE__,__LINE__);
                }

            }

        }   break;
        case AST_UNARY_OPERATION: {
            PANIC("%s %d: TODO",__FILE__,__LINE__);

            /*
            bool right_is_value = is_value_ast(stm->unary_operation.right);
            AstNode* right = stm->unary_operation.right;

            TokenKind opp_kind = stm->unary_operation.opp_token.kind;
            //char* opp;

            if( !right_is_value ) {
                Register target_register = take_next_available_register(&context->available_registers);
                if( target_register == 0 ) { PANIC("%s %d: RUN OUT OF REGISTERS",__FILE__,__LINE__); }

                add_register(&context->touched_registers,target_register);
                // this puts the value of the expression in the target_register
                generate_asm_for_expression(sb, curr_expr, context, target_register);


                remove_register(&context->available_registers,target_register);
                break;
            } else {

            }
            */

        }   break;
        // Values 
        case AST_STRING:
            PANIC("%s %d: STRING NOT SUPPORTED",__FILE__,__LINE__);
        case AST_NUMBER:
            // mov target_register, NUMBER
            sb_append(sb,"mov %s, %s\n",get_register_str(target_register),stm->number.value);
            break;
        case AST_IDENTIFIER:
            // Location loc = get_ident_location(IDENTIFIER);
            // mov target_register, loc
            VariableLocation location = get_location_of_variable(context->variables_list,stm->identifier.token.value);
            sb_append(sb,"mov %s, %s\n",get_register_str(target_register),get_location_str(location));
            break;
        case AST_FUNC_CALL:
            // generate_asm_for_function_call(...);
            // mov target_register, r15
            generate_asm_for_function_call(sb,stm,context);
            sb_append(sb,"mov %s, r15\n",get_register_str(target_register));
            //PANIC("%s %d: GENERATE_ASM_FOR_EXPR USE ONLY WHEN ITS NOT A VALUE (UNARY/BINARY NODE): %s",__FILE__,__LINE__,format_ast_type(stm));
            break;
        default:
            PANIC("%s %d: NOT SUPPORTED: %s",__FILE__,__LINE__,format_ast_type(stm));
    }
}

void free_registers_from_later_unused_variables(FunctionContext* context) {
    for(int idx = 0; idx < context->variables_list.count; idx++) {
        VariableInfo* variable = &context->variables_list.items[idx];

        if( variable->location.type != NOT_ASSIGNED && context->current_line > variable->last_line_used ) {
            if( variable->location.type == REGISTER ) {
                printf("FREEING REGISTER %s USED FOR VARIABLE %s on line %d\n",get_register_str(variable->location.register_location.register_),variable->identifier,context->current_line);
                add_register(&context->available_registers,variable->location.register_location.register_);
            } else {
                printf("FREEING VARIABLE %s with stack offset %d on line %d\n",variable->identifier,variable->location.stack_location.base_offset,context->current_line);
            }
            variable->location.type = NOT_ASSIGNED;
        }
    }
}

const char* generate_asm_for_statements(StringBuilder* sb, AstNode* next,FunctionContext* context) {
    while( next != NULL ) {
        AstNode* curr = next;

        free_registers_from_later_unused_variables(context);

        switch(curr->type) {
            case AST_BLOCK_STATEMENT:
                generate_asm_for_statements(sb,curr->block_statement.statements,context);
                next = curr->block_statement.next;
                break;
            case AST_DECLARATION:
                //printf("DECLARING %s on line %d\n",curr->declaration.name,context->current_line);

                Register target_register = take_next_available_register(&context->available_registers);
                if( target_register == 0 ) {
                    PANIC("%s %d: RUN OUT OF REGISTERS",__FILE__,__LINE__);
                }
                add_register(&context->touched_registers,target_register);

                generate_asm_for_expression(sb,
                    curr->declaration.value->expression_statement.expression,
                    context,
                    target_register
                );

                // ASSIGN THE target_register AS THE LOCATION FOR VARIABLE THATS BEEN DECLARED
                set_location_for_variable(
                    context->variables_list,
                    curr->declaration.name,
                    (VariableLocation){ .type=REGISTER, .register_location.register_ = target_register }
                );

                context->current_line++;
                next = curr->declaration.next;
                break;
            case AST_EXPRESSION_STATEMENT:
                //printf("EXPRESSION on line %d\n",context->current_line);
                generate_asm_for_hanging_expression(sb,curr->expression_statement.expression,context);
                context->current_line++;
                next = next->expression_statement.next;
                break;
            case AST_RETURN_STATEMENT:
                //PANIC("%s %d: TODO",__FILE__,__LINE__);
                
                generate_asm_for_expression(sb,
                    curr->return_statement.expression->expression_statement.expression,
                    context,
                    R15
                );

                context->current_line++;
                next = next->return_statement.next;
                break;
            case AST_IF_STATEMENT:
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                next = next->if_statement.next;
                break;
            case AST_STRUCT_DECLARATION:    
                PANIC("%s %d: UNREACHABLE",__FILE__,__LINE__);
                next = next->struct_declaration.next;
                break;
            case AST_FOR_STATEMENT:
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                next = next->for_statement.next;
                break;
            case AST_WHILE_STATEMENT:
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                next = next->while_statement.next;
                break;
            default:
                PANIC("%s %d: NOT SUPPORTED: %s",__FILE__,__LINE__,format_ast_type(curr));
        }
    }
}

#define MAX_SCOPE_NUMBERS 1000
#define MAX_SEEN_VARIABLES 1000
#define MAX_SEEN_VARIABLES_FRAMES 1000

typedef struct {
    int curr_scope_number;
    int numbers[1000];
    int numbers_pointer;
} ScopeNumbersStack;

typedef struct {
    int seen_variables_frames[1000];
    int seen_variables_frames_pointer;

    char* seen_variables[1000];
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
            PANIC("%s %d: NOT SUPPORTED: %s",__FILE__,__LINE__,format_ast_type(stm));
    }
}

void mangle_names_inner(AstNode* stm, NameManglingContext* stack);
void mangle_names(AstNode* stm) {
    ASSERT(stm->type == AST_BLOCK_STATEMENT, "%s %d: PANICKED",__FILE__,__LINE__);

    NameManglingContext stack = {0}; 
    mangle_names_inner(stm,&stack);
}

void mangle_names_inner(AstNode* stm, NameManglingContext* context) {
    AstNode* next = stm;
    while( next != NULL ) {
        AstNode* curr = next;
        switch(curr->type) {
            case AST_BLOCK_STATEMENT:
                NameManglingContext_new_scope(context);
                mangle_names_inner(curr->block_statement.statements, context);
                NameManglingContext_pop_scope(context);
                next = curr->block_statement.next;
                break;
            case AST_DECLARATION:
                mangle_expression(context,curr->declaration.value);
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
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                next = next->if_statement.next;
                break;
            case AST_FOR_STATEMENT:
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                next = next->for_statement.next;
                break;
            case AST_WHILE_STATEMENT:
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                next = next->while_statement.next;
                break;
            default:
                PANIC("%s %d: NOT SUPPORTED: %s",__FILE__,__LINE__,format_ast_type(curr));
        }
    }
}

#include "print_ast.h"
const char* generate_asm_for_function(AstNode* stm){
    StringBuilder sb = sb_new();
    VariableInfoList variables_list = {0};

    sb_append(&sb,stm->function_declaration.name);
    sb_append(&sb,":\n");

    // Stack frame setup
    sb_append(&sb,"push rbp\n");
    sb_append(&sb,"mov rbp, rsp\n");

    AstNode* arg_decl = stm->function_declaration.args;
    int curr_arg_stack_offset = 16;
    while(arg_decl != NULL) {
        //int arg_size = Type_size_of(arg_decl->argument_decl.type);
        int arg_size = 8;
        VariableInfo info = {.identifier = arg_decl->argument_decl.ident, 
                            .first_line_used = 0, 
                            .last_line_used = 0 , 
                            .location = (VariableLocation){ .type=STACK, .stack_location.base_offset = curr_arg_stack_offset}};
        da_append(variables_list,info);

        curr_arg_stack_offset += arg_size;

        arg_decl = arg_decl->argument_decl.next;
    }
    int curr_line = 1;

    print_program_ast(stm);

    printf("================================== MANGLING =======================================\n"); 
    mangle_names(stm->function_declaration.body);

    print_program_ast(stm);

    analyze_variable_lifetimes(&variables_list,stm->function_declaration.body,1);

    /*
    for(int i = 0; i<variables_list.count; i++) {
        char* identifier = variables_list.items[i].identifier;
        int first_line_used = variables_list.items[i].first_line_used;
        int last_line_used = variables_list.items[i].last_line_used;
        switch(variables_list.items[i].location.type) {
            case REGISTER:
                printf("ident: %s first_line_used: %d last_line_used: %d, location: type: REGISTER register_: %s",
                       identifier,first_line_used,last_line_used,get_register_str(variables_list.items[i].location.register_location.register_));
                break;
            case STACK:
                printf("ident: %s first_line_used: %d last_line_used: %d, location: type: STACK base_offset: %d",
                       identifier,first_line_used,last_line_used, variables_list.items[i].location.stack_location.base_offset);
                break;
            case NOT_ASSIGNED:
                printf("ident: %s first_line_used: %d last_line_used: %d, location: type: NOT_ASSIGNED",identifier,first_line_used,last_line_used);
                break;
        }
        printf("\n");
    }
    /*
    */

    StringBuilder statements_asm_sb = sb_new();
    FunctionContext context = { .touched_registers = {0}, .available_registers = {~0 - R15 - RSP - RBP}, .variables_list = variables_list, .current_line = 1 };

    generate_asm_for_statements(&statements_asm_sb,stm->function_declaration.body,&context);

    //PANIC("%s %d: TODO: SAVE REGISTERS BEFORE USING THEM AND RESTORE THEM AT THE END",__FILE__,__LINE__);
    for(int i = 0; i <= 15; i++) {
        Register reg = (Register)(1 << i);
        if( (context.touched_registers & reg) != 0) {
            sb_append(&sb,"push %s\n",get_register_str(reg));
        } 
    }

    sb_append(&sb,statements_asm_sb.buffer);

    for(int i = 15; i >= 0; i--) {
        Register reg = (Register)(1 << i);
        if( (context.touched_registers & reg) != 0) {
            sb_append(&sb,"pop %s\n",get_register_str(reg));
        } 
    }

    // Stack frame cleanup; same as leave
    sb_append(&sb,"mov rsp, rbp\n");
    sb_append(&sb,"pop rbp \n");
    // return
    sb_append(&sb,"ret \n");
    
    //printf("FUNCTION ASM:\n%s",sb.buffer);
    return sb.buffer;
}

void generate_asm_for_extern_statements(StringBuilder* sb, AstNode* stm) {
    AstNode* next = stm;
    while( next != NULL ) {
        switch( next->type ) {
            case AST_BLOCK_STATEMENT:
                next = next->block_statement.statements;
                break;
            case AST_FUNCTION_DECLARATION:
                sb_append(sb, "extern %s\n", next->function_declaration.name);
                next = next->function_declaration.next;
                break;
            default:
                PANIC("%s %d: SHOULD NOT BE POSSIBLE %s", __FILE__,__LINE__, format_ast_type(next));
        }
    }
}
 
typedef struct {
    CStringList functions_asm_list;
    StringBuilder extern_asm;
    CStringList global_declarations_asm_list;
} TopLevelStatemetnsAsm;

TopLevelStatemetnsAsm generate_asm_for_top_level_statements(AstNode* stm) {
    TopLevelStatemetnsAsm list = {0};
    list.extern_asm = sb_new();

    AstNode* next = stm;
    while( next != NULL ) {
        const char* asm_str;
        switch( next->type ) { /* ALL POSSIBLE TOP LEVEL STATEMENTS */
            case AST_FUNCTION_DECLARATION:
                asm_str = generate_asm_for_function(next); 
                da_append(list.functions_asm_list,asm_str);

                next = next->function_declaration.next;
                break;
            case AST_DECLARATION: /* GLOBAL DECL */
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                //generate_decl(sb,next); 
                next = next->declaration.next;
                break;
            case AST_STRUCT_DECLARATION:    
                PANIC("%s %d: TODO",__FILE__,__LINE__);
                //generate_struct_decl(sb,next);
                next = next->struct_declaration.next;
                break;
            case AST_EXTERN_STATEMENT:    
                generate_asm_for_extern_statements(&list.extern_asm, next->extern_statement.body);
                next = next->extern_statement.next;
                break;
            default:
                PANIC("NOT SUPPORTED: %s",format_ast_type(next));
        }
    }
    return list;
}

char* generate_asm(AstNode* node) {
    StringBuilder output_sb = sb_new();
    /*
    const char *gnu_header = 
        ".intel_syntax noprefix\n"
        ".text\n"

        ".globl _start\n"
        "_start:\n"
        // # Call main function
        "call main\n"

        //# Exit the program
        "mov eax, 60\n"//     # syscall: exit
        "xor edi, edi\n"//    # status: 0
        "syscall\n"
        "# ===================== end of HEADER =================================\n"
    ;
    */

    const char *nasm_header = 
        "section .text\n"
            "global _start\n"

        "_start:\n"
        // # Call main function
        "call main\n"

        //# Exit the program
        "mov eax, 60\n"//     # syscall: exit
        "xor edi, edi\n"//    # status: 0
        "syscall\n"

        "; ===================== end of HEADER =================================\n"
    ;


    sb_append(&output_sb,nasm_header);
    TopLevelStatemetnsAsm top_level_defs = generate_asm_for_top_level_statements(node);
    CStringList function_defs = top_level_defs.functions_asm_list;

    sb_append(&output_sb,top_level_defs.extern_asm.buffer);
    sb_append(&output_sb,"; ===================== end of EXTERN =================================\n");

    for(int i = 0; i < function_defs.count; i++) {
        sb_append(&output_sb,function_defs.items[i]);
    }

    return output_sb.buffer;
}
