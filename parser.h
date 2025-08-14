#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

/*
typedef struct Expr {
    Token opp;
    struct Expr* lhs;
    struct Expr* rhs;
}Expr;
*/

typedef enum {
    AST_BINARY_OPERATION,   
    AST_UNARY_OPERATION,    
    AST_FUNC_CALL,          
    AST_ARGUMENT,          
    AST_ARGUMENT_DECLARATION,          
    AST_NUMBER,            
    AST_STRING,             
    AST_IDENTIFIER,       
    AST_ASSIGNMENT,      
    AST_DECLARATION,    
    AST_FUNCTION_DECLARATION,
    AST_COMPOUND_STATEMENT, 
    //AST_RETURN_STATEMENT,   
    AST_IF_STATEMENT,       
    AST_WHILE_STATEMENT,   
    AST_FOR_STATEMENT,    
    AST_EXPRESSION_STATEMENT 
    //AST_EMPTY_STATEMENT,  
} Ast_ExprType;

typedef struct AstExpr {
    Ast_ExprType type;            
    union {
        struct BinaryOperation {
            Token opp_token;        
            struct AstExpr* left; 
            struct AstExpr* right; 
        } binary_operation;
        struct UnaryOperation {
            Token opp_token;        
            struct AstExpr* right; 
        } unary_operation; // TODO implement unary in parser
        struct FuncCall {
            Token identifier;
            struct AstExpr* args;
        } func_call;   
        struct Arg {
            struct AstExpr* value;
            struct AstExpr* next; // CAN BE NULL
        } argument;
        struct ArgDecl {
            Token type;
            Token ident;
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
            Token type;          
            Token name;         
            struct AstExpr* value; // AST_EXPRESSION_STATEMENT // CAN BE NULL
            struct AstExpr* next; // CAN BE NULL
        } declaration;
        struct Assignment {
            Token ident;      
            struct AstExpr* value;      
        } assignment;             
        struct FunctionDeclaration {
            Token return_type;   
            Token name;          
            struct AstExpr* args;      
            struct AstExpr* body;
            struct AstExpr* next; // CAN BE NULL
        } function_declaration;   
        struct IfStatement {
            struct AstExpr* condition;
            struct AstExpr* body;
            struct AstExpr* next;
        } if_statement;
        struct ForStatement {
            struct AstExpr* initial;
            struct AstExpr* condition;
            struct AstExpr* iteration;
            struct AstExpr* body;
            struct AstExpr* next;
        } for_statement;
        struct WhileStatement {
            struct AstExpr* condition;
            struct AstExpr* body;
            struct AstExpr* next;
        } while_statement;
    };
} AstExpr;

int get_binding_power(Token opp);
int is_opp(Token k);
int is_type(Token k);
int is_unary(Token k);

AstExpr* parse_expr(Lexer* lexer, int curr_bp);
AstExpr* parse_decl(Lexer* lexer,Token type, Token ident);
AstExpr* parse_func_decl(Lexer* lexer,Token type,Token ident);
AstExpr* parse_program(Lexer* lexer);
AstExpr* parse_arg_decl(Lexer* lexer);
AstExpr* parse_args(Lexer* lexer);
AstExpr* parse_statement(Lexer* lexer);
AstExpr* parse_function_call(Lexer* lexer,Token ident);
AstExpr* parse_unary(Lexer* lexer, Token opp);

AstExpr* Ast_make_number(Token number);
AstExpr* Ast_make_ident(Token ident);
AstExpr* AST_make_binary(AstExpr* left, Token opp, AstExpr* right);

#endif
