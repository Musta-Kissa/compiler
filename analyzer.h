#ifndef ANALYZER_H
#define ANALYZER_H

#define FRAMES_NUM 1000
#define VARS_NUM   1000
#define TYPES_NUM  1000

#include <stdint.h>
#include "parser.h"
#include "types.h"

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
int type_is_impl(const char* type, ...);
Type create_type_from_ast_node(AstExpr* node); // Depricated
int analyze_type(Type* type);

#define type_is(...) type_is_impl(__VA_ARGS__,NULL)

#endif
