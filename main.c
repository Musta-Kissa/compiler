#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "my_string.h"
#include "parser.h"
#include "analyzer.h"
#include "backend.h"

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}


int compile(const char *asm_code);

int main(int argc, char* argv[]) {
    FILE* f = fopen("./input.txt","r");

    String source = String_readfile(f);
    //printf("source: \n%s",source.data);
    //printf("============= end source ===============\n\n");

    //printf("sizeof(Tyep) = %d\n",sizeof(Type));
    //printf("sizeof(Ast) = %d\n",sizeof(AstNode));


    Lexer lexer = lex_file(source);

    /*
    for( int n = 0; lexer.tokens[n-1].kind != EOF_TOKEN ; n++) {
        Token t = lexer.tokens[n];
        printf("%d: %s ",n,format_token(t));
        switch(t.kind) {
            case IDENT: 
            case NUMBER:
            case STRING:
                printf("val: %s",t.value);
        }
        printf("\n");
    }
    */

    AstNode* program = parse_program(&lexer);

    //print_program_ast(program);

    analyze_program_ast(program);
    //printf("\e[0;32manalyzed ✓\e[0m\n"); 
    
    const char* output = generate_asm(program);
    if( compile(output) == 0 ) {
        printf("COMPILATION DONE\n");
    } else {
        PANIC("COMPILATION FAILED");
    }
}

int compile(const char *asm_code) {
    char *asm_filename = "./out/out.s";
    char *output_name = "./out/out";
    char command[512];
    
    // Write assembly file
    FILE *fp = fopen(asm_filename, "w");
    if (!fp) {
        perror("Failed to create assembly file");
        return -1;
    }

    fprintf(fp, "%s", asm_code);
    fclose(fp);
    
    // Build command string
    snprintf(command, sizeof(command),
             "nasm -f elf64 %s -o %s.o && ld ./std/std.o %s.o -o %s",
             asm_filename, output_name, output_name, output_name);
    
    //printf("Executing: %s\n", command);
    
    // Execute the command
    return system(command);
}
