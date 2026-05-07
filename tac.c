#include "tac.h"
#include "analyzer.h"
#include "dyn_arrays_macro.h"
#include "panic_macros.h"
#include <stdlib.h>



#define VOID_VAR() (TacVar){.kind=VAR_VOID}
#define LOCAL_VAR(ident) (TacVar){.kind=VAR_LOCAL, .ident = ident}
#define TEMP_VAR(id) (TacVar){.kind=VAR_TEMP, .temp_id=id}
#define LABEL_VAR(name) (TacVar){.kind=VAR_LABEL, .label_name=name}
#define INT_VAR(val) (TacVar){.kind=VAR_CONST_INT, .int_val=val}
#define FLOAT_VAR(val) (TacVar){.kind=VAR_CONST_FLOAT, .float_val=val}

#include "tac_print.c"

TacProgram TAC_PROGRAM = {0};
int        TEMP_IDX    =  0 ;
int        LABEL_IDX   =  0 ;

TacType tac_type_from_type(Type* type) {
    switch(type->type_kind) {
        case INTIGER_TYPE:  
            switch(type->intiger_type.size) {
                case BITS_8:
                case BITS_16:
                case BITS_32:
                    PANIC("NOT SUPPORTED");
                case BITS_64: return TAC_I64;
            } PANIC("UNRACHABLE");
        case FLOAT_TYPE:   
            switch(type->float_type.size) {
                case BITS_8:
                case BITS_16:
                case BITS_32:
                    PANIC("NOT SUPPORTED");
                case BITS_64: return TAC_F64;
            } PANIC("UNRACHABLE");
        case BOOL_TYPE:   
            switch(type->bool_type.size) {
                case BITS_8:
                case BITS_16:
                case BITS_32:
                    PANIC("NOT SUPPORTED");
                case BITS_64: return TAC_B64;
            } PANIC("UNRACHABLE");
        case POINTER_TYPE:   
            return TAC_PTR;
        case VOID_TYPE:   
            return TAC_VOID;
        default:
            PANIC();
    }
}

void tac_assign(TacInstrDA* instructions, AstNode* stm) {
    switch(stm->binary_operation.left->type) {
        case AST_IDENTIFIER: // regular assign
            char* ident = stm->binary_operation.left->identifier.token.value;
            int temp_idx = tac_expression(instructions, stm->binary_operation.right);
            da_append_ref(instructions, ((TacInstr){ 
                .op=TAC_MOV, 
                .move.dest = LOCAL_VAR(ident),
                .move.src = TEMP_VAR(temp_idx)
            }));
            break;
        case AST_UNARY_OPERATION: // regular dereferenced assing
            int dest_temp_idx = tac_expression(instructions, stm->binary_operation.left->unary_operation.right);
            int src_temp_idx = tac_expression(instructions, stm->binary_operation.right);
            da_append_ref(instructions, ((TacInstr){ 
                .op=TAC_STORE, 
                .move.dest = TEMP_VAR(dest_temp_idx),
                .move.src = TEMP_VAR(src_temp_idx)
            }));
            break;
        default:
            PANIC("UNREACHABLE, ANALYZER ERROR");
    }
}

int tac_function_call(TacInstrDA* instructions,AstNode* stm) {
    char* ident = stm->function_call.identifier.value;
    TacVarDA args = {0};

    AstNode* curr_arg = stm->function_call.args;
    for(;curr_arg != NULL; curr_arg = curr_arg->argument.next ) {
        int temp_idx = tac_expression(instructions,curr_arg->argument.value->expression_statement.expression);
        da_append(args, TEMP_VAR(temp_idx));
    }

    TacType type = tac_type_from_type(stm->function_call.return_type);
    TacVar var;
    int temp_idx;

    if( type == TAC_VOID ) {
        temp_idx = -1;
        var = VOID_VAR();
    } else {
        temp_idx = TEMP_IDX++;
        var = TEMP_VAR(temp_idx);
    }
    TacInstr instr = { 
        .op=TAC_FCALL, 
        .type=type,
        .fcall.result = var, 
        .fcall.ident = ident, 
        .fcall.args = args 
    };

    da_append_ref(instructions, instr);
    return temp_idx;
}

int tac_expression(TacInstrDA* instructions, AstNode* stm) {
    switch(stm->type) {
        case AST_BINARY_OPERATION: {
            AstNode* left = stm->binary_operation.left;
            AstNode* right = stm->binary_operation.right;

            TacOp op;
            switch(stm->binary_operation.opp_token.kind) {
                case PLUS:          op = TAC_ADD; break;
                case MINUS:         op = TAC_SUB; break;
                case STAR:          op = TAC_MUL; break;
                case DIVITION:      op = TAC_DIV; break;
                case EQUAL:         op = TAC_CMP_EQ; break;
                case NOT_EQUAL:     op = TAC_CMP_NE; break;
                case LESS_THEN:     op = TAC_CMP_LT; break;
                case MORE_THEN:     op = TAC_CMP_GT; break;
                case LESS_EQUAL:    op = TAC_CMP_LE; break;
                case MORE_EQUAL:    op = TAC_CMP_GE; break;

                case ASSIGN:
                    tac_assign(instructions,stm);
                    return -1;
                    TODO("ASSING");

                default:
                    PANIC("NOT SUPPORTED: %s",format_token(stm->binary_operation.opp_token));
            }

            int temp_left  = tac_expression(instructions,left);
            int temp_right = tac_expression(instructions,right);

            TacType op_type = tac_type_from_type(stm->binary_operation.op_type);

            int result_temp_idx = TEMP_IDX++;
            TacInstr instr = (TacInstr){ 
                .op=op, 
                .type=op_type, 
                .binary.result = TEMP_VAR(result_temp_idx), 
                .binary.arg1 = TEMP_VAR(temp_left), 
                .binary.arg2 = TEMP_VAR(temp_right) 
            };
            da_append_ref(instructions, instr);
            return result_temp_idx;
        } PANIC("Unreachable");
        case AST_UNARY_OPERATION: {
            switch(stm->unary_operation.opp_token.kind) {
                case STAR: {
                    TacType op_type = tac_type_from_type(stm->unary_operation.op_type);
                    int dest_temp_idx = TEMP_IDX++;
                    int src_temp_idx = tac_expression(instructions,stm->unary_operation.right);
                    TacInstr instr = { .op = TAC_LOAD, .type = op_type, .move.dest = TEMP_VAR(dest_temp_idx), .move.src = TEMP_VAR(src_temp_idx)};
                    da_append_ref(instructions,instr);
                    return dest_temp_idx;
                }   break;
                case AMPERSAND: {
                    int dest_temp_idx = TEMP_IDX++;
                    int src_temp_idx = tac_expression(instructions,stm->unary_operation.right);
                    TacInstr instr = { .op = TAC_ADDR, .move.dest = TEMP_VAR(dest_temp_idx), .move.src = TEMP_VAR(src_temp_idx)};
                    da_append_ref(instructions,instr);
                    return dest_temp_idx;
                }   break;
                default:
                    PANIC("NOT SUPPORTED: %s",format_token(stm->unary_operation.opp_token));
            }
        } PANIC("Unreachable");
        case AST_NUMBER: {
            int temp_idx = TEMP_IDX++;
            da_append_ref(instructions, ((TacInstr){ 
                .op=TAC_MOV, 
                .move.dest = TEMP_VAR(temp_idx),
                .move.src = INT_VAR(atoi(stm->number.value))
            }));
            return temp_idx;
        } PANIC("Unreachable");
        case AST_IDENTIFIER: {
            int temp_idx = TEMP_IDX++;
            char* ident = stm->identifier.token.value;
            da_append_ref(instructions, ((TacInstr){ 
                .op=TAC_MOV, 
                .move.dest = TEMP_VAR(temp_idx),
                .move.src = LOCAL_VAR(ident),
            }));
            return temp_idx;
        } PANIC("Unreachable");
        case AST_FUNC_CALL: {
            return tac_function_call(instructions,stm);
        } PANIC("Unreachable");
        case AST_STRING:
        default:
            PANIC("Not supported");
    }
}
void tac_declaration(TacInstrDA* instructions, AstNode* stm) {
    TacType type = tac_type_from_type(stm->declaration.type);
    char* ident = stm->declaration.name;

    if( stm->declaration.value == NULL || stm->declaration.value->expression_statement.expression == NULL) {
        da_append_ref(instructions, ((TacInstr){ .op=TAC_ALLOC, .type=type, .unary.src = LOCAL_VAR(ident)}) );
    } else {
        int temp_idx = tac_expression(instructions,stm->declaration.value->expression_statement.expression);
        da_append_ref(instructions, ((TacInstr){ 
            .op=TAC_MOV, 
            .move.dest = LOCAL_VAR(ident), 
            .move.src = TEMP_VAR(temp_idx)
        }));
    }
}
char* gen_label() {
    char* label = (char*)calloc(sizeof(char), 10);
    sprintf(label, "L%d", LABEL_IDX++);
    return label;
}
void tac_while(TacInstrDA* instructions, AstNode* stm) {
    char* condition_label = gen_label();
    da_append_ref(instructions, ((TacInstr){ .op = TAC_JMP, .jump.label = condition_label }));

    char* body_label = gen_label();
    da_append_ref(instructions, ((TacInstr){ .op = TAC_LABEL, .unary.src = LABEL_VAR(body_label) }));

    tac_statements(instructions, stm->while_statement.body);
    
    da_append_ref(instructions, ((TacInstr){ .op = TAC_LABEL, .unary.src = LABEL_VAR(condition_label) }));
    int cond_temp = tac_expression(instructions, stm->while_statement.condition->expression_statement.expression);
    da_append_ref(instructions, ((TacInstr){
        .op = TAC_JMP_IF,
        .jump.src = TEMP_VAR(cond_temp),
        .jump.label = body_label
    }));

}
void tac_if(TacInstrDA* instructions, AstNode* current) {
    char* end_label = gen_label();
    bool encountered_else = false;
    
    while (current && current->type == AST_IF_STATEMENT) {
        // Label to jump to when condition is false (skip the then-block)
        char* false_label = gen_label();

        int cond_temp = tac_expression(instructions, current->if_statement.condition->expression_statement.expression);
        da_append_ref(instructions, ((TacInstr){
            .op = TAC_JMP_IF_NOT,
            .jump.src = TEMP_VAR(cond_temp),
            .jump.label = false_label
        }));
        tac_statements(instructions, current->if_statement.body);
        
        // If there is any else part (another if or a block), we need to
        // jump over it after finishing the then-block.
        AstNode* else_part = current->if_statement.else_block;
        if (else_part != NULL) {
            encountered_else = true;
            da_append_ref(instructions, ((TacInstr){ .op = TAC_JMP, .jump.label = end_label }));
        }

        da_append_ref(instructions, ((TacInstr){ .op = TAC_LABEL, .unary.src = LABEL_VAR(false_label) }));

        // If it's another if, continue the loop to generate another condition. 
        // If it's a non‑if block, generate its statements and then stop. 
        // If NULL, stop.
        if (else_part == NULL) {
            break;
        } else if (else_part->type == AST_IF_STATEMENT) {
            current = else_part;
        } else {
            tac_statements(instructions, else_part);
            break;
        }
    }
    if( encountered_else ) {
        da_append_ref(instructions, ((TacInstr){ .op = TAC_LABEL, .unary.src = LABEL_VAR(end_label) }));
    } else {
        free(end_label);
    }
}
/*
void tac_if(TacInstrDA* instructions, AstNode* curr_if) {
    char* end_label = (char*)calloc(sizeof(char), 10);
    sprintf(end_label, "L%d", LABEL_IDX++);


    while(1){
        char* end_of_block_label = (char*)calloc(sizeof(char), 10);
        sprintf(end_of_block_label, "L%d", LABEL_IDX++);
       
        int temp_idx =  tac_expression(instructions, curr_if->if_statement.condition->expression_statement.expression);

        da_append_ref(instructions, ((TacInstr){ .op=TAC_JMP_IF_NOT, .jump.src = TEMP_VAR(temp_idx), .jump.label = end_of_block_label }) );
        tac_statements(instructions,curr_if->if_statement.body);
        if( curr_if->if_statement.else_block != NULL ) {
            da_append_ref(instructions, ((TacInstr){ .op=TAC_JMP, .jump.label = end_label }) );
        }
        da_append_ref(instructions, ((TacInstr){ .op=TAC_LABEL, .unary.src = LABEL_VAR(end_of_block_label)}) );

        if( curr_if->if_statement.else_block != NULL ) {
            if( curr_if->if_statement.else_block->type == AST_IF_STATEMENT ) {
                curr_if = curr_if->if_statement.else_block;
            } else {
                tac_statements(instructions,curr_if->if_statement.else_block);
                break;
            }
        } else {
            break;
        }
    }
    da_append_ref(instructions, ((TacInstr){ .op=TAC_LABEL, .unary.src = LABEL_VAR(end_label)}) );
}
*/
void tac_return(TacInstrDA* instructions, AstNode* stm) {
    TacType type = tac_type_from_type(stm->return_statement.return_type);
    int temp_idx = tac_expression(instructions,stm->return_statement.expression->expression_statement.expression);
    da_append_ref(instructions, ((TacInstr){ .op=TAC_RET, .type=type, .unary.src = TEMP_VAR(temp_idx)}) );
}

void tac_statements(TacInstrDA* instructions, AstNode* next) {
    while( next != NULL ) {
        AstNode* curr = next;

        switch(curr->type) {
            case AST_BLOCK_STATEMENT:
                tac_statements(instructions, curr->block_statement.statements);
                next = curr->block_statement.next;
                break;
            case AST_EXPRESSION_STATEMENT:
                tac_expression(instructions, curr->expression_statement.expression);
                next = next->expression_statement.next;
                break;
            case AST_DECLARATION:
                tac_declaration(instructions, curr);
                next = curr->declaration.next;
                break;
            case AST_RETURN_STATEMENT:
                tac_return(instructions, curr);
                next = next->return_statement.next;
                break;
            case AST_IF_STATEMENT:
                tac_if(instructions, curr);
                next = next->if_statement.next;
                break;
            case AST_WHILE_STATEMENT:
                tac_while(instructions, curr);
                next = next->while_statement.next;
                break;
            case AST_STRUCT_DECLARATION:    
                TODO("TAC STRUCT DECL");
                next = next->struct_declaration.next;
                break;
            case AST_FOR_STATEMENT:
                TODO("TAC FOR STM");
                next = next->for_statement.next;
                break;
            default:
                PANIC("NOT SUPPORTED: %s",format_ast_type(curr));
        }
    }
}

void tac_function_declaration(TacInstrDA* instructions, AstNode* stm) {
    tac_statements(instructions,stm->function_declaration.body);
}
void tac_get_function_args(ProcArgs *args, AstNode *fn) {
    AstNode* arg_decl = fn->function_declaration.args;
    while(arg_decl != NULL) {
        TacType type = tac_type_from_type(arg_decl->argument_decl.type);
        char *identifier = arg_decl->argument_decl.ident;
        ProcArg arg = {.ident = identifier, .type = type };
        da_append_ref(args,arg);

        arg_decl = arg_decl->argument_decl.next;
    }
}

void tac_extern_statement(AstNode* stm) {
    switch( stm->type ) {
        case AST_FUNCTION_DECLARATION:
            da_append(TAC_PROGRAM.extern_declarations, stm->function_declaration.name);
            break;
        case AST_BLOCK_STATEMENT:
            AstNode* next = stm->block_statement.statements;
            while( next != NULL ) {
                da_append(TAC_PROGRAM.extern_declarations, next->function_declaration.name);
                next = next->function_declaration.next;
            }
            break;
    }
}
void tac_top_level_statements(AstNode* stm) {
    AstNode* next = stm;
    while( next != NULL ) {
        switch( next->type ) { /* ALL POSSIBLE TOP LEVEL STATEMENTS */
            case AST_FUNCTION_DECLARATION:
                TacInstrDA instructions = {0};
                ProcArgs args = {0};
                tac_get_function_args(&args, next);

                TEMP_IDX = 0;
                tac_function_declaration(&instructions, next);
                da_append(TAC_PROGRAM.procedures,((TacProc){.ident=next->function_declaration.name, .instructions=instructions,.args=args}));
                next = next->function_declaration.next;
                break;
            case AST_EXTERN_STATEMENT:    
                tac_extern_statement(next->extern_statement.body);
                next = next->extern_statement.next;
                break;
            case AST_DECLARATION: 
                TODO("GLOBAL DECLARATION");
                next = next->declaration.next;
                break;
            case AST_STRUCT_DECLARATION:    
                TODO("STRUCT DECLARATION");
                next = next->struct_declaration.next;
                break;
            default:
                PANIC("UNREACHABLE: %s",format_ast_type(next));
        }
    }
}

TacProgram generate_tac(AstNode* program) {
    tac_top_level_statements(program);
    return TAC_PROGRAM;

    /*
    TacVar result = (TacVar){ .kind=VAR_TEMP, .temp_id=1};
    TacVar ptr_var = (TacVar){ .kind=VAR_LOCAL, .ident="ptr_var"};
    TacVar arg1 = (TacVar){ .kind=VAR_LOCAL, .ident="var_a"};
    TacVar arg2 = (TacVar){ .kind=VAR_LOCAL, .ident="var_b"};

    da_append(instructions, ((TacInstr){ .op=TAC_ADD, .type=TAC_I64, .binary.result = result, .binary.arg1 = arg1, .binary.arg2 = arg2 }) );
    da_append(instructions, ((TacInstr){ .op=TAC_RET, .type=TAC_I64, .unary.src = result}) );
    da_append(instructions, ((TacInstr){ .op=TAC_MOV, .type=TAC_VOID, .move.dest = ptr_var, .move.src = TEMP_VAR(51)}) );
    da_append(instructions, ((TacInstr){ .op=TAC_STORE, .type=TAC_I64, .move.dest = ptr_var, .move.src = TEMP_VAR(51)}) );
    da_append(instructions, ((TacInstr){ .op=TAC_LOAD, .type=TAC_F64, .move.dest = TEMP_VAR(51), .move.src = ptr_var}) );
    da_append(instructions, ((TacInstr){ .op=TAC_ADDR, .move.dest = ptr_var, .move.src = TEMP_VAR(51)}) );
    da_append(instructions, ((TacInstr){ .op=TAC_ALLOC, .type=TAC_F64, .unary.src = TEMP_VAR(12)}) );
    da_append(instructions, ((TacInstr){ .op=TAC_LABEL, .unary.src = LABEL_VAR("LABEL")}) );

    TacVarDA args = {0};
    da_append(args,arg1);
    da_append(args,arg2);

    da_append(instructions, ((TacInstr){ .op=TAC_FCALL, .type=TAC_I64, .fcall.result = VOID_VAR(), .fcall.ident = "foo", .fcall.args = args }) );
    da_append(instructions, ((TacInstr){ .op=TAC_FCALL, .type=TAC_I64, .fcall.result = TEMP_VAR(4), .fcall.ident = "foo", .fcall.args = args }) );

    da_append(instructions, ((TacInstr){ .op=TAC_CMP_EQ, .type=TAC_I64, .binary.result = TEMP_VAR(3), .binary.arg1 = arg1, .binary.arg2 = arg2 }) );
    da_append(instructions, ((TacInstr){ .op=TAC_JMP_IF, .jump.src = TEMP_VAR(3), .jump.label = "L_one" }) );
    da_append(instructions, ((TacInstr){ .op=TAC_JMP_IF_NOT, .jump.src = TEMP_VAR(4), .jump.label = "L_three" }) );
    da_append(instructions, ((TacInstr){ .op=TAC_JMP, .jump.label = "L_two" }) );
    //da_append(instructions, ((TacInstr){ .op=TAC_EXTERN, .unary.src = LABEL_VAR("bar") }) );

    print_tac(instructions);
    printf("\n");

    instructions = (TacInstrDA){0};
    */

}
