//==================================
// Every statement should return with the last token as Lexer_curr():
// after: 
//      parse_while -> lexer_curr() == CLOSE_CURRLY_PARENT
//      parse_decl  -> lexer_curr() == SEMICOLON
//      parse_expr  -> lexer_curr() == {last ident / opp} NOT semicolon thats not part of expr
//      .... and so on
//
//==================================

#include "panic_macros.h"
#include "parser.h"
#include "my_string.h"
#include "lexer.h"

AstNode* AST_make_binary(AstNode* left, Token opp, AstNode* right) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_BINARY_OPERATION;
    
    node->binary_operation.opp_token    = opp;
    node->binary_operation.left         = left;
    node->binary_operation.right        = right;
    return node;
}
/*
AstNode* Ast_make_number(Token number) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_NUMBER;
    node->number.token = number;
    return node;
}
*/
AstNode* Ast_make_ident(Token ident) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_IDENTIFIER;
    node->identifier.token = ident;
    return node;
}
AstNode* Ast_make_unary(Token opp, AstNode* right) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_UNARY_OPERATION;
    node->unary_operation.opp_token = opp;
    node->unary_operation.right = right;
    return node;
}

int get_binding_power(Token opp) {
    switch(opp.kind){
        case ASSIGN:            return 1;

        case LOGIC_OR:          return 2;
        case LOGIC_AND:         return 3;

        case LESS_THEN:         return 4;
        case MORE_THEN:         return 4;
        case NOT_EQUAL:         return 4;
        case LESS_EQUAL:        return 4;
        case MORE_EQUAL:        return 4;
        case EQUAL:             return 4;

        case PLUS:              return 5;
        case MINUS:             return 5;
        case STAR:              return 6; // posible problem when dereferencing
        case DIVITION:          return 6;

        case PLUS_PLUS:         return 7;
        case MINUS_MINUS:       return 7;

        case NOT:               return 8;
        case AMPERSAND:         return 8;

        case SUBSCRIPT_OPEN:    return 9;
        case DOT:               return 10;

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
        case AMPERSAND:
        case STAR:
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
        
        case LOGIC_OR:
        case LOGIC_AND:
            return 1;
        default:
            return 0;
    }
}

// consumes the whole function call
AstNode* parse_args(Lexer* lexer) {
    AstNode* arg_node = (AstNode*)malloc(sizeof(AstNode));
        arg_node ->type = AST_ARGUMENT;
        arg_node ->argument.value = parse_expr_statement(lexer);

    Token curr = Lexer_next(lexer);
    switch(curr.kind) {
        case CLOSE_PARENT:
            arg_node->argument.next = NULL;
            return arg_node;
        case COMMA:
            arg_node ->argument.next = parse_args(lexer);
            return arg_node;
        default:
            PANIC("%s %d: expected COMMA or CLOSE_PARENT after expr in function call, got: %s:%s",__FILE__,__LINE__,format_token(curr),curr.value);
    }
}

AstNode* parse_function_call(Lexer* lexer,Token ident) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_FUNC_CALL;
    if( Lexer_peek(lexer).kind == CLOSE_PARENT) { // EMPTY FUNCTION CALL
        Lexer_next(lexer);
        node->function_call.identifier = ident;
        node->function_call.args = NULL;
    } else {
        node->function_call.identifier = ident;
        node->function_call.args = parse_args(lexer);
    }
    return node;
}

// 2 = unary; 1 = terminal; 0 = open_parent
int parse_leaf(Lexer* lexer,AstNode** left) {
    Token t = Lexer_peek(lexer);

    if( is_unary(t) ){
        *left = NULL;
        return 2;
    }
    Lexer_next(lexer);
    AstNode* leaf = (AstNode*)malloc(sizeof(AstNode));

    switch(t.kind) {
        case FALSE:
            leaf->type = AST_BOOL;
            leaf->ast_bool.value = 0;
            *left = leaf;
            return 1;
        case TRUE:
            leaf->type = AST_BOOL;
            leaf->ast_bool.value = 1;
            *left = leaf;
            return 1;
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
            TODO("Make it use new tokens for FLOAT_LITERAL and INT_LITERAL");
        case INTIGER_LITERAL:
            leaf->type = AST_NUMBER;
            leaf->number.value = t.value;
            leaf->number.type = (Type*)malloc(sizeof(Type));
            *leaf->number.type = (Type){.type_kind=INTIGER_TYPE, .intiger_type.size=BITS_64, .type_name = "int" };
            *left = leaf;
            return 1;
        case FLOAT_LITERAL:
            leaf->type = AST_NUMBER;
            leaf->number.value = t.value;
            leaf->number.type = (Type*)malloc(sizeof(Type));
            *leaf->number.type = (Type){.type_kind=FLOAT_TYPE, .float_type.size=BITS_64, .type_name = "float" };
            *left = leaf;
            return 1;
        case STRING:
            leaf->type = AST_STRING;
            leaf->string.token = t;
            *left = leaf;
            return 1;
        case OPEN_PARENT:
            *left = leaf;
            return 0;
        default:
            PANIC("%s %d: expected IDENT or NUMBER or STRING after %s, got: %s",
                  __FILE__,
                  __LINE__,
                  format_token(Lexer_peek_back(lexer)),
                  format_token(t));
    };
}

// recursion good when bp rising 
// loop good when bp lowering
// a < b + c * d + e;

AstNode* parse_incrising_bp(Lexer* lexer, AstNode* left, int min_bp) {
    Token next = Lexer_peek(lexer);

    if( next.kind == CLOSE_PARENT  || next.kind == SUBSCRIPT_CLOSE ) {
        return NULL; //PRETEND EOF
    }
    if( !is_opp(next) && !is_unary(next)) { // EOF
        // comma,close_parent -> function_call ; semicolon -> any expr; open_curly_parent -> for/while/if statement
        ASSERT((next.kind == SEMICOLON || next.kind == COMMA || next.kind == OPEN_CURRLY_PARENT || next.kind == CLOSE_PARENT), 
                "%s %d: expected SEMICOLON, COMMA , CLOSE_PARENT or OPEN_CURRLY_PARENT, got %s, lexer idx: %d", __FILE__, __LINE__, format_token(next), lexer->idx);
        return NULL;
    }

    int next_bp = get_binding_power(next);
    if( next_bp <= min_bp ) {
        return NULL; // Pretend EOF
    } else {
        Lexer_next(lexer);
        AstNode* right;
        if( next.kind == SUBSCRIPT_OPEN) {
            right = parse_expr(lexer,0);
            Lexer_next(lexer); // CONSUME SUBSCRIPT_CLOSE
            ASSERT(Lexer_curr(lexer).kind == SUBSCRIPT_CLOSE, 
                    "%s %d: expected close CLOSE_PARENT got: %s", __FILE__, __LINE__, format_token(Lexer_peek_back(lexer)));
        } else {
            right = parse_expr(lexer,next_bp);
        }
        if( left == NULL ) {
            return Ast_make_unary(next, right);
        } else {
            ASSERT( (!is_unary(next) || next.kind == MINUS || next.kind == STAR), "%s %d: attempted to add unary opp to binary node: (%s)",__FILE__,__LINE__,format_token(next));
            return AST_make_binary(left,next,right);
        }
    }
    
}
AstNode* parse_expr(Lexer* lexer, int min_bp) {
    if( Lexer_peek(lexer).kind == SEMICOLON ) { //EMPTY EXPR
        ASSERT(min_bp == 0, "%s %d: expected SEMICOLON to be at the begginig of the expr",__FILE__,__LINE__);
        return NULL;
    }

    AstNode* left;
    int leaf_return = parse_leaf(lexer,&left);
    if( leaf_return == 0 ) // OPENING PARENT
    { 
        left = parse_expr(lexer,0);
        Lexer_next(lexer); // CONSUME CLOSE_PARENT
        ASSERT(Lexer_curr(lexer).kind == CLOSE_PARENT, 
                "%s %d: expected close CLOSE_PARENT got: %s", __FILE__,__LINE__,format_token(Lexer_curr(lexer)));
    }
    while(true) {
        AstNode* node;
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
AstNode* parse_decl(Lexer* lexer) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
        node->type = AST_DECLARATION;
    Token ident = Lexer_next(lexer);
        node->declaration.name = ident.value;

    Lexer_next(lexer); // Consume colon
    ASSERT( (Lexer_curr(lexer).kind == COLON ), "%s %d: Expected COLON after type in variable decl, got %s",__FILE__,__LINE__,format_token(Lexer_curr(lexer)));

    switch( Lexer_peek(lexer).kind ) {
        case ASSIGN:
            node->declaration.type = NULL;
            Lexer_next(lexer); // Consume Assign
            node->declaration.value = parse_expr_statement(lexer);
            break;
        case STAR: // *int
        case SUBSCRIPT_OPEN: // []int
        case IDENT: // int
            node->declaration.type = parse_type(lexer);
            if( Lexer_peek(lexer).kind == ASSIGN ) { // Value given
                Lexer_next(lexer); // Consume Assign
                node->declaration.value = parse_expr_statement(lexer);
            } else // No value given
            if( Lexer_peek(lexer).kind == SEMICOLON ) { 
                node->declaration.value = parse_expr_statement(lexer); // empty expression
            } else {
                PANIC("%s %d: Expected ASSIGN or SEMICOLON after TYPE in declaration, got %s",__FILE__,__LINE__,format_token(Lexer_peek(lexer)));
            }
            break;
        default:
            PANIC("%s %d: Expected TYPE in declaration after COLON, got %s",__FILE__,__LINE__,format_token(Lexer_peek(lexer)));
    }
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON), "%s %d: Expected SEMICOLON, got %s, lexer idx:%d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
    return node;
}

AstNode* parse_arg_decl(Lexer* lexer) {
    AstNode* arg_node = (AstNode*)malloc(sizeof(AstNode));
        arg_node->type = AST_ARGUMENT_DECLARATION;
    /*
        arg_node->argument_decl.type_info.star_number = 0;
    while( Lexer_peek(lexer).kind == STAR) {
        arg_node->argument_decl.type_info.star_number += 1;
        Lexer_next(lexer);
    }
    */
    arg_node->argument_decl.type = parse_type(lexer);


    //ASSERT( is_type(arg_node->argument_decl.type) ,"%s %d: expected TYPE for arg decl, got %s",__FILE__,__LINE__,format_token(arg_node->argument_decl.type) );
        arg_node->argument_decl.ident = Lexer_next(lexer).value;
    ASSERT( Lexer_curr(lexer).kind == IDENT ,"%s %d: expected TYPE for arg decl, got %s",__FILE__,__LINE__,format_token(Lexer_curr(lexer)) );

    Token next = Lexer_next(lexer);
    switch(next.kind) {
        case CLOSE_PARENT:
            arg_node->argument_decl.next = NULL;
            return arg_node;
        case COMMA:
            arg_node ->argument_decl.next = parse_arg_decl(lexer);
            return arg_node;
        default:
            PANIC("%s %d: expected COMMA or CLOSE_PARENT after ARG_DECL in FUNC_DECL, got: %s",__FILE__,__LINE__,format_token(next));
    }
}

// expects Lexer_curr() == OPEN_CURRLY_PARENT
// consumes whole block including ending CLOSE_CURRLY_PARENT '}'
// doesnt set block_statement.next
AstNode* parse_block_statement(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME OPEN_CURRLY_PARENT 
    ASSERT( (Lexer_curr(lexer).kind == OPEN_CURRLY_PARENT) ,"%s %d: expected OPEN_CURRLY_PARENT",__FILE__,__LINE__);

    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
        node->type = AST_BLOCK_STATEMENT;
        node->block_statement.statements = parse_statements(lexer);
    Lexer_next(lexer); // CONSUME CLOSE_CURRLY_PARENT
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) ,"%s %d: expected CLOSE_CURRLY_PARENT, got %s, lexer idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
    return node;
}

AstNode* parse_func_decl(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME FN 
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
        node->type = AST_FUNCTION_DECLARATION;
        //node->function_declaration.return_type_info.star_number = 0;

    Token ident = Lexer_next(lexer);
        node->function_declaration.name = ident.value;
    ASSERT( (Lexer_curr(lexer).kind == IDENT) , "%s %d: expected fn IDENT, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);

    Lexer_next(lexer); // CONSUME OPEN_PARENT
    ASSERT( (Lexer_curr(lexer).kind == OPEN_PARENT) , "%s %d: expected OPEN_PARENT after fn IDENT, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);

    if( Lexer_peek(lexer).kind == CLOSE_PARENT) { // NO ARGS
        node->function_declaration.args = NULL;
        Lexer_next(lexer); // CONSUME CLOSE_PARENT
    } else {
        node->function_declaration.args = parse_arg_decl(lexer);
        ASSERT( (Lexer_curr(lexer).kind == CLOSE_PARENT) , "%s %d: expected CLOSE_PARENT, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
    }

    // Explicit return
    if( Lexer_peek(lexer).kind == ARROW ) {
        Lexer_next(lexer);
        node->function_declaration.return_type = parse_type(lexer);
        /*
        while( Lexer_peek(lexer).kind == STAR ) {
            node->function_declaration.return_type_info.star_number += 1;
            Lexer_next(lexer);
        }
        Token return_type_name = Lexer_next(lexer);
        //ASSERT( (is_type(return_type)) , "%s %d: expected type name after '->', got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
        node->function_declaration.return_type_info.type_name = return_type_name.value;
        */

    } else { // Implicid void return
        Type* return_type = (Type*)malloc(sizeof(Type));
        *return_type = (Type){.type_kind=VOID_TYPE, .type_name = "void" };
        node->function_declaration.return_type = return_type;
    }
    ASSERT( (Lexer_peek(lexer).kind == OPEN_CURRLY_PARENT) , "%s %d: expected '{', got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
    node->function_declaration.body = parse_block_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after if_statement body, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
    return node;
}

AstNode* parse_for(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME FOR
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
        node->type = AST_FOR_STATEMENT;

    node->for_statement.initial = parse_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON ), "%s %d: Expected SEMICOLON after FOR init expr, got %s",__FILE__,__LINE__,format_token(Lexer_curr(lexer)));

    node->for_statement.condition = parse_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON ), "%s %d: Expected SEMICOLON after FOR condition expr",__FILE__,__LINE__);

    node->for_statement.iteration = parse_statement(lexer);

    ASSERT( (Lexer_peek(lexer).kind == OPEN_CURRLY_PARENT) , "%s %d: expected '{', got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
    node->for_statement.body = parse_block_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after if_statement body, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
    return node;
}

AstNode* parse_while(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME WHILE
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
        node->type = AST_WHILE_STATEMENT;
        node->while_statement.condition = parse_statement(lexer);

    ASSERT( (Lexer_peek(lexer).kind == OPEN_CURRLY_PARENT) , "%s %d: expected '{', got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
    node->while_statement.body = parse_block_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after if_statement body, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
    return node;
}
AstNode* parse_return(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME RETURN
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
        node->type = AST_RETURN_STATEMENT;
        node->return_statement.expression = parse_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == SEMICOLON ), "%s %d: Expected SEMICOLON after return expr , got %s",__FILE__,__LINE__,format_token(Lexer_curr(lexer)));
    return node;
}
AstNode* parse_if(Lexer* lexer) {
    Lexer_next(lexer); // CONSUME IF
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
        node->type = AST_IF_STATEMENT;
        node->if_statement.condition = parse_expr_statement(lexer);

    ASSERT( (Lexer_peek(lexer).kind == OPEN_CURRLY_PARENT) , "%s %d: expected '{', got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_peek(lexer)),lexer->idx);
    node->if_statement.body = parse_block_statement(lexer);
    ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after if_statement body, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);

    if( Lexer_peek(lexer).kind == ELSE ) {
        Lexer_next(lexer);
        switch( Lexer_peek(lexer).kind ) {
            case OPEN_CURRLY_PARENT:
                node->if_statement.else_block = parse_block_statement(lexer);
                ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after else_statement body, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
                break;
            case IF:
                node->if_statement.else_block = parse_if(lexer);
                ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after else_if_statement body, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
                break;
            default:
            PANIC("%s %d: expected '{' or 'if', got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_peek(lexer)),lexer->idx);
        }
    }

    return node;
}

/// Consumes ending SEMICOLON
AstNode* parse_expr_statement(Lexer* lexer) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
        node->type = AST_EXPRESSION_STATEMENT;
        node->expression_statement.expression = parse_expr(lexer,0);

    Token next = Lexer_peek(lexer);
    if( next.kind == SEMICOLON ) {
        Lexer_next(lexer);
    } else {
        //          (arfer conditional stm)          (inside function call)    (inside function call)
        ASSERT( (next.kind == OPEN_CURRLY_PARENT || next.kind == CLOSE_PARENT  ||  next.kind == COMMA ), "%s %d: Expected SEMICOLON, OPEN_CURRLY_PARENT, CLOSE_PARENT or COMMA after expr statement , got %s",__FILE__,__LINE__,format_token(Lexer_curr(lexer)));
    }
    return node;
}
AstNode* parse_struct_decl(Lexer* lexer) {
    Lexer_next(lexer); // Consume STRUCT
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
        node->type = AST_STRUCT_DECLARATION;
        node->struct_declaration.name = Lexer_next(lexer).value;
    ASSERT( (Lexer_curr(lexer).kind == IDENT), "%s %d: Expected IDENT after STRUCT keyword",__FILE__,__LINE__);
        node->struct_declaration.body = parse_block_statement(lexer);
    return node;
}
AstNode* parse_extern_statement(Lexer* lexer) {
    Lexer_next(lexer); // Consume EXTERN
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
        node->type = AST_EXTERN_STATEMENT;
        node->extern_statement.body = parse_statement(lexer);
    return node;
}
AstNode* parse_statement(Lexer* lexer) {
    Token next = Lexer_peek(lexer);
    if( next.kind == EOF_TOKEN || next.kind == CLOSE_CURRLY_PARENT) // The caller must consume the CLOSE_CURRLY_PARENT
        return NULL;
    if( next.kind == SEMICOLON || next.kind == CLOSE_PARENT ) { // We are in an empty for loop statement or empty return_expr
        Lexer_next(lexer);
        return NULL;
    }

    AstNode* node = (AstNode*)malloc(sizeof(AstNode));

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
            ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after block statement, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
            node->block_statement.next = NULL;
            return node;
        case FN:
            node = parse_func_decl(lexer);
            node->function_declaration.next = NULL;
            return node;
        case EXTERN:
            node = parse_extern_statement(lexer);
            node->extern_statement.next = NULL;
            return node;
        case STRUCT:
            node = parse_struct_decl(lexer);
            node->struct_declaration.next = NULL; 
            return node;
    }
    // expected identifier than if ':' its a declaration if not an expression;
    //ASSERT( (next.kind == IDENT || next.kind == OPEN_PARENT || is_unary(next)) ,"expected KEYWORD,UNARY_OPP,OPEN_CURRLY_PARENT, OPEN_PARENT or IDENT got %s, idx: %d",format_token(next),lexer->idx);

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
AstNode* parse_statements(Lexer* lexer) {
    Token next = Lexer_peek(lexer);
    if( next.kind == EOF_TOKEN || next.kind == CLOSE_CURRLY_PARENT ) 
        return NULL;

    AstNode* node = (AstNode*)malloc(sizeof(AstNode));

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
            ASSERT( (Lexer_curr(lexer).kind == CLOSE_CURRLY_PARENT) , "%s %d: expected '}' after block statement, got %s, idx: %d",__FILE__,__LINE__,format_token(Lexer_curr(lexer)),lexer->idx);
            node->block_statement.next = parse_statements(lexer);
            return node;
        case FN:
            node = parse_func_decl(lexer);
            node->function_declaration.next = parse_statements(lexer);
            return node;
        case EXTERN:
            node = parse_extern_statement(lexer);
            node->extern_statement.next = parse_statements(lexer);
            return node;
        case STRUCT:
            PANIC("Structs Are not Supported")
            /*
            node = parse_struct_decl(lexer);
            node->struct_declaration.next = parse_statements(lexer);
            return node;
            */
    }
    //ASSERT( (next.kind == IDENT || next.kind == OPEN_PARENT || is_unary(next)) ,"expected KEYWORD,UNARY_OPP,OPEN_CURRLY_PARENT, OPEN_PARENT or IDENT got %s, idx: %d",format_token(next),lexer->idx);

    // expected identifier than if ':' its a declaration if not an expression;
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

AstNode* parse_program(Lexer* lexer) {
    AstNode* ast = parse_statements(lexer);
    return ast;
}

#include <errno.h>
//returns one of:
// POINTER_TYPE,
// ARRAY_TYPE,
// UNKNOWN_TYPE,
Type* parse_type(Lexer* lexer) {
    Type* type = (Type*)malloc(sizeof(Type));
    Token next = Lexer_next(lexer);
    switch( next.kind ) {
        case STAR:
            type->type_kind = POINTER_TYPE;
            type->type_name = NULL;

            type->pointer_type.sub_type = parse_type(lexer);
            return type;
        case SUBSCRIPT_OPEN: 
            PANIC("Arrays are not supported")
        case IDENT:
            type->type_kind = UNKNOWN_TYPE;
            type->type_name = next.value;
            return type;
        default:
            PANIC("Expected STAR,SUBSCRIPT_OPEN or IDENT, got %s (%s)",format_token(next),next.value);
    }
}
