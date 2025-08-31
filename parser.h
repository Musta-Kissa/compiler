#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "types.h"

typedef enum {
    AST_BINARY_OPERATION,   
    AST_UNARY_OPERATION,    
    AST_FUNC_CALL,          
    AST_ARGUMENT,          
    AST_ARGUMENT_DECLARATION,          
    AST_NUMBER,            
    AST_STRING,             
    AST_IDENTIFIER,       
    AST_DECLARATION,    
    AST_FUNCTION_DECLARATION,
    AST_RETURN_STATEMENT,   
    AST_IF_STATEMENT,       
    AST_WHILE_STATEMENT,   
    AST_FOR_STATEMENT,    
    AST_EXPRESSION_STATEMENT,
    AST_BLOCK_STATEMENT,

    AST_STRUCT_DECLARATION,
} Ast_ExprType;

typedef struct TypeInfo {
    uint8_t star_number;
    uint8_t is_array; 
    char* type_name;
} TypeInfo;

typedef struct AstExpr {
    Ast_ExprType type;            
    union {
        struct BinaryOperation {
            Type type;
            Token opp_token;        
            struct AstExpr* left; 
            struct AstExpr* right; 
        } binary_operation;
        struct UnaryOperation {
            //Type* type;
            Token opp_token;        
            struct AstExpr* right; 
        } unary_operation; // TODO implement unary in parser
        struct FuncCall {
            Token identifier;
            struct AstExpr* args; // argument*
        } func_call;   
        struct FuncArg {
            struct AstExpr* value; // expression_statement*
            struct AstExpr* next; // CAN BE NULL
        } argument;
        struct ArgDecl {
            Type* type;
            char* ident;
            struct AstExpr* next; // CAN BE NULL
        } argument_decl;
        struct Number {
            Token token;   
        } number;     
        struct AstString {
            Token token;   
        } string;     
        struct Identifier {
            Token token;
        } identifier;
        struct Declaretion {
            Type* type;
            char* name;         
            struct AstExpr* value; // AST_EXPRESSION_STATEMENT // CAN BE NULL
            struct AstExpr* next; // CAN BE NULL
        } declaration;
        struct FunctionDeclaration {
            Type* return_type;
            char* name;          
            struct AstExpr* args;      
            struct AstExpr* body; // BlockStatment
            struct AstExpr* next; // CAN BE NULL
        } function_declaration;   
        struct IfStatement {
            struct AstExpr* condition;
            struct AstExpr* body; // BlockStatment
            struct AstExpr* else_block; // NOT IMPLEMENTED
            struct AstExpr* next;
        } if_statement;
        struct ForStatement {
            struct AstExpr* initial;
            struct AstExpr* condition;
            struct AstExpr* iteration;
            struct AstExpr* body; // BlockStatment
            struct AstExpr* next;
        } for_statement;
        struct WhileStatement {
            struct AstExpr* condition;
            struct AstExpr* body; // BlockStatment
            struct AstExpr* next;
        } while_statement;
        struct ReturnStatement {
            struct AstExpr* expression;
            struct AstExpr* next; // Can be NULL
        } return_statement;
        struct BlockStatment {
            struct AstExpr* statements; // Can be NULL
            struct AstExpr* next; // Can be NULL
        } block_statement;
        struct ExpressionStatement {
            struct AstExpr* value; // Can be NULL
            struct AstExpr* next; // Can be NULL
        } expression_statement;
        struct StructDeclaration {
            char* name;
            struct AstExpr* body; // BlockStatment
            struct AstExpr* next; // Can be NULL
        } struct_declaration;
    };
} AstExpr;

int get_binding_power(Token opp);
int is_opp(Token k);
int is_unary(Token k);

AstExpr* parse_expr_statement(Lexer* lexer);
AstExpr* parse_expr(Lexer* lexer, int curr_bp);
AstExpr* parse_decl(Lexer* lexer);
AstExpr* parse_func_decl(Lexer* lexer);
AstExpr* parse_program(Lexer* lexer);
AstExpr* parse_arg_decl(Lexer* lexer);
AstExpr* parse_args(Lexer* lexer);
AstExpr* parse_statements(Lexer* lexer);
AstExpr* parse_statement(Lexer* lexer);
AstExpr* parse_function_call(Lexer* lexer,Token ident);
AstExpr* parse_unary(Lexer* lexer, Token opp);
TypeInfo parse_type_info(Lexer* lexer);
Type* parse_type(Lexer* lexer);

AstExpr* Ast_make_number(Token number);
AstExpr* Ast_make_ident(Token ident);
AstExpr* AST_make_binary(AstExpr* left, Token opp, AstExpr* right);

#endif
