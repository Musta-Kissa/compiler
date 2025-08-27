//==================================
// Every statement should return with the last token as Lexer_curr():
// after: 
//      parse_while -> lexer_curr() == CLOSE_CURRLY_PARENT
//      parse_decl  -> lexer_curr() == SEMICOLON
//      parse_expr  -> lexer_curr() == {last ident / opp} NOT semicolon thats not part of expr
//      .... and so on
//
//==================================

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
        case ASSIGN:            return 1;
        case LESS_THEN:         return 2;
        case MORE_THEN:         return 2;
        case NOT_EQUAL:         return 2;
        case LESS_EQUAL:        return 2;
        case MORE_EQUAL:        return 2;
        case EQUAL:             return 2;

        case PLUS:              return 3;
        case MINUS:             return 3;
        case STAR:              return 4;
        case DIVITION:          return 4;

        case PLUS_PLUS:         return 5;
        case MINUS_MINUS:       return 5;

        case NOT:               return 6;
        case SUBSCRIPT_OPEN:    return 7;
        case DOT:               return 8;
        default:
            PANIC("BINDING POWER NOT SUPPORTED");
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
        case STAR:
        case PLUS:
        case DIVITION:
        case MINUS:
        case EQUAL:
        case NOT_EQUAL:
        case LESS_EQUAL:
        case MORE_EQUAL:
        case ASSIGN:
            return 1;
        default:
            return 0;
    }
}

AstExpr* parse_args(Lexer* lexer) {
    AstExpr* arg_node = (AstExpr*)malloc(sizeof(AstExpr));
        arg_node ->type = AST_ARGUMENT;
        arg_node ->argument.value = parse_expr_statement(lexer);

    Token curr = Lexer_curr(lexer);
    switch(curr.kind) {
        case CLOSE_PARENT:
            arg_node->argument.next = NULL;
            return arg_node;
        case COMMA:
            arg_node ->argument.next = parse_args(lexer);
            return arg_node;
        default:
            PANIC("%s %d: expected COMMA or CLOSE_PARENT after expr in function call, got: %s:%s",__FILE__,__LINE__,format_enum(curr),curr.value);
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
            Lexer_next(lexer); // CONSUME SUBSCRIPT_CLOSE
            ASSERT(Lexer_curr(lexer).kind == SUBSCRIPT_CLOSE, 
                    "%s %d: expected close CLOSE_PARENT got: %s", __FILE__, __LINE__, format_enum(Lexer_peek_back(lexer)));
        } else {
            right = parse_expr(lexer,next_bp);
        }
        if( left == NULL ) {
            return Ast_make_unary(next, right);
        } else {
            ASSERT( (!is_unary(next) || next.kind == MINUS ), "%s %d: attempted to add unary opp to binary node: (%s)",__FILE__,__LINE__,format_enum(next));
            return AST_make_binary(left,next,right);
        }
    }
    
}
AstExpr* parse_expr(Lexer* lexer, int min_bp) {
    if( Lexer_peek(lexer).kind == SEMICOLON ) { //EMPTY EXPR
        ASSERT(min_bp == 0, "%s %d: expected SEMICOLON to be at the begginig of the expr",__FILE__,__LINE__);
        return NULL;
    }

    AstExpr* left;
    int leaf_return = parse_leaf(lexer,&left);
    if( leaf_return == 0 ) // OPENING PARENT
    { 
        left = parse_expr(lexer,0);
        Lexer_next(lexer); // CONSUME CLOSE_PARENT
        ASSERT(Lexer_curr(lexer).kind == CLOSE_PARENT, 
                "%s %d: expected close CLOSE_PARENT got: %s", __FILE__,__LINE__,format_enum(Lexer_curr(lexer)));
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

//Consume whole decl WITH SEMICOLON -> after call Lexer_curr() == SEMICOLON
// examples:  
//  banana : int = 5; 
//  banana : int;
//  banana := 5;     
AstExpr* parse_decl(Lexer* lexer) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_DECLARATION;
    Token ident = Lexer_next(lexer);
        node->declaration.name = ident.value;
        node->declaration.star_number = 0;

    Lexer_next(lexer); // Consume colon
    ASSERT( (Lexer_curr(lexer).kind == COLON ), "%s %d: Expected COLON after type in variable decl, got %s",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)));

    switch( Lexer_peek(lexer).kind ) {
        case ASSIGN:
            node->declaration.type_name  = NULL;
            Lexer_next(lexer); // Consume Assign
            node->declaration.value = parse_expr_statement(lexer);
            break;
        case STAR:
            while( Lexer_peek(lexer).kind == STAR ) {
                node->declaration.star_number += 1;
                Lexer_next(lexer);
            }
            // fall-through
        case IDENT:
            node->declaration.type_name  = Lexer_next(lexer).value;
            if( Lexer_peek(lexer).kind == ASSIGN ) { // Value given
                Lexer_next(lexer); // Consume Assign
                node->declaration.value = parse_expr_statement(lexer);
            } else // No value given
            if( Lexer_peek(lexer).kind == SEMICOLON ) { 
                node->declaration.value = parse_expr_statement(lexer); // empty expression
            } else {
                PANIC("%s %d: Expected ASSIGN or SEMICOLON after TYPE in declaration, got %s",__FILE__,__LINE__,format_enum(Lexer_peek(lexer)));
            }
            break;
        default:
            PANIC("%s %d: Expected TYPE in declaration after COLON, got %s",__FILE__,__LINE__,format_enum(Lexer_peek(lexer)));
    }
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON), "%s %d: Expected SEMICOLON, got %s, lexer idx:%d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    return node;
}

AstExpr* parse_arg_decl(Lexer* lexer) {
    AstExpr* arg_node = (AstExpr*)malloc(sizeof(AstExpr));
        arg_node->type = AST_ARGUMENT_DECLARATION;
        arg_node->argument_decl.star_number = 0;

    while( Lexer_peek(lexer).kind == STAR) {
        arg_node->argument_decl.star_number += 1;
        Lexer_next(lexer);
    }
        arg_node->argument_decl.type_name = Lexer_next(lexer).value;
    //ASSERT( is_type(arg_node->argument_decl.type) ,"%s %d: expected TYPE for arg decl, got %s",__FILE__,__LINE__,format_enum(arg_node->argument_decl.type) );
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

// expects Lexer_curr() == OPEN_CURRLY_PARENT
// consumes whole block including ending CLOSE_CURRLY_PARENT '}'
// doesnt set block_statement.next
AstExpr* parse_block_statement(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME OPEN_CURRLY_PARENT 
    ASSERT( (Lexer_curr(lexer).kind == OPEN_CURRLY_PARENT) ,"%s %d: expected OPEN_CURRLY_PARENT",__FILE__,__LINE__);

    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_BLOCK_STATEMENT;
        node->block_statement.statements = parse_statements(lexer);
    Lexer_next(lexer); // CONSUME CLOSE_CURRLY_PARENT
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) ,"%s %d: expected CLOSE_CURRLY_PARENT, got %s, lexer idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    return node;
}

AstExpr* parse_func_decl(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME FN 
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_FUNCTION_DECLARATION;
        node->function_declaration.star_number = 0;

    Token ident = Lexer_next(lexer);
        node->function_declaration.name = ident.value;
    ASSERT( (Lexer_curr(lexer).kind == IDENT) , "%s %d: expected fn IDENT, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);

    Lexer_next(lexer); // CONSUME OPEN_PARENT
    ASSERT( (Lexer_curr(lexer).kind == OPEN_PARENT) , "%s %d: expected OPEN_PARENT after fn IDENT, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);

    if( Lexer_peek(lexer).kind == CLOSE_PARENT) { // NO ARGS
        node->function_declaration.args = NULL;
        Lexer_next(lexer); // CONSUME CLOSE_PARENT
    } else {
        node->function_declaration.args = parse_arg_decl(lexer);
        ASSERT( (Lexer_curr(lexer).kind == CLOSE_PARENT) , "%s %d: expected CLOSE_PARENT, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    }

    if( Lexer_peek(lexer).kind == ARROW ) {
        Lexer_next(lexer);
        while( Lexer_peek(lexer).kind == STAR ) {
            node->function_declaration.star_number += 1;
            Lexer_next(lexer);
        }
        Token return_type_name = Lexer_next(lexer);
        //ASSERT( (is_type(return_type)) , "%s %d: expected type name after '->', got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
        node->function_declaration.return_type_name = return_type_name.value;
    } else {
        node->function_declaration.return_type_name = "void";
    }
    ASSERT( (Lexer_peek(lexer).kind == OPEN_CURRLY_PARENT) , "%s %d: expected '{', got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    node->function_declaration.body = parse_block_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after if_statement body, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    return node;
}

AstExpr* parse_for(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME FOR
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_FOR_STATEMENT;
    Lexer_next(lexer); // consume the opent paret so it will not be interpreted by the as part of the parse expr 
    ASSERT( (Lexer_curr(lexer).kind == OPEN_PARENT), "%s %d: Expected OPEN_PARENT after FOR keyword",__FILE__,__LINE__);

    node->for_statement.initial = parse_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON ), "%s %d: Expected SEMICOLON after IF init expr, got %s",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)));

    node->for_statement.condition = parse_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON ), "%s %d: Expected SEMICOLON after IF condition expr",__FILE__,__LINE__);

    node->for_statement.iteration = parse_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_PARENT ), "%s %d: Expected SEMICOLON after IF iteration expr",__FILE__,__LINE__);

    ASSERT( (Lexer_peek(lexer).kind == OPEN_CURRLY_PARENT) , "%s %d: expected '{', got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    node->for_statement.body = parse_block_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after if_statement body, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    return node;
}

AstExpr* parse_while(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME WHILE
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_WHILE_STATEMENT;
    Lexer_next(lexer); // consume the opent paret so it will not be interpreted by the as part of the parse expr 
    ASSERT( (Lexer_curr(lexer).kind == OPEN_PARENT), "%s %d: Expected OPEN_PARENT after WHILE keyword",__FILE__,__LINE__);
    node->while_statement.condition = parse_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_PARENT ), "%s %d: Expected CLOSE_PARENT after WHILE condition iteration expr, got %s",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)));

    ASSERT( (Lexer_peek(lexer).kind == OPEN_CURRLY_PARENT) , "%s %d: expected '{', got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    node->while_statement.body = parse_block_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after if_statement body, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    return node;
}
AstExpr* parse_return(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME RETURN
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_RETURN_STATEMENT;
        node->return_statement.expression = parse_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON ), "%s %d: Expected SEMICOLON after return expr , got %s",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)));
    return node;
}
AstExpr* parse_if(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME IF
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_IF_STATEMENT;
    Lexer_next(lexer); // consume the opent paret so it will not be interpreted by the as part of the parse expr 
    ASSERT( (Lexer_curr(lexer).kind == OPEN_PARENT ), "%s %d: Expected OPEN_PARENT after IF keyword",__FILE__,__LINE__);
    node->if_statement.condition = parse_expr_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_PARENT ), "%s %d: Expected CLOSE_PARENT after IF condition, got %s",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)));


    ASSERT( (Lexer_peek(lexer).kind == OPEN_CURRLY_PARENT) , "%s %d: expected '{', got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    node->if_statement.body = parse_block_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after if_statement body, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
    return node;
}

/// Consumes ending SEMICOLON
AstExpr* parse_expr_statement(Lexer* lexer) {
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_EXPRESSION_STATEMENT;
        //node->expression_statement.type_name = NULL;
        node->expression_statement.value = parse_expr(lexer,0);
    Token next = Lexer_next(lexer); // CONSUME (SEMICOLON) or (CLOSE_PARENT if in a for loop)
    ASSERT( (next.kind == SEMICOLON || next.kind == CLOSE_PARENT || next.kind == COMMA ), "%s %d: Expected SEMICOLON, CLOSE_PARENT or COMMA after expr statement , got %s",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)));
    return node;
}
AstExpr* parse_struct_decl(Lexer* lexer) {
    Lexer_next(lexer); // Consume STRUCT
    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));
        node->type = AST_STRUCT_DECLARATION;
        node->struct_declaration.name = Lexer_next(lexer).value;
    ASSERT( (Lexer_curr(lexer).kind == IDENT), "%s %d: Expected IDENT after STRUCT keyword",__FILE__,__LINE__);
        node->struct_declaration.body = parse_block_statement(lexer);
    return node;
}

AstExpr* parse_statement(Lexer* lexer) {
    Token next = Lexer_peek(lexer);
    if( next.kind == EOF_TOKEN || next.kind == CLOSE_CURRLY_PARENT) // The caller must consume the CLOSE_CURRLY_PARENT
        return NULL;
    if( next.kind == SEMICOLON || next.kind == CLOSE_PARENT ) { // We are in an empty for loop statement
        Lexer_next(lexer);
        return NULL;
    }

    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));

    switch( next.kind ) {
        case IF:
            node = parse_if(lexer);
            node->if_statement.next = NULL;
            return node;
        case FOR:
            node = parse_for(lexer);
            node->for_statement.next = NULL;
            return node;
        case WHILE:
            node = parse_while(lexer);
            node->while_statement.next = NULL;
            return node;
        case RETURN:
            node = parse_return(lexer);
            node->return_statement.next = NULL;
            return node;
        case OPEN_CURRLY_PARENT:
            node = parse_block_statement(lexer);
            ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after block statement, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
            node->block_statement.next = NULL;
            return node;
        case FN:
            node = parse_func_decl(lexer);
            node->function_declaration.next = NULL;
            return node;
        case STRUCT:
            node = parse_struct_decl(lexer);
            node->struct_declaration.next = NULL; 
            return node;
    }
    // expected identifier than if ':' its a declaration if not an expression;
    ASSERT( (next.kind == IDENT || next.kind == OPEN_PARENT || is_unary(next)) ,"expected KEYWORD,UNARY_OPP,OPEN_CURRLY_PARENT, OPEN_PARENT or IDENT got %s, idx: %d",format_enum(next),lexer->idx);

    if( Lexer_peek_n(lexer,2).kind == COLON ) {
        node = parse_decl(lexer);
        node->declaration.next = NULL;
        return node;
    } else { // EXPR
        node = parse_expr_statement(lexer);
        node->expression_statement.next = NULL;
        return node;
    }
}

// expects Lexer_next() == OPEN_CURRLY_PARENT | {KEYWORD} | {DECL}
// consumes whole statement with ; and } 
AstExpr* parse_statements(Lexer* lexer) {
    Token next = Lexer_peek(lexer);
    if( next.kind == EOF_TOKEN || next.kind == CLOSE_CURRLY_PARENT ) 
        return NULL;

    AstExpr* node = (AstExpr*)malloc(sizeof(AstExpr));

    switch( next.kind ) {
        case IF:
            node = parse_if(lexer);
            node->if_statement.next = parse_statements(lexer);
            return node;
        case FOR:
            node = parse_for(lexer);
            node->for_statement.next = parse_statements(lexer);
            return node;
        case WHILE:
            node = parse_while(lexer);
            node->while_statement.next = parse_statements(lexer);
            return node;
        case RETURN:
            node = parse_return(lexer);
            node->return_statement.next = parse_statements(lexer);
            return node;
        case OPEN_CURRLY_PARENT:
            node = parse_block_statement(lexer);
            ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after block statement, got %s, idx: %d",__FILE__,__LINE__,format_enum(Lexer_curr(lexer)),lexer->idx);
            node->block_statement.next = parse_statements(lexer);
            return node;
        case FN:
            node = parse_func_decl(lexer);
            node->function_declaration.next = parse_statements(lexer);
            return node;
        case STRUCT:
            node = parse_struct_decl(lexer);
            node->struct_declaration.next = parse_statements(lexer);
            return node;
    }
    // expected identifier than if ':' its a declaration if not an expression;
    ASSERT( (next.kind == IDENT) ,"expected KEYWORK, OPEN_CURRLY_PARENT or IDENT got %s",format_enum(next));

    if( Lexer_peek_n(lexer,2).kind == COLON ) {
        node = parse_decl(lexer);
        node->declaration.next = parse_statements(lexer);
        return node;
    } else { // EXPR
        node = parse_expr_statement(lexer);
        node->expression_statement.next = parse_statements(lexer);
        return node;
    }
}

AstExpr* parse_program(Lexer* lexer) {
    AstExpr* ast = parse_statements(lexer);
    return ast;
}
