#ifndef ANALYZER_H
#define ANALYZER_H

#define FRAMES_NUM 1000
#define VARS_NUM   1000
#define TYPES_NUM  1000

#include "parser.h"

typedef struct ArgType ArgType;

typedef enum TypeKind {
    FUNCTION_TYPE,
    STRUCT_TYPE,
    PRIMITIVE_TYPE,
    ENUM_TYPE,
    UNION_TYPE,
    //UNKNOWN_TYPE,
} TypeKind;

typedef struct Type {
    TypeKind type_kind;
    const char* type_name;
    union {
        struct FunctionType {
            ArgType* arg_types;
        } function_type;
    }
} Type;

struct ArgType {
    Type type;
    ArgType* next; // Can be NULL
};

Type Type_new(char* type_name, TypeKind type_kind);

typedef struct Variable {
    char* ident;
    Type type;
} Variable;

typedef struct Stack {
    int       frames[FRAMES_NUM];
    int       frames_idx;
    Variable  vars[VARS_NUM];
    int       pointer;
} Stack;

typedef struct Analyzer {
    Stack declared_vars;
    Type  types[TYPES_NUM];
    int   types_idx;
} Analyzer;

Stack Stack_new();
void Stack_new_frame(Stack* stk);
void Stack_pop_frame(Stack* stk);
void Stack_append(Stack* stk, Variable var);
void analyze_statements(AstExpr* stm);
void analyze_program_ast(AstExpr* ast);
Type analyze_expr_statement_inner(AstExpr* stm);
Type analyze_expr_statement(AstExpr* stm);
Type analyze_func_call(AstExpr* stm);
void analyze_func_call_args(AstExpr* stm);

#endif
