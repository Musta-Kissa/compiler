#define UNARY 1
#define BINARY 2
#define FCALL 3
#define BRANCH 4
#define MOVE 5

int tac_instr_kind(TacInstr instr) {
        switch(instr.op) {
            case TAC_ADD: 
            case TAC_SUB: 
            case TAC_MUL: 
            case TAC_DIV: 
            case TAC_CMP_EQ:
            case TAC_CMP_NE:
            case TAC_CMP_LT:
            case TAC_CMP_GT:
            case TAC_CMP_LE:
            case TAC_CMP_GE:
            case TAC_AND:     
            case TAC_OR:     
                return BINARY;
                break;
            case TAC_RET: 
            case TAC_LABEL:
            case TAC_ALLOC:
                return UNARY;
                break;
            case TAC_FCALL: 
                return FCALL;
                break;
            case TAC_JMP_IF_NOT: 
            case TAC_JMP_IF: 
            case TAC_JMP: 
                return BRANCH;
            case TAC_MOV:
            case TAC_LOAD:
            case TAC_STORE:
            case TAC_ADDR:
                return MOVE;
                break;
            default: PANIC();
        }
    return -1;
}
void print_tac_var(TacVar var){
    switch(var.kind) {
        case VAR_TEMP:          printf("t%d",var.temp_id);  break;
        case VAR_LOCAL:         printf("%s",var.ident);     break;
        case VAR_LABEL:         printf("%s",var.label_name);break;
        case VAR_CONST_INT:     printf("%d",var.int_val);   break;
        case VAR_CONST_UINT:    printf("%u",var.uint_val);  break;
        case VAR_CONST_FLOAT:   printf("%f",var.float_val); break;
        case VAR_VOID:          printf("!VOID!"); break;
        default: PANIC();
    }
}

void print_tac_type(TacType type) {
    switch(type) {
        case TAC_VOID: break;
        case TAC_PTR: printf("ptr"); break;
        case TAC_I64: printf("i64"); break;
        case TAC_F64: printf("f64"); break;
        //case TAC_B64: PANIC("DEPRECATED"); break;
        case TAC_U64: printf("u64"); break;
        default: PANIC();
    }
}

void print_tac(TacInstrDA instructions);

void print_tac_proc(TacProc proc) {
    TacInstrDA instructions = proc.instructions;
    printf("%s",proc.ident);
    if( proc.args.count > 0 ) {
        for(int i = 0; i < proc.args.count; i++) {
            ProcArg arg = proc.args.items[i];
            printf(" %s ",arg.ident);
            print_tac_type(arg.type);
            printf(",");
        }
        printf("\b");
    }
    printf(":\n");
    print_tac(instructions);
}

void print_tac(TacInstrDA instructions) {
    for(int i = 0; i < instructions.count; i++){
        TacInstr instr = instructions.items[i];
        switch(tac_instr_kind(instr)) {
            case BINARY: {
                print_tac_var(instr.binary.result);
                printf(" = ");
                switch(instr.op) {
                    case TAC_ADD: printf("add"); break;
                    case TAC_SUB: printf("sub"); break;
                    case TAC_MUL: printf("mul"); break;
                    case TAC_DIV: printf("div"); break;
                    case TAC_CMP_EQ: printf("cmp_eq"); break;
                    case TAC_CMP_NE: printf("cmp_ne"); break;
                    case TAC_CMP_LT: printf("cmp_lt"); break;
                    case TAC_CMP_GT: printf("cmp_gt"); break;
                    case TAC_CMP_LE: printf("cmp_le"); break;
                    case TAC_CMP_GE: printf("cmp_ge"); break;
                    case TAC_AND: printf("and"); break;
                    case TAC_OR:  printf("or"); break;
                    default: PANIC();
                }
                if(instr.type != TAC_VOID ) {
                    printf("_");
                    print_tac_type(instr.type);
                }
                printf(" ");
                print_tac_var(instr.binary.arg1);
                printf(", ");
                print_tac_var(instr.binary.arg2);
                printf("\n");

            } break;
            case UNARY: {
                switch(instr.op) {
                    case TAC_ALLOC: printf("alloc"); break;
                    case TAC_ADDR:  printf("addr_of"); break;
                    case TAC_RET:   printf("ret"); break;
                    case TAC_LABEL: {
                        printf("%s:\n",instr.unary.src.label_name);
                        continue;
                    }
                }
                if(instr.type != TAC_VOID ) {
                    printf("_");
                    print_tac_type(instr.type);
                }
                printf(" ");
                print_tac_var(instr.unary.src);
                printf("\n");

            } break;
            case FCALL: {
                if( instr.fcall.result.kind != VAR_VOID ) {
                    print_tac_var(instr.fcall.result);
                    printf(" = ");
                }
                if(instr.type == TAC_VOID) {
                    printf("call");
                } else {
                    printf("call_");
                    print_tac_type(instr.type);
                }
                printf(" %s(",instr.fcall.ident);
                if( instr.fcall.args.count > 0 ) {
                    for(int i = 0; i < instr.fcall.args.count; i++) {
                        print_tac_var(instr.fcall.args.items[i]);  
                        printf(", ");
                    }
                    printf("\b\b");
                }
                printf(")\n");
            } break;
            case BRANCH: {
                switch(instr.op) {
                    case TAC_JMP_IF_NOT:
                        printf("if not ");
                        print_tac_var(instr.jump.src);
                        printf(" goto %s\n",instr.jump.label);
                        break;
                    case TAC_JMP_IF:
                        printf("if ");
                        print_tac_var(instr.jump.src);
                        printf(" goto %s\n",instr.jump.label);
                        break;
                    case TAC_JMP:
                        printf("goto %s\n",instr.jump.label);
                        break;
                }
            } break;
            case MOVE: {
                switch(instr.op) {
                    case TAC_LOAD: printf("load"); break;
                    case TAC_STORE: printf("store"); break;
                    case TAC_MOV: 
                        print_tac_var(instr.move.dest);
                        printf(" = "); 
                        print_tac_var(instr.move.src);
                        printf("\n");
                        continue;
                    case TAC_ADDR:
                        print_tac_var(instr.move.dest);
                        printf(" = addr_of "); 
                        print_tac_var(instr.move.src);
                        printf("\n");
                        continue;
                }
                if(instr.type != TAC_VOID ) {
                    printf("_");
                    print_tac_type(instr.type);
                }
                printf(" ");
                print_tac_var(instr.move.dest);
                printf(", ");
                print_tac_var(instr.move.src);
                printf("\n");
            } break;
        }
    }
}
