void analyze_lifetimes_addr(TacInstr instr, VariableInfoDA *vars, int curr_line) {
    // addr creates temp var
    TacVar dest = instr.move.dest;
    TacVar src = instr.move.src;
    VariableInfo *info;
    TacType type;

    DBG_ASSERT((dest.kind == VAR_TEMP && src.kind == VAR_TEMP),"");

    if(!get_var_info_ref(*vars, &info, src)) PANIC();
    info->last_line_used = curr_line;
    type = info->type;

    DBG_ASSERT((!get_var_info_ref(*vars, &info, dest)),"");
    da_append_ref(vars,((VariableInfo){
        .type = type,
        .key  = dest,
        .location = {0},
        .first_line_used = curr_line,
        .last_line_used  = curr_line,
    }));

}
void analyze_lifetimes_load(TacInstr instr, VariableInfoDA *vars, int curr_line) {
    //load crates a temp var
    TacVar dest = instr.move.dest;
    TacVar src = instr.move.src;
    VariableInfo *info;
    TacType type;

    DBG_ASSERT((dest.kind == VAR_TEMP && src.kind == VAR_TEMP),"");

    if(!get_var_info_ref(*vars, &info, src)) PANIC();
    info->last_line_used = curr_line;
    type = info->type;

    DBG_ASSERT((!get_var_info_ref(*vars, &info, dest)),"");
    da_append_ref(vars,((VariableInfo){
        .type = type,
        .key  = dest,
        .location = {0},
        .first_line_used = curr_line,
        .last_line_used  = curr_line,
    }));
}
void analyze_lifetimes_store(TacInstr instr, VariableInfoDA *vars, int curr_line) {
    //store puts into existing var
    TacVar dest = instr.move.dest;
    TacVar src = instr.move.src;
    VariableInfo *info;
    //TacType type;

    DBG_ASSERT((dest.kind == VAR_TEMP && src.kind == VAR_TEMP),"");

    if(!get_var_info_ref(*vars, &info, src)) PANIC();
    info->last_line_used = curr_line;
    //type = info->type;

    if(!get_var_info_ref(*vars, &info, dest)) PANIC();
    info->last_line_used = curr_line;
}
void analyze_lifetimes_branch(TacInstr instr, VariableInfoDA *vars, int curr_line) {
    TacVar src = instr.jump.src;
    VariableInfo *info;

    if(!get_var_info_ref(*vars, &info, src)) PANIC();
    info->last_line_used = curr_line;
}

void analyze_lifetimes_fcall(TacInstr instr, VariableInfoDA *vars, int curr_line) {
    TacVar result = instr.fcall.result;
    VariableInfo *info;

    if( result.kind != VAR_VOID ) {
        DBG_ASSERT((instr.type != TAC_VOID),"");
        if(get_var_info_ref(*vars, &info, result)){
            info->last_line_used = curr_line;
        } else {
            DBG_ASSERT((result.kind == VAR_TEMP),"");
            da_append_ref(vars,((VariableInfo){
                .type = instr.type,
                .key  = result,
                .location = {0},
                .first_line_used = curr_line,
                .last_line_used  = curr_line,
            }));
        }
    }

    for(int i = 0; i < instr.fcall.args.count; i++) {
        if(!get_var_info_ref(*vars, &info, instr.fcall.args.items[i])) PANIC();
        info->last_line_used = curr_line;
    }
}

void analyze_lifetimes_ret(TacInstr instr, VariableInfoDA *vars, int curr_line) {
    TacVar src = instr.unary.src;
    VariableInfo *info;

    if(!get_var_info_ref(*vars, &info, src)) PANIC();
    info->last_line_used = curr_line;
}

void analyze_lifetimes_binary(TacInstr instr, VariableInfoDA *vars, int curr_line) {
    TacVar arg1 = instr.binary.arg1;
    TacVar arg2 = instr.binary.arg2;
    TacVar result = instr.binary.result;
    VariableInfo *info;

    DBG_ASSERT((result.kind == VAR_TEMP && arg1.kind == VAR_TEMP && arg2.kind == VAR_TEMP ),"");

    DBG_ASSERT((!get_var_info_ref(*vars, &info, result)), "");
    da_append_ref(vars,((VariableInfo){
        .type = instr.type,
        .key  = result,
        .location = {0},
        .first_line_used = curr_line,
        .last_line_used  = curr_line,
    }));

    if(!get_var_info_ref(*vars, &info, arg1)) PANIC();
    info->last_line_used = curr_line;

    if(!get_var_info_ref(*vars, &info, arg2)) PANIC();
    info->last_line_used = curr_line;
}

void analyze_lifetimes_mov(TacInstr instr, VariableInfoDA *vars, int curr_line) {
    TacVar dest = instr.move.dest;
    TacVar src = instr.move.src;
    VariableInfo *info;
    TacType type;

    if(get_var_info_ref(*vars, &info, src)) {
        info->last_line_used = curr_line;
        type = info->type;
    } else {
        switch(src.kind) {
            case VAR_CONST_INT:     type = TAC_I64; break;
            case VAR_CONST_FLOAT:   type = TAC_F64; break;
            default: PANIC("ASSUMED CONST");
        }
    }

    if(get_var_info_ref(*vars, &info, dest)) {
        info->last_line_used = curr_line;
    } else {
        DBG_ASSERT((dest.kind == VAR_TEMP || dest.kind == VAR_LOCAL),"");
        da_append_ref(vars,((VariableInfo){
            .type = type,
            .key  = dest,
            .location = {0},
            .first_line_used = curr_line,
            .last_line_used  = curr_line,
        }));
    }
}

void analyze_variable_lifetimes_and_types(TacInstrDA instructions, VariableInfoDA *vars) {
    for(int i = 0; i < instructions.count; i++) {
        TacInstr curr_instr = instructions.items[i];

        switch(curr_instr.op) {
            case TAC_ALLOC:     
            case TAC_LABEL:      
            case TAC_JMP:      
                break;
            case TAC_MOV:       analyze_lifetimes_mov(curr_instr, vars, i+1);       break;
            case TAC_RET:       analyze_lifetimes_ret(curr_instr, vars, i+1);       break;
            case TAC_FCALL:     analyze_lifetimes_fcall(curr_instr, vars, i+1);     break;
            case TAC_LOAD:      analyze_lifetimes_load(curr_instr, vars, i+1);      break;
            case TAC_STORE:     analyze_lifetimes_store(curr_instr, vars, i+1);     break;
            case TAC_ADDR:      analyze_lifetimes_addr(curr_instr, vars, i+1);      break;

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
                analyze_lifetimes_binary(curr_instr, vars, i+1);    
                break;
                break;
            case TAC_JMP_IF:
            case TAC_JMP_IF_NOT:
                analyze_lifetimes_branch(curr_instr, vars, i+1);
                break; 
                TODO();
            default: PANIC();
        }
    }
}
