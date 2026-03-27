#include "backend.h"
#include "lexer.h"
#include "dyn_arrays_macro.h"

#include "backend_ops_impl.c"


#define ASSERT(expr, fmt, ...) { \
    if (!expr) { \
        printf("%s %d: " fmt "\n",__FILE__, __LINE__,  ##__VA_ARGS__); \
        exit(-1); \
    } \
}
#define PANIC(fmt, ...) { \
    printf("%s %d: " "\033[1;31m" fmt "\033[0m" "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    *(int*)0=0;\
    exit(-1); \
}

ProgramContext PROGRAM_CONTEXT = { .curr_label_number = 0 };

char* get_location_str(VariableLocation location) {
    static char buffer[256];
    switch( location.type ) {
        case NOT_ASSIGNED:
            PANIC("PANICKED");
        case REGISTER:
            return get_register_str(location.register_location.register_); 
        case STACK:
            snprintf(buffer, sizeof(buffer), "qword [rbp%+d]", location.stack_location.base_offset);
            return buffer;
    }
}

const VariableLocation get_location_of_variable(const VariableInfoList variables_list, const char* variable_identifier) {
    for(int idx = 0; idx < variables_list.count; idx++) {
        if( strcmp(variable_identifier,variables_list.items[idx].identifier) == 0 ) {
            return variables_list.items[idx].location;
        }
    }
    PANIC("PANICKED");
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
            return true;
        case AST_EXPRESSION_STATEMENT:
            return is_value_ast(stm->expression_statement.expression);
        case AST_STRING:
            PANIC("TODO");
        default:
            return false;
            
    }
}
/*
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
            PANIC("PANICKED");
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
            PANIC("NOT SUPPORTED: %s",format_ast_type(stm));
  }
}
*/
int analyze_variable_stack_usage(VariableInfoList* variables_list, AstNode* next, int curr_frame_ptr) {
    while( next != NULL ) {
        AstNode* curr = next;
        switch(curr->type) {
            case AST_BLOCK_STATEMENT:
                curr_frame_ptr = analyze_variable_stack_usage(variables_list,curr->block_statement.statements, curr_frame_ptr);
                next = curr->block_statement.next;
                break;
            case AST_DECLARATION:
                curr_frame_ptr -= 8; // add 8 bytes TODO use arg_size
                
                char* name = curr->declaration.name;
                Type* type = curr->declaration.type;

                int arg_size = Type_size_of(type);

                VariableInfo info = {.identifier = name, 
                                     //.first_line_used = -1, 
                                     //.last_line_used = -1, 
                                     .location = (VariableLocation){ .type = STACK , .stack_location.base_offset = curr_frame_ptr }
                                    };
                da_append_ref(variables_list,info);

                next = curr->declaration.next;
                break;
            case AST_EXPRESSION_STATEMENT:
                next = next->expression_statement.next;
                break;
            case AST_RETURN_STATEMENT:
                next = next->return_statement.next;
                break;
            case AST_IF_STATEMENT:
                curr_frame_ptr = analyze_variable_stack_usage(variables_list,curr->if_statement.body, curr_frame_ptr);
                curr_frame_ptr = analyze_variable_stack_usage(variables_list,curr->if_statement.else_block, curr_frame_ptr);

                next = next->if_statement.next;
                break;
            case AST_WHILE_STATEMENT:
                curr_frame_ptr = analyze_variable_stack_usage(variables_list,curr->while_statement.body, curr_frame_ptr);
                next = next->while_statement.next;
                break;
            case AST_FOR_STATEMENT:
                PANIC("TODO");
                break;
            default:
                PANIC("NOT SUPPORTED: %s",format_ast_type(curr));
        }
    }
    return curr_frame_ptr;
}

/*
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
                PANIC("TODO");
                next = next->if_statement.next;
                break;
            case AST_FOR_STATEMENT:
                PANIC("TODO");
                next = next->for_statement.next;
                break;
            case AST_WHILE_STATEMENT:
                PANIC("TODO");
                next = next->while_statement.next;
                break;
            //AST_BINARY_OPERATION,   
            //AST_UNARY_OPERATION,    
            //AST_FUNC_CALL,          
            default:
                PANIC("NOT SUPPORTED: %s",format_ast_type(curr));
        }
    }
    return current_line;
}
*/

void generate_asm_for_hanging_expression(StringBuilder* sb, AstNode* stm, FunctionContext* context ) {
    //PANIC("TODO",__FILE__,__LINE__);
    if( stm == NULL ) {
        PANIC("NULL AST_NODE");
    }

    switch(stm->type) {
        case AST_EXPRESSION_STATEMENT:
            generate_asm_for_hanging_expression(sb, stm->expression_statement.expression, context);
            break;
        case AST_BINARY_OPERATION:
            switch( stm->binary_operation.opp_token.kind ) {
                case ASSIGN:
                    Register new_register = take_next_available_register(&context->available_registers);
                    if( new_register == 0 ) {
                        PANIC("RUN OUT OF REGISTERS");
                    }
                    add_register(&context->touched_registers,new_register);

                    generate_asm_for_expression(sb, stm, context, new_register);

                    add_register(&context->available_registers,new_register);
                    break;

                case EQUAL:
                case NOT_EQUAL:
                case LESS_THEN:
                case MORE_THEN:
                case LESS_EQUAL:
                case MORE_EQUAL:

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
            PANIC("NOT SUPPORTED: %s",format_ast_type(stm));
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
                    PANIC("RUN OUT OF REGISTERS");
                }

                add_register(&context->touched_registers,target_register);
                // this puts the value of the expression in the target_register
                generate_asm_for_expression(sb, curr_expr, context, target_register);
                // push target_register
                sb_append(sb,"\tpush %s\n",get_register_str(target_register));

                add_register(&context->available_registers,target_register);
                break;
            case AST_FUNC_CALL:
                generate_asm_for_function_call(sb, curr_expr, context);
                sb_append(sb,"\tpush rax\n");
                break;

            case AST_STRING:
                PANIC("STRING NOT SUPPORTED");
            case AST_NUMBER:
                sb_append(sb,"\tpush %s\n",curr_expr->number.value);
                break;
            case AST_IDENTIFIER:
                VariableLocation location = get_location_of_variable(context->variables_list,curr_expr->identifier.token.value);
                sb_append(sb,"\tpush %s\n",get_location_str(location));
                break;
        }
    } 

    sb_append(sb,"\tcall %s\n",identifier);
    //clear the pushed args from the stack
    sb_append(sb,"\tsub rsp, %d\n",arguments_list.count*8);
}

char* handle_value(StringBuilder* sb, AstNode* stm, FunctionContext* context) {
    switch(stm->type) {
        case AST_FUNC_CALL:
            generate_asm_for_function_call(sb,stm,context);
            return "rax";
        case AST_IDENTIFIER:
            VariableLocation location = get_location_of_variable(context->variables_list,stm->identifier.token.value);
            return get_location_str(location);
        case AST_NUMBER:
            return stm->number.value;
        case AST_STRING:
            PANIC("STRING NOT SUPPORTED");
        default:
            PANIC("PANICKED");
    }
}
void generate_asm_for_while_statement(StringBuilder* sb, AstNode* stm, FunctionContext* context) {
    bool is_value = is_value_ast(stm->while_statement.condition);
    AstNode* condition = stm->while_statement.condition->expression_statement.expression;
    TokenKind opp_kind;

    int condition_check_lebel_number = PROGRAM_CONTEXT.curr_label_number++;
    int while_body_start_lebel_number = PROGRAM_CONTEXT.curr_label_number++;

    sb_append(sb,"\tjmp L%i\n",condition_check_lebel_number);
    sb_append(sb,"L%i:\n",while_body_start_lebel_number);
    generate_asm_for_statements(sb,stm->while_statement.body,context);
    
    sb_append(sb,"L%i:\n",condition_check_lebel_number);
    if( is_value ) {
        sb_append(sb,"\tcmp %s, 0\n", handle_value(sb,condition,context));
        sb_append(sb,"\tjne L%i\n", while_body_start_lebel_number);

    } else {
        if( condition->type == AST_BINARY_OPERATION ) {
            opp_kind = condition->binary_operation.opp_token.kind;
            handle_boolian_binary_cmp(sb,condition,context);
        } else { // AST_UNARY_OPERATION 
            opp_kind = condition->unary_operation.opp_token.kind;
            PANIC("TODO");
        }
        switch(opp_kind) {

            case EQUAL:         sb_append(sb,"\tje   L%i\n", while_body_start_lebel_number); break;
            case NOT_EQUAL:     sb_append(sb,"\tjne  L%i\n", while_body_start_lebel_number); break;
            case LESS_THEN:     sb_append(sb,"\tjl   L%i\n", while_body_start_lebel_number); break;
            case MORE_THEN:     sb_append(sb,"\tjg   L%i\n", while_body_start_lebel_number); break;
            case LESS_EQUAL:    sb_append(sb,"\tjle  L%i\n", while_body_start_lebel_number); break;
            case MORE_EQUAL:    sb_append(sb,"\tjge  L%i\n", while_body_start_lebel_number); break;
            case NOT:
                PANIC("TODO");
            default:
                PANIC("PANICKED");
        }
    }
}

void generate_asm_for_if_statement(StringBuilder* sb, AstNode* stm, FunctionContext* context) {
    bool is_value = is_value_ast(stm->if_statement.condition);
    AstNode* condition = stm->if_statement.condition->expression_statement.expression;
    TokenKind opp_kind;

    int end_of_body_label_number = PROGRAM_CONTEXT.curr_label_number++;
    
    if( is_value ) {
        sb_append(sb,"\tcmp %s, 0\n", handle_value(sb,condition,context));
        sb_append(sb,"\tje L%i\n",  end_of_body_label_number);
    } else {
        if( condition->type == AST_BINARY_OPERATION ) {
            opp_kind = condition->binary_operation.opp_token.kind;
            handle_boolian_binary_cmp(sb,condition,context);
        } else { // AST_UNARY_OPERATION 
            opp_kind = condition->unary_operation.opp_token.kind;
            PANIC("TODO");
        }
        switch(opp_kind) {
            case EQUAL:         sb_append(sb,"\tjne L%i\n", end_of_body_label_number); break;
            case NOT_EQUAL:     sb_append(sb,"\tje  L%i\n", end_of_body_label_number); break;
            case LESS_THEN:     sb_append(sb,"\tjge L%i\n", end_of_body_label_number); break;
            case MORE_THEN:     sb_append(sb,"\tjle L%i\n", end_of_body_label_number); break;
            case LESS_EQUAL:    sb_append(sb,"\tjg  L%i\n", end_of_body_label_number); break;
            case MORE_EQUAL:    sb_append(sb,"\tjl  L%i\n", end_of_body_label_number); break;
            case NOT:
                PANIC("TODO");
            default:
                PANIC("PANICKED");
        }
    }

    generate_asm_for_statements(sb,stm->if_statement.body,context);

    // Jump over the else block if it exists
    int end_of_else_block_label_number;
    if( stm->if_statement.else_block != NULL ) {
        end_of_else_block_label_number = PROGRAM_CONTEXT.curr_label_number++;
        sb_append(sb,"\tjmp L%i\n",end_of_else_block_label_number);
    }

    sb_append(sb,"L%i:\n",end_of_body_label_number);

    if( stm->if_statement.else_block != NULL ) {
        generate_asm_for_statements(sb,stm->if_statement.else_block,context);
        sb_append(sb,"L%i:\n",end_of_else_block_label_number);
    }
}

void generate_asm_for_expression(StringBuilder* sb, AstNode* stm, FunctionContext* context, Register target_register) {
    if( stm == NULL ) {
        PANIC("NULL AST_NODE");
    }

    switch(stm->type) {
        case AST_BINARY_OPERATION: {
            TokenKind opp_kind = stm->binary_operation.opp_token.kind;
            switch(opp_kind) {
                case PLUS: 
                case MINUS:
                case STAR:
                    handle_simple_ops(sb,stm,context,target_register);
                    break;
                case ASSIGN:
                    handle_assing_op(sb,stm,context,target_register);
                    break;
                case DIVITION:
                    handle_divition_op(sb,stm,context,target_register);
                    break;
                case EQUAL:     
                case NOT_EQUAL: 
                case LESS_THEN: 
                case MORE_THEN: 
                case LESS_EQUAL:
                case MORE_EQUAL:
                    handle_boolian_binary_ops(sb,stm,context,target_register);
                    break;
                    
                default:
                    PANIC("NOT SUPPORTED: %s",format_token(stm->binary_operation.opp_token));
            }
            break; 
        }
        case AST_UNARY_OPERATION: {
            TokenKind opp_kind = stm->binary_operation.opp_token.kind;
            switch(opp_kind) {
                case STAR:
                    handle_dereferance_op(sb,stm,context,target_register);
                    break;
                case AMPERSAND:
                    handle_get_address_op(sb,stm,context,target_register);
                    break;
                default:
                    PANIC("NOT SUPPORTED: %s",format_token(stm->unary_operation.opp_token));
            }
        }   break;

        case AST_NUMBER:
        case AST_IDENTIFIER:
        case AST_FUNC_CALL:
        case AST_STRING:
            sb_append(sb,"\tmov %s, %s\n",get_register_str(target_register), handle_value(sb,stm,context));
            break;
        default:
            PANIC("NOT SUPPORTED: %s",format_ast_type(stm));
    }
}

/*
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
*/

void generate_asm_for_statements(StringBuilder* sb, AstNode* next,FunctionContext* context) {
    while( next != NULL ) {
        AstNode* curr = next;

        //free_registers_from_later_unused_variables(context);

        switch(curr->type) {
            case AST_BLOCK_STATEMENT:
                generate_asm_for_statements(sb,curr->block_statement.statements,context);
                next = curr->block_statement.next;
                break;
            case AST_DECLARATION:
                //printf("DECLARING %s on line %d\n",curr->declaration.name,context->current_line);
                // TODO: REMOVE UNNASSASARY MOVES TO REGISTERS FOR DIRECT VALUES (VALUES NOT IN MEMORY so REGISTERS, NUMBERS)

                Register target_register = take_next_available_register(&context->available_registers);
                if( target_register == 0 ) {
                    PANIC("RUN OUT OF REGISTERS");
                }
                add_register(&context->touched_registers,target_register);

                generate_asm_for_expression(sb,
                    curr->declaration.value->expression_statement.expression,
                    context,
                    target_register
                );

                //TODO("FIND VARIABLE LOCATION AND MOVE TARGET REGISTER TO THE LOCATION");
                VariableLocation location = get_location_of_variable(context->variables_list, curr->declaration.name);

                sb_append(sb,"\tmov %s, %s\n", get_location_str(location), get_register_str(target_register));


                add_register(&context->available_registers,target_register);

                //context->current_line++;
                next = curr->declaration.next;
                break;
            case AST_EXPRESSION_STATEMENT:
                //printf("EXPRESSION on line %d\n",context->current_line);
                generate_asm_for_hanging_expression(sb,curr->expression_statement.expression,context);
                //context->current_line++;
                next = next->expression_statement.next;
                break;
            case AST_RETURN_STATEMENT:
                generate_asm_for_expression(sb,
                    curr->return_statement.expression->expression_statement.expression,
                    context,
                    RAX
                );

                sb_append(sb,"\tjmp L%i\n",context->return_label_number);
                next = next->return_statement.next;
                break;
            case AST_IF_STATEMENT:
                generate_asm_for_if_statement(sb,curr,context);
                next = next->if_statement.next;
                break;
            case AST_WHILE_STATEMENT:
                generate_asm_for_while_statement(sb,curr,context);
                next = next->while_statement.next;
                break;
            case AST_STRUCT_DECLARATION:    
                PANIC("UNREACHABLE");
                next = next->struct_declaration.next;
                break;
            case AST_FOR_STATEMENT:
                PANIC("TODO");
                next = next->for_statement.next;
                break;
            default:
                PANIC("NOT SUPPORTED: %s",format_ast_type(curr));
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

#include "print_ast.h"
const char* generate_asm_for_function(AstNode* stm){
    StringBuilder sb = sb_new();
    VariableInfoList variables_list = {0};

    sb_append(&sb,stm->function_declaration.name);
    sb_append(&sb,":\n");

    // Stack frame setup
    sb_append(&sb,"\tpush rbp\n");
    sb_append(&sb,"\tmov rbp, rsp\n");

    print_program_ast(stm);
    printf("================================== MANGLING =======================================\n"); 
    mangle_names(stm);
    print_program_ast(stm);

    AstNode* arg_decl = stm->function_declaration.args;
    int curr_arg_stack_offset = 16;
    while(arg_decl != NULL) {
        //int arg_size = Type_size_of(arg_decl->argument_decl.type);
        int arg_size = 8;
        VariableInfo info = {.identifier = arg_decl->argument_decl.ident, 
                            //.first_line_used = 0, 
                            //.last_line_used = 0 , 
                            .location = (VariableLocation){ .type=STACK, .stack_location.base_offset = curr_arg_stack_offset}};
        da_append(variables_list,info);

        curr_arg_stack_offset += arg_size;

        arg_decl = arg_decl->argument_decl.next;
    }
    int curr_line = 1;



    //analyze_variable_lifetimes(&variables_list,stm->function_declaration.body,1);
    analyze_variable_stack_usage(&variables_list,stm->function_declaration.body,0);

    StringBuilder statements_asm_sb = sb_new();
    FunctionContext context = { 
        .touched_registers = {0}, 
        .available_registers = { R8 + R9 + R10 + R11 + R12 + R13 + R14 + R15}, 
        .variables_list = variables_list,
        .return_label_number = PROGRAM_CONTEXT.curr_label_number++,
    };

    generate_asm_for_statements(&statements_asm_sb,stm->function_declaration.body,&context);

    for(int i = 0; i <= 15; i++) {
        Register reg = (Register)(1 << i);
        if( (context.touched_registers & reg) != 0) {
            sb_append(&sb,"\tpush %s\n",get_register_str(reg));
        } 
    }

    sb_append(&sb,statements_asm_sb.buffer);


    // function cleanup

    sb_append(&sb,"L%i:\n",context.return_label_number);
    for(int i = 15; i >= 0; i--) {
        Register reg = (Register)(1 << i);
        if( (context.touched_registers & reg) != 0) {
            sb_append(&sb,"\tpop %s\n",get_register_str(reg));
        } 
    }

    // Stack frame cleanup; same as leave
    sb_append(&sb,"\tmov rsp, rbp\n");
    sb_append(&sb,"\tpop rbp \n");
    // return
    sb_append(&sb,"\tret \n");
    
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
                PANIC("SHOULD NOT BE POSSIBLE %s", format_ast_type(next));
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
                PANIC("TODO");
                //generate_decl(sb,next); 
                next = next->declaration.next;
                break;
            case AST_STRUCT_DECLARATION:    
                PANIC("TODO");
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
