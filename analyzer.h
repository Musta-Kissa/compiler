#ifndef ANALYZER_H
#define ANALYZER_H

#define FRAMES_NUM 1000
#define VARS_NUM   1000
#define TYPES_NUM  1000

#include "parser.h"

typedef struct Variable {
    char* ident;
    char* type;
} Variable;

typedef struct Stack {
    int       frames[FRAMES_NUM];
    int       frames_idx;
    Variable  vars[VARS_NUM];
    int       pointer;
} Stack;

typedef struct Analyzer {
    Stack declared_vars;
    char* types[TYPES_NUM];
    int   types_idx;
} Analyzer;

Stack Stack_new();
void Stack_new_frame(Stack* stk);
void Stack_pop_frame(Stack* stk);
void Stack_append(Stack* stk, Variable var);
void analyze_statements(AstExpr* stm);
void analyze_program_ast(AstExpr* ast);

#endif
