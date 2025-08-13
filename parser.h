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
    AST_BINARY_OPERATION,   // Represents binary operations (e.g., a + b)
    AST_UNARY_OPERATION,    // Represents unary operations (e.g., -a)
    AST_FUNC_CALL,          // Represents function calls (e.g., add(1, 2))
    AST_ARGUMENT,          
    AST_ARGUMENT_DECLARATION,          
    AST_NUMBER,             // Represents numeric literals (e.g., 42)
    AST_STRING,             
    AST_IDENTIFIER,         // Represents variable identifiers (e.g., x)
    AST_ASSIGNMENT,         // Represents assignment statements (e.g., x = 5)
    AST_DECLARATION,        // Represents variable declarations (e.g., int x = 10)
    AST_FUNCTION_DECLARATION,// Represents function declarations (e.g., int add(int a, int b) {})
    //AST_STATEMENT_LIST,     // Represents a list of statements
    AST_COMPOUND_STATEMENT, // Represents compound statements (e.g., { ... })
    //AST_RETURN_STATEMENT,   // Represents return statements (e.g., return x;)
    //AST_EMPTY_STATEMENT,    // Represents empty statements (e.g., ;)
    AST_IF_STATEMENT,       // Represents if statements (e.g., if (condition) { ... })
    //AST_WHILE_STATEMENT,    // Represents while statements (e.g., while (condition) { ... })
    //AST_FOR_STATEMENT,      // Represents for statements (e.g., for (int i = 0; i < 10; i++) { ... })
    AST_EXPRESSION_STATEMENT // Represents statements that are just expressions (e.g., x + y;)
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
    };
} AstExpr;

int get_binding_power(TokenKind opp);
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
