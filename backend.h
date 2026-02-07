#include "parser.h"
#include "types.h"
#include "my_string.h"
#include "analyzer.h"
#include "registers.h"

#include "dyn_arrays_macro.h"

typedef struct {
    make_da(char*)
} CStringList;

typedef struct {
    enum { REGISTER, STACK, NOT_ASSIGNED} type;
    union {
        struct {
            Register register_;
        } register_location;
        struct {
            int base_offset;
        } stack_location;
    };
} VariableLocation;

typedef struct {
    const char* identifier;
    int first_line_used;
    int last_line_used;
    VariableLocation location;
} VariableInfo;

typedef struct {
    make_da(VariableInfo)
} VariableInfoList;

typedef struct {
    Registers touched_registers;
    Registers available_registers;
    VariableInfoList variables_list;
    int current_line;
} FunctionContext;

typedef struct {
    make_da(AstNode*)
} FunctionArgumentsList;

const char* generate_asm_for_function(AstNode* stm); 

void generate_asm_for_function_call(StringBuilder* sb, AstNode* stm, FunctionContext* context);
Register generate_asm_for_expression(StringBuilder* sb, AstNode* stm, FunctionContext* context, Register target_register);
char* generate_asm(AstNode* program);
