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
    AST_EXTERN_STATEMENT,
} AstNode_Type;

typedef struct TypeInfo {
    uint8_t star_number;
    uint8_t is_array; 
    char* type_name;
} TypeInfo;

typedef struct AstNode {
    AstNode_Type type;            
    union {
        struct BinaryOperation {
            bool is_lvalue;
            Type* type;
            Token opp_token;        
            struct AstNode* left; 
            struct AstNode* right; 
        } binary_operation;
        struct UnaryOperation {
            bool is_lvalue;
            Type* type;
            Token opp_token;        
            struct AstNode* right; 
        } unary_operation; // TODO implement unary in parser
        struct FuncCall {
            Token identifier;
            struct AstNode* args; // argument*
        } function_call;   
        struct Arg {
            struct AstNode* value; // expression_statement*
            struct AstNode* next; // CAN BE NULL
        } argument;
        struct ArgDecl {
            Type* type;
            char* ident;
            struct AstNode* next; // CAN BE NULL
        } argument_decl;
        struct Number {
            char* value;
            Type* type;
        } number;     
        struct AstString {
            Token token;   
        } string;     
        struct Identifier {
            Type* type;
            Token token;
        } identifier;
        struct Declaretion {
            Type* type;
            char* name;         
            struct AstNode* value; // AST_EXPRESSION_STATEMENT // CAN BE NULL
            struct AstNode* next; // CAN BE NULL
        } declaration;
        struct FunctionDeclaration {
            Type* return_type;
            char* name;          
            struct AstNode* args;      
            struct AstNode* body; // BlockStatment
            struct AstNode* next; // CAN BE NULL
        } function_declaration;   
        struct IfStatement {
            struct AstNode* condition;
            struct AstNode* body; // BlockStatment
            struct AstNode* else_block; // NOT IMPLEMENTED
            struct AstNode* next;
        } if_statement;
        struct ForStatement {
            struct AstNode* initial;
            struct AstNode* condition;
            struct AstNode* iteration;
            struct AstNode* body; // BlockStatment
            struct AstNode* next;
        } for_statement;
        struct WhileStatement {
            struct AstNode* condition;
            struct AstNode* body; // BlockStatment
            struct AstNode* next;
        } while_statement;
        struct ReturnStatement {
            struct AstNode* expression;
            struct AstNode* next; // Can be NULL
        } return_statement;
        struct BlockStatment {
            struct AstNode* statements; // Can be NULL
            struct AstNode* next; // Can be NULL
        } block_statement;
        struct ExpressionStatement {
            Type* type;
            struct AstNode* expression; // Can be NULL
            struct AstNode* next; // Can be NULL
        } expression_statement;
        struct StructDeclaration {
            char* name;
            struct AstNode* body; // BlockStatment
            struct AstNode* next; // Can be NULL
        } struct_declaration;
        struct ExternStatement {
            struct AstNode* body; // BlockStatment / declaration / fn_declaration in global scope
            struct AstNode* next; // Can be NULL
        } extern_statement;
    };
} AstNode;

int get_binding_power(Token opp);
int is_opp(Token k);
int is_unary(Token k);

AstNode* parse_expr_statement(Lexer* lexer);
AstNode* parse_expr(Lexer* lexer, int curr_bp);
AstNode* parse_decl(Lexer* lexer);
AstNode* parse_func_decl(Lexer* lexer);
AstNode* parse_program(Lexer* lexer);
AstNode* parse_arg_decl(Lexer* lexer);
AstNode* parse_args(Lexer* lexer);
AstNode* parse_statements(Lexer* lexer);
AstNode* parse_statement(Lexer* lexer);
AstNode* parse_function_call(Lexer* lexer,Token ident);
AstNode* parse_unary(Lexer* lexer, Token opp);
TypeInfo parse_type_info(Lexer* lexer);
Type* parse_type(Lexer* lexer);

AstNode* Ast_make_number(Token number);
AstNode* Ast_make_ident(Token ident);
AstNode* AST_make_binary(AstNode* left, Token opp, AstNode* right);

#endif
