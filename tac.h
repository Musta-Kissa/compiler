#ifndef TAC_H
#define TAC_H

#include "dyn_arrays_macro.h"
#include "parser.h"

typedef enum {
    TAC_ADD,
    TAC_SUB,
    TAC_MUL,
    TAC_DIV,

    TAC_RET,

    TAC_FCALL,
    //TAC_EXTERN, //deprecated
    TAC_LABEL,

    TAC_JMP_IF,
    TAC_JMP_IF_NOT,
    TAC_JMP,

    // ptr ops
    TAC_LOAD,        
    TAC_STORE,      
    TAC_ADDR,      

    TAC_ALLOC,

    TAC_MOV,

    TAC_CMP_EQ,
    TAC_CMP_NE,
    TAC_CMP_LT,
    TAC_CMP_GT,
    TAC_CMP_LE,
    TAC_CMP_GE,
    TAC_AND,
    TAC_OR,
} TacOp;

typedef enum {
    TAC_VOID = 0,
    TAC_PTR,
    TAC_I64,
    TAC_F64,
    //TAC_B64,
    TAC_U64,
} TacType;

typedef enum {
    VAR_VOID = 0, // for hanging fcalls
    
    VAR_TEMP,
    VAR_LOCAL, 

    VAR_LABEL,

    VAR_CONST_INT,
    VAR_CONST_UINT,
    VAR_CONST_FLOAT,
} TacVarKind;

typedef struct {
    TacVarKind kind;
    union {
        int     temp_id;       // for temps: e.g., 1 -> "t1"
        char    *ident;        // for local vars: "a", "b"
        int     int_val;
        unsigned int uint_val;
        float   float_val;
        char    *label_name;  // for labels: "L1", "L2"
    };
} TacVar;
typedef struct {
    make_da(TacVar)    
} TacVarDA;

typedef struct {
    TacOp op;
    TacType type; 
    union {
        struct { TacVar arg1, arg2, result; } binary;
        struct { TacVar src; } unary;
        struct { TacVar src; TacVar dest; } move;
        struct { TacVar result; char* ident; TacVarDA args; } fcall;
        struct { TacVar src; char* label; } jump;
    };
} TacInstr;

typedef struct {
    make_da(TacInstr)    
} TacInstrDA;

typedef struct {
    make_da(char*)
} CStringDA;

typedef struct {
    TacType type;
    char* ident;
} ProcArg;
typedef struct {
    make_da(ProcArg)
} ProcArgs;

typedef struct {
    TacInstrDA instructions;
    ProcArgs args; 
    char* ident;
} TacProc;

typedef struct {
    make_da(TacProc)
} TacProcDA;

typedef struct {
    CStringDA extern_declarations;
    TacProcDA procedures;
} TacProgram;

TacProgram generate_tac(AstNode* node);
int tac_expression(TacInstrDA* instructions, AstNode* stm);
void tac_statements(TacInstrDA* instructions, AstNode* next);
void print_tac(TacInstrDA instructions);
void print_tac_proc(TacProc proc);

void print_tac_var(TacVar var);

#endif
