#include "parser.h"
#include "types.h"
#include "my_string.h"
#include "analyzer.h"

char* generate_output(AstExpr* node);
void generate_statements(StringBuilder* sb, AstExpr* stm);
int compile_string(char* source);
