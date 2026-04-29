#include "tac.h"
#include "dyn_arrays_macro.h"

#define UNARY 1
#define BINARY 2
#define FCALL 3
#define BRANCH 4

#define make_void_var() (TacVar){.kind=VAR_VOID}
#define make_temp(id) (TacVar){.kind=VAR_TEMP, .temp_id=id}

#include "tac_print.c"

TacInstrDA generate_tac(AstNode* program) {
    TacInstrDA instructions = {0};
    TacVar result = (TacVar){ .kind=VAR_TEMP, .temp_id=1};
    TacVar arg1 = (TacVar){ .kind=VAR_LOCAL, .ident="var_a"};
    TacVar arg2 = (TacVar){ .kind=VAR_LOCAL, .ident="var_b"};

    da_append(instructions, ((TacInstr){ .op=TAC_ADD, .type=TAC_I64, .binary.result = result, .binary.arg1 = arg1, .binary.arg2 = arg2 }) );
    da_append(instructions, ((TacInstr){ .op=TAC_RET, .type=TAC_I64, .unary.src = result}) );

    TacVarDA args = {0};
    da_append(args,arg1);
    da_append(args,arg2);

    da_append(instructions, ((TacInstr){ .op=TAC_FCALL, .type=TAC_I64, .fcall.result = make_void_var(), .fcall.ident = "foo", .fcall.args = args }) );

    da_append(instructions, ((TacInstr){ .op=TAC_CMP_EQ, .type=TAC_I64, .binary.result = make_temp(3), .binary.arg1 = arg1, .binary.arg2 = arg2 }) );
    da_append(instructions, ((TacInstr){ .op=TAC_COND_BR, .branch.src = make_temp(3), .branch.label = "L_one" }) );
    da_append(instructions, ((TacInstr){ .op=TAC_BR, .branch.label = "L_two" }) );

    print_tac(instructions);

    return instructions;
}
