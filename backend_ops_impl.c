#include "backend.h"
#include "lexer.h"
#include "dyn_arrays_macro.h"

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
void handle_int_divition_op(StringBuilder* sb, AstNode* stm, FunctionContext* context, Register target_register) {
    AstNode* right = stm->binary_operation.right;
    AstNode* left = stm->binary_operation.left;

    bool right_is_value = is_value_ast(right);
    bool left_is_value = is_value_ast(left);

    TokenKind opp_kind = stm->binary_operation.opp_token.kind;
    ASSERT((opp_kind == DIVITION), "Expected DIVITION");

    if(!right_is_value && !left_is_value) { // have to allocate a new register
        Register new_register = take_next_available_register(&context->available_registers);
        if( new_register == 0 ) {
            PANIC("RUN OUT OF REGISTERS");
        }
        add_register(&context->touched_registers,new_register);

        generate_asm_for_expression(sb,left,context, new_register);
        generate_asm_for_expression(sb,right,context, target_register);

        sb_append(sb,"\tmov rax, %s\n", get_register_str(new_register));
        sb_append(sb,"\tcqo\n");
        sb_append(sb,"\tidiv %s\n",get_register_str(target_register));
        sb_append(sb,"\tmov %s, rax\n",get_register_str(target_register));

        add_register(&context->available_registers,new_register);
    } else if ( right_is_value && left_is_value ) { // bottom of the tree

        sb_append(sb,"\tmov rax, %s\n", handle_value(sb,left,context));
        sb_append(sb,"\tcqo\n");
        if(right->type == AST_NUMBER) {
            Register new_register = take_next_available_register(&context->available_registers);
            if( new_register == 0 ) { PANIC("RUN OUT OF REGISTERS"); }

            sb_append(sb,"\tmov %s, %s\n",get_register_str(new_register),handle_value(sb,right,context));
            sb_append(sb,"\tidiv %s\n",get_register_str(new_register));

            add_register(&context->available_registers,new_register);
        } else {
            sb_append(sb,"\tidiv %s\n",handle_value(sb,right,context));
        }
        sb_append(sb,"\tmov %s, rax\n",get_register_str(target_register));

    } else if ( right_is_value ) { 
        generate_asm_for_expression(sb,left,context, target_register);

        sb_append(sb,"\tmov rax, %s\n", get_register_str(target_register));
        sb_append(sb,"\tcqo\n");

        if(right->type == AST_NUMBER) {
            Register new_register = take_next_available_register(&context->available_registers);
            if( new_register == 0 ) { PANIC("RUN OUT OF REGISTERS"); }

            sb_append(sb,"\tmov %s, %s\n",get_register_str(new_register),handle_value(sb,right,context));
            sb_append(sb,"\tidiv %s\n",get_register_str(new_register));

            add_register(&context->available_registers,new_register);
        } else {
            sb_append(sb,"\tidiv %s\n",handle_value(sb,right,context));
        }
        sb_append(sb,"\tmov %s, rax\n",get_register_str(target_register));

    } else { // left is value
        generate_asm_for_expression(sb,right,context, target_register);

        sb_append(sb,"\tmov rax, %s\n", handle_value(sb,left,context));
        sb_append(sb,"\tcqo\n");
        sb_append(sb,"\tidiv %s\n",get_register_str(target_register));
        sb_append(sb,"\tmov %s, rax\n",get_register_str(target_register));
    }

}

void handle_int_simple_ops(StringBuilder* sb, AstNode* stm, FunctionContext* context, Register target_register) {
    AstNode* right = stm->binary_operation.right;
    AstNode* left = stm->binary_operation.left;

    bool right_is_value = is_value_ast(right);
    bool left_is_value = is_value_ast(left);

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
        default:
            PANIC("OPERATION NOT SUPPORTED: %s",format_token_kind(opp_kind));
    }

    if(!right_is_value && !left_is_value) { // have to allocate a new register
        Register new_register = take_next_available_register(&context->available_registers);
        if( new_register == 0 ) {
            PANIC("RUN OUT OF REGISTERS");
        }
        add_register(&context->touched_registers,new_register);

        generate_asm_for_expression(sb,left,context, new_register);
        generate_asm_for_expression(sb,right,context, target_register);

        sb_append(sb,"\t%s %s, %s\n",opp,get_register_str(target_register),get_register_str(new_register));
        add_register(&context->available_registers,new_register);

    } else if ( right_is_value && left_is_value ) { // bottom of the tree

        sb_append(sb,"\tmov %s, %s\n", get_register_str(target_register), handle_value(sb,left,context));
        sb_append(sb,"\t%s %s, %s\n", opp, get_register_str(target_register), handle_value(sb,right,context));

    } else if ( right_is_value ) { 

        generate_asm_for_expression(sb,left,context, target_register);
        sb_append(sb,"\t%s %s, %s\n", opp, get_register_str(target_register), handle_value(sb,right,context));

    } else { // left is value
        
        generate_asm_for_expression(sb,right,context, target_register);
        sb_append(sb,"\t%s %s, %s\n", opp, get_register_str(target_register), handle_value(sb,left,context));
    }
}
void handle_assing_op(StringBuilder* sb, AstNode* stm, FunctionContext* context, Register target_register) {
    AstNode* right = stm->binary_operation.right;
    AstNode* left = stm->binary_operation.left;

    TokenKind opp_kind = stm->binary_operation.opp_token.kind;
    ASSERT((opp_kind == ASSIGN), "EXPECTED ASSING");

    switch(left->type) {
        case AST_UNARY_OPERATION: // Dereferance
            Register new_register = take_next_available_register(&context->available_registers);
            if( new_register == 0 ) {
                PANIC("RUN OUT OF REGISTERS");
            }
            add_register(&context->touched_registers,new_register);

            generate_asm_for_expression(sb,left->unary_operation.right, context, new_register);
            generate_asm_for_expression(sb,right, context, target_register);
            sb_append(sb, "\tmov [%s], %s\n", get_register_str(new_register), get_register_str(target_register));

            add_register(&context->available_registers,new_register);
            break;
        case AST_IDENTIFIER:
            generate_asm_for_expression(sb,right, context, target_register);

            VariableLocation location = get_location_of_variable(context->variables_list,left->identifier.token.value);
            sb_append(sb,"\tmov %s, %s\n", get_location_str(location), get_register_str(target_register));
            break;
        default:
            PANIC("PANICKED");
    }
}
// ================ UNARY OPS ======================

void handle_dereferance_op(StringBuilder* sb, AstNode* stm, FunctionContext* context, Register target_register) {
    AstNode* right = stm->unary_operation.right;

    TokenKind opp_kind = stm->unary_operation.opp_token.kind;
    ASSERT(( opp_kind == STAR), "EXPECTED STAR");

    generate_asm_for_expression(sb,right,context, target_register);
    sb_append(sb,"\tmov %s, [%s]\n", get_register_str(target_register), get_register_str(target_register));
 }

void handle_get_address_op(StringBuilder* sb, AstNode* stm, FunctionContext* context, Register target_register) {
    AstNode* right = stm->unary_operation.right;

    TokenKind opp_kind = stm->unary_operation.opp_token.kind;
    ASSERT(( opp_kind == AMPERSAND), "EXPECTED AMPERSAND");

    ASSERT((right->type == AST_IDENTIFIER), "It is only possible to take an address of a variable for now");
    VariableLocation location = get_location_of_variable(context->variables_list, right->identifier.token.value);
    sb_append(sb,"\tlea %s, %s\n", get_register_str(target_register), get_location_str(location));
}

void handle_boolian_binary_cmp(StringBuilder *sb, AstNode* stm, FunctionContext* context) {
    Register target_register = take_next_available_register(&context->available_registers);
    if( target_register == 0 ) {
        PANIC("RUN OUT OF REGISTERS");
    }
    add_register(&context->touched_registers,target_register);

    AstNode* right = stm->binary_operation.right;
    AstNode* left = stm->binary_operation.left;

    bool right_is_value = is_value_ast(right);
    bool left_is_value = is_value_ast(left);

    TokenKind opp_kind = stm->binary_operation.opp_token.kind;

    if(!right_is_value && !left_is_value) { // have to allocate a new register
        Register new_register = take_next_available_register(&context->available_registers);
        if( new_register == 0 ) {
            PANIC("RUN OUT OF REGISTERS");
        }
        add_register(&context->touched_registers,new_register);

        generate_asm_for_expression(sb,left,context, new_register);
        generate_asm_for_expression(sb,right,context, target_register);

        sb_append(sb,"\tcmp %s, %s\n", get_register_str(target_register),get_register_str(new_register));

        add_register(&context->available_registers,new_register);
    } else if ( right_is_value && left_is_value ) { // bottom of the tree

        sb_append(sb,"\tmov %s, %s\n", get_register_str(target_register), handle_value(sb,left,context));
        sb_append(sb,"\tcmp %s, %s\n",  get_register_str(target_register), handle_value(sb,right,context));

    } else if ( right_is_value ) { 

        generate_asm_for_expression(sb,left,context, target_register);
        sb_append(sb,"\tcmp %s, %s\n",  get_register_str(target_register), handle_value(sb,right,context));

    } else { // left is value
        
        generate_asm_for_expression(sb,right,context, target_register);
        sb_append(sb,"\tcmp %s, %s\n",  get_register_str(target_register), handle_value(sb,left,context));
    }

    add_register(&context->available_registers,target_register);
}

void handle_boolian_binary_ops(StringBuilder *sb, AstNode* stm, FunctionContext* context, Register target_register) {
    AstNode* right = stm->binary_operation.right;
    AstNode* left = stm->binary_operation.left;

    bool right_is_value = is_value_ast(right);
    bool left_is_value = is_value_ast(left);

    TokenKind opp_kind = stm->binary_operation.opp_token.kind;

    if(!right_is_value && !left_is_value) { // have to allocate a new register
        Register new_register = take_next_available_register(&context->available_registers);
        if( new_register == 0 ) {
            PANIC("RUN OUT OF REGISTERS");
        }
        add_register(&context->touched_registers,new_register);

        generate_asm_for_expression(sb,left,context, new_register);
        generate_asm_for_expression(sb,right,context, target_register);

        sb_append(sb,"\tcmp %s, %s\n", get_register_str(target_register),get_register_str(new_register));

        add_register(&context->available_registers,new_register);
    } else if ( right_is_value && left_is_value ) { // bottom of the tree

        sb_append(sb,"\tmov %s, %s\n", get_register_str(target_register), handle_value(sb,left,context));
        sb_append(sb,"\tcmp %s, %s\n",  get_register_str(target_register), handle_value(sb,right,context));

    } else if ( right_is_value ) { 

        generate_asm_for_expression(sb,left,context, target_register);
        sb_append(sb,"\tcmp %s, %s\n",  get_register_str(target_register), handle_value(sb,right,context));

    } else { // left is value
        
        generate_asm_for_expression(sb,right,context, target_register);
        sb_append(sb,"\tcmp %s, %s\n",  get_register_str(target_register), handle_value(sb,left,context));
    }

    switch(opp_kind) {
        case EQUAL:     sb_append(sb,"\tsete  al\n"); break;
        case NOT_EQUAL: sb_append(sb,"\tsetne al\n"); break;
        case LESS_THEN: sb_append(sb,"\tsetl  al\n"); break;
        case MORE_THEN: sb_append(sb,"\tsetg  al\n"); break;
        case LESS_EQUAL:sb_append(sb,"\tsetle al\n"); break;
        case MORE_EQUAL:sb_append(sb,"\tsetge al\n"); break;
        default:
            PANIC("PANICKED");
    }
    sb_append(sb,"\tmovzx %s, al\n", get_register_str(target_register));
}
