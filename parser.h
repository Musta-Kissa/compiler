#ifndef PRATT_H
#define PRATT_H

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
    //AST_UNARY_OPERATION,    // Represents unary operations (e.g., -a)
    AST_FUNC_CALL,          // Represents function calls (e.g., add(1, 2))
    AST_NUMBER,             // Represents numeric literals (e.g., 42)
    AST_IDENTIFIER,         // Represents variable identifiers (e.g., x)
    AST_ASSIGNMENT,         // Represents assignment statements (e.g., x = 5)
    AST_DECLARATION,        // Represents variable declarations (e.g., int x = 10)
    AST_FUNCTION_DECLARATION,// Represents function declarations (e.g., int add(int a, int b) {})
    //AST_STATEMENT_LIST,     // Represents a list of statements
    AST_COMPOUND_STATEMENT, // Represents compound statements (e.g., { ... })
    //AST_RETURN_STATEMENT,   // Represents return statements (e.g., return x;)
    //AST_EMPTY_STATEMENT,    // Represents empty statements (e.g., ;)
    //AST_IF_STATEMENT,       // Represents if statements (e.g., if (condition) { ... })
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
        struct FuncCall {
            struct AstExpr* ident;
            struct AstExpr* args;
        } func_call;   
        struct Number {
            Token token;   
        } number;     
        struct Identifier {
            Token token;
        } identifier;
        struct Declaretion {
            Token type;          
            Token name;         
            struct AstExpr* value; // AST_EXPRESSION_STATEMENT
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
        } function_declaration;   
    };
} AstExpr;

int get_binding_power(TokenKind opp);
int is_opp(TokenKind k);
AstExpr* parse_expr(Lexer* lexer, int curr_bp);

#endif
