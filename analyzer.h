#ifndef ANALYZER_H
#define ANALYZER_H

#define NESTED_FUNCTIONS 1000
#define FRAMES_NUM 1000
#define VARS_NUM   1000
#define TYPES_NUM  1000

#include <stdint.h>
#include <stdbool.h>
#include "parser.h"
#include "types.h"
#include "dyn_arrays_macro.h"

typedef struct Variable {
    char* ident;
    Type type;
} Variable;

typedef struct {
    make_da(Variable)
} VariableList;

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

bool is_lvalue(AstNode* node);

typedef struct {
    bool encountered_return;
} AnalyzeStatementsReturn;

void Analyzer_init();
void Analyzer_append_type(Type type);
Type Analyzer_get_type(char* type_name,int* err);

Stack Stack_new();
void Stack_new_frame(Stack* stk);
void Stack_pop_frame(Stack* stk);
void Stack_append(Stack* stk, Variable var);
AnalyzeStatementsReturn analyze_statements(AstNode* stm);
void analyze_program_ast(AstNode* ast);
Type analyze_expr_statement_inner(AstNode* stm);
Type analyze_expr_statement(AstNode* stm);
Type analyze_func_call(AstNode* stm);
void analyze_func_call_args(AstNode* stm);
int type_is_impl(const char* type, ...);
Type create_type_from_ast_node(AstNode* node); // Depricated
int analyze_type(Type* type);
const char* format_ast_type(AstNode* stm);

#define type_is(...) type_is_impl(__VA_ARGS__,NULL)

#endif
