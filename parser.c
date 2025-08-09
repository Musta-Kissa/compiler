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

int get_binding_power(TokenKind opp) {
    switch(opp){
        case LESS_THEN:         return 2;
        case MORE_THEN:         return 2;
        case ADDITION:          return 3;
        case MULTIPLICATION:    return 4;
        case DIVITION:          return 4;
        case SUBSCRIPT_OPEN:    return 5;
        case DOT:               return 6;
    }
}

int is_opp(TokenKind k) {
    switch(k) {
        case DOT: 
        case SUBSCRIPT_OPEN: 
        case LESS_THEN: 
        case MORE_THEN: 
        case MULTIPLICATION:
        case ADDITION:
        case DIVITION:
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
            PANIC("%s %d: expected COMMA or CLOSE_PARENT after expr in function call, got: %s",__FILE__,__LINE__,format_enum(next.kind));
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

AstExpr* parse_leaf(Lexer* lexer) {
    Token t = Lexer_next(lexer);
    AstExpr* leaf = (AstExpr*)malloc(sizeof(AstExpr));
    switch(t.kind) {
        case IDENT:
            if( Lexer_peek(lexer).kind == OPEN_PARENT ) {
                //FunctionCall
                Lexer_next(lexer); 
                return parse_function_call(lexer,t);
            } else {
                leaf->type = AST_IDENTIFIER;
                leaf->identifier.token = t;
                return leaf;
            }
        case NUMBER:
            leaf->type = AST_NUMBER;
            leaf->number.token = t;
            return leaf;
        case OPEN_PARENT:
            return NULL;
            
        default:
            PANIC("%s %d: expected IDENT or NUMBER after %s, got: %s",
                  __FILE__,
                  __LINE__,
                  format_enum(Lexer_peek_back(lexer).kind),
                  format_enum(t.kind));
    };
}

// recursion good when bp rising 
// loop good when bp lowering
// a < b + c * d + e;

AstExpr* parse_incrising_bp(Lexer* lexer, AstExpr* left, int min_bp) {
    Token next = Lexer_peek(lexer);

    if( next.kind == CLOSE_PARENT  || next.kind == SUBSCRIPT_CLOSE ) {
        return left; //PRETEND EOF
    }
    if( !is_opp(next.kind)) { // EOF
        ASSERT((next.kind == SEMICOLON || next.kind == COMMA), "%s %d: expected SEMICOLON or COMMA, got %s, lexer idx: %d",
                __FILE__,
                __LINE__,
                format_enum(next.kind),
                lexer->idx
                );
        return left;
    }

    int next_bp = get_binding_power(next.kind);
    if( next_bp <= min_bp ) {
        return left; // Pretend EOF
    } else {
        Lexer_next(lexer);
        AstExpr* right;
        if( next.kind == SUBSCRIPT_OPEN) {
            right = parse_expr(lexer,0);
            ASSERT(Lexer_next(lexer).kind == SUBSCRIPT_CLOSE, 
                    "%s %d: expected close CLOSE_PARENT got: %s", __FILE__, __LINE__, format_enum(Lexer_peek_back(lexer).kind));
        } else {
            right = parse_expr(lexer,next_bp);
        }
        return AST_make_binary(left,next,right);
    }
    
}
AstExpr* parse_expr(Lexer* lexer, int min_bp) {
    if( Lexer_peek(lexer).kind == SEMICOLON ) { //EMPTY EXPR
        ASSERT(min_bp == 0, "%s %d: expected SEMICOLON should be at the begginig of the expr",__FILE__,__LINE__);
        return NULL;
    }
    AstExpr* left = parse_leaf(lexer);
    if( left == NULL ) {
        // OPENING PARENT
        left = parse_expr(lexer,0);
        ASSERT(Lexer_next(lexer).kind == CLOSE_PARENT, 
                "%s %d: expected close CLOSE_PARENT got: %s", __FILE__,__LINE__,format_enum(Lexer_peek_back(lexer).kind));
    }
    while(true) {
        AstExpr* node = parse_incrising_bp(lexer,left,min_bp);
        if( node == left ) {
            return left;
        } 
        left = node;
    }
}


/*
typedef struct Expr {
    Token opp;
    struct Expr* lhs;
    struct Expr* rhs;
}Expr;

Expr* parse_expr(Lexer* lexer, int curr_bp) {
    Token t = Lexer_next(lexer);

    if (t.kind != IDENT && t.kind != NUMBER && t.kind != OPEN_PARENT ) {
        PANIC("expected a NUMBER or \'(\', got \"%s\"",format_enum(t.kind));
    }

    Expr* lhs = (Expr*)malloc(sizeof(Expr));
    Expr* rhs;

    if ( t.kind == OPEN_PARENT) {
        lhs = parse_expr(lexer,0);
        if (Lexer_next(lexer).kind != CLOSE_PARENT) {
            PANIC("Missing closing parenthesis");
        }
    } else {
        lhs->opp = t;
    }

    while(1) {
        t = Lexer_peek(lexer);
        if (!is_opp(t.kind)) {
            if (t.kind == EOF_TOKEN || t.kind == CLOSE_PARENT) {
                break;
            }
            PANIC("expected a EOF or CLOSE_PARENT, got \"%s\"",format_enum(t.kind));
        }
        int binding_power = get_binding_power(t.kind);
        if( binding_power <= curr_bp ) {
            break;
        }
        Lexer_next(lexer);
        
        rhs = parse_expr(lexer,binding_power);

        Expr* old_lhs = lhs;
        lhs = (Expr*)malloc(sizeof(Expr));
        *lhs = (Expr){ .opp = t, .lhs = old_lhs, .rhs = rhs};
    }
    return lhs;
}
*/
