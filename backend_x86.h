#include "tac.h"
#include "registers.h"

typedef struct {
    enum { NOT_ASSIGNED = 0, REGISTER, STACK, } type;
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
    TacType type;
    TacVar key;
    VariableLocation location;
    int first_line_used;
    int last_line_used;
} VariableInfo;

typedef struct {
    make_da(VariableInfo)
} VariableInfoDA;

typedef struct {
    Registers touched_registers;
    Registers available_registers;
    VariableInfoDA vars;
    int return_label_number;
} ProcContext;

typedef struct {
    make_da(float);
} FloatDA;

char* gen_x86_asm(TacProgram tac_program);
int get_var_info_ref(VariableInfoDA vars, VariableInfo **out, TacVar needle);
