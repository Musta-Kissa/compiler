#include "parser.h"
#include "lexer.h"

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}
#define ASSERT_EQUAL(expected, actual, fmt, ...) { \
    if ((expected) != (actual)) { \
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
    }
}

int is_opp(TokenKind k) {
    switch(k) {
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


AstExpr* parse_leaf(Lexer* lexer) {
    Token t = Lexer_next(lexer);
    AstExpr* leaf = (AstExpr*)malloc(sizeof(AstExpr));
    switch(t.kind) {
        case IDENT:
            leaf->type = AST_IDENTIFIER;
            leaf->identifier.token = t;
            return leaf;
        case NUMBER:
            leaf->type = AST_NUMBER;
            leaf->number.token = t;
            return leaf;
        case OPEN_PARENT:
            return NULL;
        //case SUBSCRIPT_OPEN:
            //leaf->type = AST_SUBSCRIPT;
            //return leaf;
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
        //PRETEND EOF
        return left;
    }
    if( !is_opp(next.kind)) {
        // EOF
        ASSERT_EQUAL(next.kind, SEMICOLON, "%s %d: expected SEMICOLON, got %s, lexer idx: %d",
                __FILE__,
                __LINE__,
                format_enum(next.kind),
                lexer->idx
                );
        return left;
    }

    int next_bp = get_binding_power(next.kind);
    if( next_bp <= min_bp ) {
        // Pretend EOF
        return left;
    } else {
        Lexer_next(lexer);
        AstExpr* right;
        if( next.kind == SUBSCRIPT_OPEN) {
            right = parse_expr(lexer,0);
            Token next = Lexer_next(lexer);
            ASSERT_EQUAL(next.kind, SUBSCRIPT_CLOSE, "%s %d: expected close CLOSE_PARENT got: %s", __FILE__, __LINE__, format_enum(next.kind));
        } else {
            right = parse_expr(lexer,next_bp);
        }
        return AST_make_binary(left,next,right);
    }
    
}
AstExpr* parse_expr(Lexer* lexer, int min_bp) {
    AstExpr* left = parse_leaf(lexer);
    //if( left->type == AST_SUBSCRIPT ) {
        //left = parse_expr(lexer,0);
        //Token next = Lexer_next(lexer);
        //ASSERT_EQUAL(next.kind, SUBSCRIPT_CLOSE, "%s %d: expected close SUBSCRIPT_CLOSE got: %s", __FILE__,__LINE__,format_enum(next.kind));
    //}
    if( left == NULL ) {
        // OPENING PARENT
        left = parse_expr(lexer,0);
        Token next = Lexer_next(lexer);
        ASSERT_EQUAL(next.kind, CLOSE_PARENT, "%s %d: expected close CLOSE_PARENT got: %s", __FILE__,__LINE__,format_enum(next.kind));
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
