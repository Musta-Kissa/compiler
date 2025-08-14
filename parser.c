#include "parser.h"
#include "lexer.h"

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}
#define ASSERT(expr, fmt, ...) { \
    if (!expr) { \
    printf(fmt "\n", ##__VA_ARGS__); \
        exit(-1); \
    } \
}

AstExpr* AST_make_binary(AstExpr* left, Token opp, AstExpr* right) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
    node->type = AST_BINARY_OPERATION;
    
    node->binary_operation.opp_token    = opp;
    node->binary_operation.left         = left;
    node->binary_operation.right        = right;
    return node;
}
AstExpr* Ast_make_number(Token number) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
    node->type = AST_NUMBER;
    node->number.token = number;
    return node;
}
AstExpr* Ast_make_ident(Token ident) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
    node->type = AST_IDENTIFIER;
    node->identifier.token = ident;
    return node;
}
AstExpr* Ast_make_unary(Token opp, AstExpr* right) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
    node->type = AST_UNARY_OPERATION;
    node->unary_operation.opp_token = opp;
    node->unary_operation.right = right;
    return node;
}

int get_binding_power(Token opp) {
    switch(opp.kind){
        case LESS_THEN:         return 2;
        case MORE_THEN:         return 2;
        case NOT_EQUAL:         return 2;
        case LESS_EQUAL:        return 2;
        case MORE_EQUAL:        return 2;
        case EQUAL:             return 2;

        case PLUS:          return 3;
        case MINUS:             return 3;
        case MULTIPLICATION:    return 4;
        case DIVITION:          return 4;

        case PLUS_PLUS:         return 5;
        case MINUS_MINUS:       return 5;

        case NOT:               return 6;
        case SUBSCRIPT_OPEN:    return 7;
        case DOT:               return 8;

    }
}
int is_type(Token k) {
    switch(k.kind) {
        case INT:
        case FLOAT:
            return 1;
        default:
            return 0;
    }
}

int is_unary(Token k) {
    switch(k.kind) {
        case NOT:
        case MINUS:
        case PLUS_PLUS:
        case MINUS_MINUS:
            return 1;
        default: 
            return 0;
    }
}

int is_opp(Token k) {
    switch(k.kind) {
        case DOT: 
        case SUBSCRIPT_OPEN: 
        case LESS_THEN: 
        case MORE_THEN: 
        case MULTIPLICATION:
        case PLUS:
        case DIVITION:
        case MINUS:
        case EQUAL:
        case NOT_EQUAL:
        case LESS_EQUAL:
        case MORE_EQUAL:
            return 1;
        default:
            return 0;
    }
}

AstExpr* parse_args(Lexer* lexer) {
    AstExpr* arg_node = (AstExpr*)malloc(sizeof(AstExpr));
        arg_node ->type = AST_ARGUMENT;
        arg_node ->argument.value = parse_expr(lexer,0);

    Token next = Lexer_next(lexer);
    switch(next.kind) {
        case CLOSE_PARENT:
            arg_node->argument.next = NULL;
            return arg_node;
        case COMMA:
            arg_node ->argument.next = parse_args(lexer);
            return arg_node;
        default:
            PANIC("%s %d: expected COMMA or CLOSE_PARENT after expr in function call, got: %s",__FILE__,__LINE__,format_enum(next));
    }
}

AstExpr* parse_function_call(Lexer* lexer,Token ident) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
    node->type = AST_FUNC_CALL;
    if( Lexer_peek(lexer).kind == CLOSE_PARENT) { // EMPTY FUNCTION CALL
        Lexer_next(lexer);
        node->func_call.identifier = ident;
        node->func_call.args = NULL;
    } else {
        node->func_call.identifier = ident;
        node->func_call.args = parse_args(lexer);
    }
    return node;
}

AstExpr* parse_unary(Lexer* lexer, Token opp) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
    node->type = AST_UNARY_OPERATION;
    node->unary_operation.opp_token = opp;
    Token next = Lexer_next(lexer);    

    switch( next.kind ) {
        case OPEN_PARENT:
            node->unary_operation.right = parse_expr(lexer,0);
            ASSERT(Lexer_next(lexer).kind == CLOSE_PARENT, 
                    "%s %d: expected close CLOSE_PARENT got: %s", __FILE__,__LINE__,format_enum(Lexer_peek_back(lexer)));
            return node;
        case IDENT: 
            if( Lexer_peek(lexer).kind == OPEN_PARENT ) { // FUNC_CALL
                Lexer_next(lexer);
                node->unary_operation.right = parse_function_call(lexer,next);
            } else {
                node->unary_operation.right = Ast_make_ident(next);
            }
            return node;
        case NUMBER:
            node->unary_operation.right = Ast_make_number(next);
            return node;
        default:
            PANIC("%s %d: expected IDENT or NUMBER after UNARY_OPERATION (%s), got: %s",
                  __FILE__,
                  __LINE__,
                  format_enum(opp),
                  format_enum(next));
    }
}

// 2 = unary; 1 = terminal; 0 = open_parent
int parse_leaf(Lexer* lexer,AstExpr** left) {
    Token t = Lexer_peek(lexer);

    if( is_unary(t) ){
        *left = NULL;
        return 2;
    }
    Lexer_next(lexer);
    AstExpr* leaf = (AstExpr*)malloc(sizeof(AstExpr));

    switch(t.kind) {
        case IDENT:
            if( Lexer_peek(lexer).kind == OPEN_PARENT ) {
                //FunctionCall
                Lexer_next(lexer); 
                *left = parse_function_call(lexer,t);
                return 1;
            } else {
                leaf->type = AST_IDENTIFIER;
                leaf->identifier.token = t;
                *left = leaf;
                return 1;
            }
        case NUMBER:
            leaf->type = AST_NUMBER;
            leaf->number.token = t;
            *left = leaf;
            return 1;
        case STRING:
            leaf->type = AST_STRING;
            leaf->number.token = t;
            *left = leaf;
            return 1;
        case OPEN_PARENT:
            *left = leaf;
            return 0;

        default:
            PANIC("%s %d: expected IDENT or NUMBER or STRING after %s, got: %s",
                  __FILE__,
                  __LINE__,
                  format_enum(Lexer_peek_back(lexer)),
                  format_enum(t));
    };
}

// recursion good when bp rising 
// loop good when bp lowering
// a < b + c * d + e;

AstExpr* parse_incrising_bp(Lexer* lexer, AstExpr* left, int min_bp) {
    Token next = Lexer_peek(lexer);

    if( next.kind == CLOSE_PARENT  || next.kind == SUBSCRIPT_CLOSE ) {
        return NULL; //PRETEND EOF
    }
    if( !is_opp(next) && !is_unary(next)) { // EOF
        ASSERT((next.kind == SEMICOLON || next.kind == COMMA || next.kind == OPEN_CURRLY_PARENT || next.kind == CLOSE_PARENT), 
                "%s %d: expected SEMICOLON, COMMA , CLOSE_PARENT or OPEN_CURRLY_PARENT, got %s, lexer idx: %d", __FILE__, __LINE__, format_enum(next), lexer->idx);
        return NULL;
    }

    int next_bp = get_binding_power(next);
    if( next_bp <= min_bp ) {
        return NULL; // Pretend EOF
    } else {
        Lexer_next(lexer);
        AstExpr* right;
        if( next.kind == SUBSCRIPT_OPEN) {
            right = parse_expr(lexer,0);
            ASSERT(Lexer_next(lexer).kind == SUBSCRIPT_CLOSE, 
                    "%s %d: expected close CLOSE_PARENT got: %s", __FILE__, __LINE__, format_enum(Lexer_peek_back(lexer)));
        } else {
            right = parse_expr(lexer,next_bp);
        }
        if( left == NULL ) {
            return Ast_make_unary(next, right);
        } else {
            return AST_make_binary(left,next,right);
        }
    }
    
}
AstExpr* parse_expr(Lexer* lexer, int min_bp) {
    if( Lexer_peek(lexer).kind == SEMICOLON ) { //EMPTY EXPR
        ASSERT(min_bp == 0, "%s %d: expected SEMICOLON should be at the begginig of the expr",__FILE__,__LINE__);
        return NULL;
    }

    AstExpr* left;
    int leaf_return = parse_leaf(lexer,&left);
    if( leaf_return == 0 ) // OPENING PARENT
    { 
        left = parse_expr(lexer,0);
        ASSERT(Lexer_next(lexer).kind == CLOSE_PARENT, 
                "%s %d: expected close CLOSE_PARENT got: %s", __FILE__,__LINE__,format_enum(Lexer_peek_back(lexer)));
    }
    while(true) {
        AstExpr* node;
        node = parse_incrising_bp(lexer,left,min_bp);
        if( node == NULL ) {
            return left;
        } 
        left = node;
    }
}

AstExpr* parse_decl(Lexer* lexer,Token type, Token ident) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_DECLARATION;
        node->declaration.type = type;
        node->declaration.name = ident;
    Token next = Lexer_next(lexer);
    if( next.kind == SEMICOLON ) {
        node->declaration.value = NULL;
    } else if( next.kind == ASSIGN ) {
        node->declaration.value = parse_expr(lexer,0);
        ASSERT( (Lexer_next(lexer).kind == SEMICOLON ), "%s %d: Expected SIMICOL after assigment expr, got %s",format_enum(Lexer_peek_back(lexer)));
    } else {
        PANIC("%s %d: expected SEMICOLON or ASSIGN, got %s",__FILE__,__LINE__,format_enum(next));
    }
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON), "%s %d: Expected SEMICOLON, got %s, lexer idx:%d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    return node;
    
}

AstExpr* parse_arg_decl(Lexer* lexer) {
    AstExpr* arg_node = (AstExpr*)malloc(sizeof(AstExpr));
        arg_node->type = AST_ARGUMENT_DECLARATION;
        arg_node->argument_decl.type = Lexer_next(lexer);
    ASSERT( is_type(arg_node->argument_decl.type) ,"%s %d: expected TYPE for arg decl, got %s",__FILE__,__LINE__,format_enum(arg_node->argument_decl.type) );

        arg_node->argument_decl.ident = Lexer_next(lexer);
    ASSERT( arg_node->argument_decl.ident.kind == IDENT ,"%s %d: expected TYPE for arg decl, got %s",__FILE__,__LINE__,format_enum(arg_node->argument_decl.ident) );

    Token next = Lexer_next(lexer);
    switch(next.kind) {
        case CLOSE_PARENT:
            arg_node->argument_decl.next = NULL;
            return arg_node;
        case COMMA:
            arg_node ->argument_decl.next = parse_arg_decl(lexer);
            return arg_node;
        default:
            PANIC("%s %d: expected COMMA or CLOSE_PARENT after ARG_DECL in FUNC_DECL, got: %s",__FILE__,__LINE__,format_enum(next));
    }
}

AstExpr* parse_compound_statement(Lexer* lexer) {
    ASSERT( (Lexer_next(lexer).kind == OPEN_CURRLY_PARENT) ,"%s %d: expected OPEN_CURRLY_PARENT",__FILE__,__LINE__);
    
    if( Lexer_peek(lexer).kind == CLOSE_CURRLY_PARENT) { // EMPTY STATEMENT
        return NULL;
    } else {
        AstExpr* tmp = parse_statement(lexer);
        ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after compound statement, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
        return tmp;
    }
}

AstExpr* parse_func_decl(Lexer* lexer,Token type,Token ident) {
    AstExpr* func_decl_node = (AstExpr*)malloc(sizeof(AstExpr));
        func_decl_node->type = AST_FUNCTION_DECLARATION;
        func_decl_node->function_declaration.return_type= type;
        func_decl_node->function_declaration.name = ident;

    if( Lexer_peek(lexer).kind == CLOSE_PARENT) { // NO ARGS
        func_decl_node->function_declaration.args = NULL;
        Lexer_next(lexer);
    } else {
        func_decl_node->function_declaration.args = parse_arg_decl(lexer);
    }
    ASSERT( (Lexer_peek(lexer).kind == OPEN_CURRLY_PARENT) , "%s %d: expected '{' after function decl ,got %s",__FILE__,__LINE__,format_enum(Lexer_peek(lexer)));
    func_decl_node->function_declaration.body = parse_compound_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after func body, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    return func_decl_node;
}

AstExpr* parse_if(Lexer* lexer) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_IF_STATEMENT;
    ASSERT( (Lexer_peek(lexer).kind == OPEN_PARENT ), "%s %d: Expected OPEN_PARENT after IF keyword",__FILE__,__LINE__);
    node->if_statement.condition = parse_expr(lexer,0);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_PARENT ), "%s %d: Expected CLOSE_PARENT after IF condition, got %s",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)));


    node->if_statement.body = parse_compound_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after func body, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    //Token next = Lexer_peek(lexer);
    //if( next.kind == ELSE ) {
        //Lexer_next(lexer);
    //}
    return node;
}

AstExpr* parse_for(Lexer* lexer) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_FOR_STATEMENT;
    ASSERT( (Lexer_next(lexer).kind == OPEN_PARENT), "%s %d: Expected OPEN_PARENT after FOR keyword",__FILE__,__LINE__);

    Token type = Lexer_next(lexer);
        ASSERT( is_type(type) , "%s %d: expected type name, got %s",__FILE__,__LINE__,format_enum(type));
    Token ident = Lexer_next(lexer);
        ASSERT( (ident.kind == IDENT) , "%s %d: expected ident ",__FILE__,__LINE__);

    node->for_statement.initial = parse_decl(lexer,type,ident);
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON ), "%s %d: Expected SEMICOLON after IF init expr",__FILE__,__LINE__);

    node->for_statement.condition = parse_expr(lexer,0);
    ASSERT( (Lexer_next(lexer).kind == SEMICOLON ), "%s %d: Expected SEMICOLON after IF condition expr",__FILE__,__LINE__);

    node->for_statement.iteration = parse_expr(lexer,0);
    ASSERT( (Lexer_next(lexer).kind == CLOSE_PARENT ), "%s %d: Expected SEMICOLON after IF iteration expr",__FILE__,__LINE__);

    node->for_statement.body = parse_compound_statement(lexer);
    return node;
}

AstExpr* parse_while(Lexer* lexer) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_WHILE_STATEMENT;
    ASSERT( (Lexer_peek(lexer).kind == OPEN_PARENT), "%s %d: Expected OPEN_PARENT after WHILE keyword",__FILE__,__LINE__);
    node->while_statement.condition = parse_expr(lexer,0);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_PARENT ), "%s %d: Expected CLOSE_PARENT after WHILE condition iteration expr, got %s",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)));

    node->while_statement.body = parse_compound_statement(lexer);
    return node;
}
AstExpr* parse_return(Lexer* lexer) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_RETURN_STATEMENT;
        node->return_statement.expression = parse_expr(lexer,0);
    Lexer_next(lexer);
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON ), "%s %d: Expected SEMICOLON after return expr , got %s",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)));
    return node;
}

AstExpr* parse_statement(Lexer* lexer) {
    Token next = Lexer_next(lexer);
    if( next.kind == EOF_TOKEN || next.kind == CLOSE_CURRLY_PARENT ) 
        return NULL;

    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));

    switch( next.kind ) {
        case IF:
            node = parse_if(lexer);
            node->if_statement.next = parse_statement(lexer);
            return node;
        case FOR:
            node = parse_for(lexer);
            node->for_statement.next = parse_statement(lexer);
            return node;
        case WHILE:
            node = parse_while(lexer);
            node->while_statement.next = parse_statement(lexer);
            return node;
        case RETURN:
            node = parse_return(lexer);
            node->return_statement.next = parse_statement(lexer);
            return node;
    }

    // parse funct / var decl

    Token type = next;
        ASSERT( is_type(type) , "%s %d: expected type name, got %s",__FILE__,__LINE__,format_enum(type));
    Token ident = Lexer_next(lexer);
        ASSERT( (ident.kind == IDENT) , "%s %d: expected func ident ",__FILE__,__LINE__);

    next = Lexer_peek(lexer);
    if( next.kind == OPEN_PARENT ) { //FUNC_DECL
        Lexer_next(lexer);
        node = parse_func_decl(lexer,type,ident);
        node->function_declaration.next = parse_statement(lexer);
        if( node->function_declaration.next == NULL) {
            ASSERT( (Lexer_curr(lexer).kind == EOF_TOKEN || Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT), "%s %d: Expected EOF_TOKEN or CLOSE_CURRLY_PARENT because statement.next == NULL, got %s, lexer idx:%d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
        }
        return node;
    } else if ( next.kind ==  ASSIGN || next.kind == SEMICOLON ) {
        node = parse_decl(lexer,type,ident);
        node->declaration.next = parse_statement(lexer);
        if( node->declaration.next == NULL) {
            ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT), "%s %d: Expected CLOSE_CURRLY_PARENT, got %s, lexer idx:%d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
        }
        return node;
    } else {
        PANIC("%s %d: expected OPEN_PARENT, got %s",__FILE__,__LINE__,format_enum(next));
    }
}

AstExpr* parse_program(Lexer* lexer) {
    AstExpr* ast = parse_statement(lexer);
    return ast;
}
