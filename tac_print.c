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
                return BINARY;
                break;
            case TAC_RET: 
                return UNARY;
                break;
            case TAC_FCALL: 
                return FCALL;
                break;
            case TAC_COND_BR: 
            case TAC_BR: 
                return BRANCH;
                break;
        }
}
void print_tac_var(TacVar var){
    switch(var.kind) {
        case VAR_TEMP:          printf("t%d",var.temp_id);  break;
        case VAR_LOCAL:         printf("%s",var.ident);     break;
        //case VAR_LABEL:         printf("%s",var.label_name);break;
        case VAR_CONST_INT:     printf("%d",var.int_val);   break;
        case VAR_CONST_FLOAT:   printf("%f",var.float_val); break;
    }
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
                    case TAC_RET: printf("ret"); break;
                    case TAC_CMP_EQ: printf("cmp_eq"); break;
                    case TAC_CMP_NE: printf("cmp_ne"); break;
                    case TAC_CMP_LT: printf("cmp_lt"); break;
                    case TAC_CMP_GT: printf("cmp_gt"); break;
                    case TAC_CMP_LE: printf("cmp_le"); break;
                    case TAC_CMP_GE: printf("cmp_ge"); break;
                }
                switch(instr.type) {
                    case TAC_VOID: break;
                    case TAC_I64: printf("_i64"); break;
                    case TAC_F64: printf("_f64"); break;
                }
                printf(" ");
                print_tac_var(instr.binary.arg1);
                printf(", ");
                print_tac_var(instr.binary.arg2);
                printf("\n");

            } break;
            case UNARY: {
                switch(instr.op) {
                    case TAC_RET: printf("ret"); break;
                }
                switch(instr.type) {
                    case TAC_VOID: break;
                    case TAC_I64: printf("_i64"); break;
                    case TAC_F64: printf("_f64"); break;
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
                printf("%s(",instr.fcall.ident);
                for(int i = 0; i < instr.fcall.args.count; i++) {
                    print_tac_var(instr.fcall.args.items[i]);  
                    printf(", ");
                }
                printf("\b\b)\n");
            } break;
            case BRANCH: {
                switch(instr.op) {
                    case TAC_COND_BR:
                        printf("if ");
                        print_tac_var(instr.branch.src);
                        printf(" goto %s\n",instr.branch.label);
                        break;
                    case TAC_BR:
                        printf("goto %s\n",instr.branch.label);
                        break;
                }
            } break;
        }
    }
}
