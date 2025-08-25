#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "my_string.h"
#include "parser.h"
#include "analyzer.h"

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}

#include "print_ast.h"

int main(int argc, char* argv[]) {
    FILE* f = fopen("./input3.txt","r");

    String source = String_readfile(f);
    printf("source: \n%s",source.data);
    printf("============= end source ===============\n\n");

    Lexer lexer = lex_file(source);

    for( int n = 0; lexer.tokens[n-1].kind != EOF_TOKEN ; n++) {
        Token t = lexer.tokens[n];
        printf("%d: %s ",n,format_enum(t));
        switch(t.kind) {
            case IDENT: 
            case NUMBER:
            case STRING:
                printf("val: %s",t.value);
        }
        printf("\n");
    }
    AstExpr* program = parse_program(&lexer);
    print_program_ast(program);

    analyze_program_ast(program);

}
