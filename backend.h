#ifndef BACKEND_H
#define BACKEND_H

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
    VariableLocation location;
} VariableInfo;

typedef struct {
    make_da(VariableInfo)
} VariableInfoList;

typedef struct {
    Registers touched_registers;
    Registers available_registers;
    VariableInfoList variables_list;
    int return_label_number;
} FunctionContext;

typedef struct {
    int curr_label_number;
} ProgramContext;

typedef struct {
    make_da(AstNode*)
} FunctionArgumentsList;

const char* generate_asm_for_function(AstNode* stm); 

void generate_asm_for_function_call(StringBuilder* sb, AstNode* stm, FunctionContext* context);
void generate_asm_for_expression(StringBuilder* sb, AstNode* stm, FunctionContext* context, Register target_register);
char* generate_asm(AstNode* program);
bool is_value_ast(AstNode* stm);
char* handle_value(StringBuilder* sb, AstNode* stm, FunctionContext* context);
const VariableLocation get_location_of_variable(const VariableInfoList variables_list, const char* variable_identifier);
char* get_location_str(VariableLocation location);
void generate_asm_for_statements(StringBuilder* sb, AstNode* next,FunctionContext* context);


#endif
