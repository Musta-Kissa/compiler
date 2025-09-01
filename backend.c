#include "backend.h"
#include "parser.h"
#include "types.h"
#include "my_string.h"
#include "analyzer.h"

#define ASSERT(expr, fmt, ...) { \
    if (!expr) { \
        printf(fmt "\n", ##__VA_ARGS__); \
        exit(-1); \
    } \
}
#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    *(int*)0=0;\
    exit(-1); \
}

#define PADDING() { \
    for(int n = 0; n < CURR_DEPTH; n++) { \
        sb_append(sb,"    "); \
    } \
} \

int CURR_DEPTH = 0;

void generate_type(StringBuilder* sb, Type* type) {
    switch( type->type_kind ) {
        case STRUCT_TYPE:
        case PRIMITIVE_TYPE:
        case ENUM_TYPE:
        case UNION_TYPE:
            ASSERT( (type->type_name != NULL), "%s %d:PANICKED",__FILE__,__LINE__);
            sb_append(sb,type->type_name);
            break;
        case POINTER_TYPE:
            generate_type(sb,type->pointer_type.sub_type);
            sb_append(sb,"*");
            break;
        case ARRAY_TYPE:
            //generate_type(sb,type->array_type.sub_type);
            //sb_append(sb,"__%s"); 
            sb_append(sb,"__Array ");
            break;
            //PANIC("%s %d:Arrays not supported",__FILE__,__LINE__);
            //sb_append(sb,"Intrinsics_Array");

        case NUMBER_TYPE:
        case BOOL_TYPE:
            PANIC("%s %d:PANICKED",__FILE__,__LINE__);
        case FUNCTION_TYPE:
        case UNKNOWN_TYPE:
        default:
            sb_append(sb,type->type_name);
            break;
            PANIC("%s %d:PANICKED",__FILE__,__LINE__);
    }
}
void generate_arg_decl(StringBuilder* sb,AstExpr* curr_stm) {
    sb_append(sb,"(");
    if(curr_stm == NULL) {
        sb_append(sb,")");
        return;
    }

    while(1) {
        Type* curr_type = curr_stm->argument_decl.type;
        char* curr_ident = curr_stm->argument_decl.ident;

        generate_type(sb,curr_type);
        sb_append(sb," %s ",curr_ident);

        curr_stm = curr_stm->argument_decl.next;

        if(curr_stm == NULL) {
            break;
        } else {
            sb_append(sb,",");
        }
    }
    sb_append(sb,")");
}
int _;

void generate_block_statement(StringBuilder* sb, AstExpr* stm) {
    //PADDING();
    sb_append(sb,"{\n");
    generate_statements(sb,stm->block_statement.statements);
    PADDING();
    sb_append(sb,"}\n");
}

void generate_func_decl(StringBuilder* sb, AstExpr* stm) {
    generate_type(sb,stm->function_declaration.return_type);
    sb_append(sb," ");
    sb_append(sb,stm->function_declaration.name);
    generate_arg_decl(sb,stm->function_declaration.args);
    generate_block_statement(sb,stm->function_declaration.body);
}
void generate_func_call(StringBuilder* sb, AstExpr* stm) {
    sb_append(sb,stm->func_call.identifier.value);
    sb_append(sb,"(");
    AstExpr* curr_arg = stm->func_call.args;
    if( curr_arg != NULL) {
        while(1) {
            generate_expr_statement(sb,curr_arg->argument.value);
            curr_arg = curr_arg->argument.next;
            if( curr_arg == NULL) {
                break;
            }
            sb_append(sb,",");
        }
    }
    sb_append(sb,")");
}
void generate_expr(StringBuilder* sb, AstExpr* stm) {
    switch( stm->type ) {
        char* operator = "";
        case AST_UNARY_OPERATION:
            switch( stm->unary_operation.opp_token.kind ) {
                case NOT:           operator = "!"; break;
                case MINUS:         operator = "-"; break;
                case PLUS_PLUS:     operator = "++"; break;
                case MINUS_MINUS:   operator = "--"; break;
                case AMPERSAND:     operator = "&"; break;
                case STAR:          operator = "*"; break;
                default: 
                    PANIC("%s %d: Panicked",__FILE__,__LINE__);
            }
            sb_append(sb,operator);
            sb_append(sb,"(");
            generate_expr(sb,stm->unary_operation.right);
            sb_append(sb,")");
            break;
        case AST_BINARY_OPERATION:
            switch( stm->binary_operation.opp_token.kind ) {
                case STAR:              operator = "*"; break;
                case PLUS:              operator = "+";break;
                case DIVITION:          operator = "/";break;
                case MINUS:             operator = "-";break;
                case EQUAL:             operator = "==";break;
                case NOT_EQUAL:         operator = "!=";break;
                case LESS_THEN:         operator = "<";break;
                case MORE_THEN:         operator = ">";break;
                case LESS_EQUAL:        operator = "<=";break;
                case MORE_EQUAL:        operator = ">=";break;
                case ASSIGN:            operator = "=";break;
                case DOT:               operator = ".";break;
                //case SUBSCRIPT_OPEN:    operator = "[";break;
                case SUBSCRIPT_OPEN:    operator = ")["; break;
                    
                default:
                    PANIC("%s %d:PANICKED",__FILE__,__LINE__);
            }
            sb_append(sb,"(");
            if(  stm->binary_operation.opp_token.kind == SUBSCRIPT_OPEN) {
                sb_append(sb,"((");
                generate_type(sb,&stm->binary_operation.type);
                sb_append(sb,"*");
                sb_append(sb,")");
            }
            generate_expr(sb,stm->binary_operation.left);

            if(  stm->binary_operation.opp_token.kind == SUBSCRIPT_OPEN) {
                sb_append(sb,".data");
            }

            sb_append(sb," ");
            sb_append(sb,operator);
            sb_append(sb," ");
            generate_expr(sb,stm->binary_operation.right);
            if(  stm->binary_operation.opp_token.kind == SUBSCRIPT_OPEN) {
                sb_append(sb,"]");
            }
            sb_append(sb,")");
            break;
        case AST_IDENTIFIER:
            sb_append(sb,stm->identifier.token.value);
            return;
        case AST_NUMBER:
            sb_append(sb,stm->number.token.value);
            return;
        case AST_STRING:
            sb_append(sb,"\"%s\"",stm->string.token.value);
            return;
        case AST_FUNC_CALL:
            return generate_func_call(sb,stm); 
        default:
            PANIC("%s %d:PANICKED",__FILE__,__LINE__);
    }
}
void generate_expr_statement(StringBuilder* sb, AstExpr* stm) {
    if( stm->expression_statement.value != NULL ) {
        generate_expr(sb,stm->expression_statement.value);
    }
}

void generate_decl(StringBuilder* sb, AstExpr* stm) {
    PADDING();
    if( stm->declaration.type->type_kind == ARRAY_TYPE ) {
        generate_type(sb,stm->declaration.type->array_type.sub_type);
        if( stm->declaration.type->array_type.length == -1 ) {
            // if len not specified there has to be an expr
            int len = stm->declaration.value->expression_statement.type.array_type.length;

            sb_append(sb," __%s[%d]; __Array %s = (__Array){.data=__%s,.length=%d}",
                      stm->declaration.name,
                      len,
                      stm->declaration.name,
                      stm->declaration.name,
                      len
                      );
            sb_append(sb,"; %s = ",stm->declaration.name);
            generate_expr_statement(sb,stm->declaration.value);
        } else {
            sb_append(sb," __%s[%d]; __Array %s = (__Array){.data=__%s,.length=%d}",
                      stm->declaration.name,
                      stm->declaration.type->array_type.length,
                      stm->declaration.name,
                      stm->declaration.name,
                      stm->declaration.type->array_type.length
                      );
            if( stm->declaration.value->expression_statement.value != NULL ) {
                sb_append(sb,"; %s = ",stm->declaration.name);
                generate_expr_statement(sb,stm->declaration.value);
            }
        }
    } else {
        generate_type(sb,stm->declaration.type);
        sb_append(sb," %s",stm->declaration.name);
        if( stm->declaration.value->expression_statement.value != NULL ) {
            sb_append(sb," = ");
            generate_expr_statement(sb,stm->declaration.value);
        }
    }
    sb_append(sb,";\n");
}

void generate_if(StringBuilder* sb, AstExpr* stm) {
    PADDING();
    sb_append(sb,"if ");
    generate_expr_statement(sb,stm->if_statement.condition);
    sb_append(sb," ");
    generate_block_statement(sb,stm->if_statement.body); 
}
void generate_struct_decl(StringBuilder* sb, AstExpr* stm) {
    sb_append(sb,"typedef struct ");
    generate_block_statement(sb,stm->struct_declaration.body); 
    sb->length--; // delete the newline
    sb_append(sb,"%s;\n",stm->struct_declaration.name);
}

void generate_statements(StringBuilder* sb, AstExpr* stm) {
    CURR_DEPTH += 1;
    AstExpr* next = stm;
    while( next != NULL ) {
        switch( next->type ) {
            case AST_FUNCTION_DECLARATION:
                generate_func_decl(sb,next); 
                next = next->function_declaration.next;
                break;
            case AST_BLOCK_STATEMENT:
                PADDING();
                generate_block_statement(sb,next); 
                next = next->block_statement.next;
                break;
            case AST_DECLARATION:
                generate_decl(sb,next); 
                next = next->declaration.next;
                break;
            case AST_IF_STATEMENT:
                generate_if(sb,next); 
                next = next->if_statement.next;
                break;
            case AST_EXPRESSION_STATEMENT:
                PADDING();
                generate_expr_statement(sb,next); 
                sb_append(sb,";\n");
                next = next->expression_statement.next;
                break;
            case AST_STRUCT_DECLARATION:    
                generate_struct_decl(sb,next);
                next = next->struct_declaration.next;
                break;
            case AST_EXTERN_STATEMENT:    
                // dont generate code for extern stm
                next = next->extern_statement.next;
                break;
            /*
            case AST_FOR_STATEMENT:
                analyze_for(next); 
                next = next->for_statement.next;
                break;
            case AST_WHILE_STATEMENT:
                analyze_while(next); 
                next = next->while_statement.next;
                break;
            case AST_RETURN_STATEMENT:
                analyze_return(next); 
                next = next->return_statement.next;
                break;
            */
            default:
                PANIC("NOT SUPPORTED: %s",format_ast_type(next));
        }
    }
    CURR_DEPTH -= 1;
}


char* generate_output(AstExpr* node) {
    StringBuilder output_sb = sb_new();
    const char *header = 
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "typedef struct {\n"
        "   void* data;\n"
        "   int length;\n"
        "} __Array;\n"
        "// ===================== end of HEADER =================================\n"
    ;

    sb_append(&output_sb,header);
    CURR_DEPTH = -1;
    generate_statements(&output_sb,node);

    return output_sb.buffer;
}

#include <sys/stat.h>
#include <sys/types.h>

int compile_string(char* source) {
    mkdir("out", 0777);
    const char *filename = "out/out.c";
    FILE *file = fopen(filename, "w");
    if (file == NULL) { PANIC("Failed to create temporary file"); }

    // Write the C code to the file
    fprintf(file, "%s", source);
    fclose(file);

    // Compile the temporary file
    int compile_status = system("cd ./out; gcc -g out.c -o out");
    if (compile_status != 0) {
        PANIC("Compilation failed\n");
        return 1;
    } else {
        printf("\e[0;32mCompiled ✓\e[0m\n"); 
        return 0;
    }
}
