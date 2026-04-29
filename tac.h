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

    TAC_COND_BR,
    TAC_BR,

    TAC_CMP_EQ,
    TAC_CMP_NE,
    TAC_CMP_LT,
    TAC_CMP_GT,
    TAC_CMP_LE,
    TAC_CMP_GE,
} TacOp;

typedef enum {
    TAC_VOID = 0,
    TAC_I64,
    TAC_F64,
} TacType;

typedef enum {
    VAR_VOID = 0, // for hanging fcalls
    
    VAR_TEMP,
    VAR_LOCAL, 

    VAR_LABEL,

    VAR_CONST_INT,
    VAR_CONST_FLOAT,
} TacVarKind;

typedef struct {
    TacVarKind kind;
    union {
        int     temp_id;       // for temps: e.g., 1 -> "t1"
        char    *ident;        // for local vars: "a", "b"
        int     int_val;
        float   float_val;
        //char    *label_name;  // for labels: "L1", "L2"
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
        struct { TacVar result; char* ident; TacVarDA args; } fcall;
        struct { TacVar src; char* label; } branch;
    };
} TacInstr;

typedef struct {
    make_da(TacInstr)    
} TacInstrDA;


TacInstrDA generate_tac(AstNode* node);

#endif
