#include "backend_x86.h"
#include "my_string.h"
#include "panic_macros.h"
#include "tac.h"

#include "backend_x86_lifetimes.c"

int CURR_JMP_LABEL_NUMBER = 0;

char* get_location_str(VariableLocation location) {
    static char buffer[64];
    switch( location.type ) {
        case NOT_ASSIGNED:
            PANIC("PANICKED");
            //return "NOT_ASSIGNED";
        case REGISTER:
            return get_register_str(location.register_location.register_); 
        case STACK:
            snprintf(buffer, sizeof(buffer), "qword [rbp%+d]", location.stack_location.base_offset);
            return buffer;
    }
}

int get_var_info_ref(VariableInfoDA vars, VariableInfo **out, TacVar needle) {
    if( needle.kind != VAR_TEMP && needle.kind != VAR_LOCAL ) {
        return 0;
        print_tac_var(needle);
        PANIC("NOT SUPPORTED");
    }
    for(int i = 0; i < vars.count; i++) {
        VariableInfo info = vars.items[i];
        if( info.key.kind == needle.kind ) {
            if( info.key.kind == VAR_TEMP && info.key.temp_id == needle.temp_id ) {
                *out = &vars.items[i];
                return 1;
            }
            if( info.key.kind == VAR_LOCAL && strcmp(info.key.ident, needle.ident) == 0 ) {
                *out = &vars.items[i];
                return 1;
            }
        }
    }
    return 0;
}
int get_var_info(VariableInfoDA vars, VariableInfo *out, TacVar needle) {
    if( needle.kind != VAR_TEMP && needle.kind != VAR_LOCAL ) {
        return 0;
        print_tac_var(needle);
        PANIC("NOT SUPPORTED");
    }
    for(int i = 0; i < vars.count; i++) {
        VariableInfo info = vars.items[i];
        if( info.key.kind == needle.kind ) {
            if( info.key.kind == VAR_TEMP && info.key.temp_id == needle.temp_id ) {
                *out = info;
                return 1;
            }
            if( info.key.kind == VAR_LOCAL && strcmp(info.key.ident, needle.ident) == 0 ) {
                *out = info;
                return 1;
            }
        }
    }
    return 0;
}
void gen_x86_fcall(StringBuilder *sb, ProcContext *context, TacInstr instr) {
    VariableInfo info;
    for(int i = 0; i < instr.fcall.args.count; i++) {
        TacVar arg = instr.fcall.args.items[i];
        if(!get_var_info(context->vars, &info, arg)) PANIC();
        DBG_ASSERT((info.location.type == REGISTER),"");
        if(info.location.register_location.register_ >= XMM0) {
            sb_append(sb,"sub rsp, 8\n");
            sb_append(sb,"movq [rsp], %s\n",get_register_str(info.location.register_location.register_));
        } else {
            sb_append(sb,"push %s\n",get_register_str(info.location.register_location.register_));
        }
    }
    sb_append(sb,"call %s\n",instr.fcall.ident);
    sb_append(sb,"add rsp, %d\n",instr.fcall.args.count * 8);

    if( instr.type != TAC_VOID ) {
        VariableInfo info;
        TacVar result = instr.fcall.result;
        if(!get_var_info(context->vars,&info,result)) PANIC();
        DBG_ASSERT((info.location.type != NOT_ASSIGNED),"");

        switch( instr.type ) {
            case TAC_PTR: 
            case TAC_I64: 
            case TAC_U64: 
                sb_append(sb,"mov %s, %s\n", get_location_str(info.location), get_register_str(RAX));
                break;
            case TAC_F64: 
                sb_append(sb,"movss %s, %s\n", get_location_str(info.location), get_register_str(XMM0));
                break;
            default: PANIC();
        }
    }
}
void gen_x86_sub(StringBuilder *sb, ProcContext *context, TacInstr instr) {
    //result is always a new temp
    TacVar arg1 = instr.binary.arg1;
    TacVar arg2 = instr.binary.arg2;
    TacVar result = instr.binary.result;
    VariableInfo *info;

    // sub reg1, reg2
    
    get_var_info_ref(context->vars,&info,arg1);
    DBG_ASSERT((info->location.type != NOT_ASSIGNED),"");
    Register arg1_reg = info->location.register_location.register_;

    get_var_info_ref(context->vars,&info,arg2);
    DBG_ASSERT((info->location.type != NOT_ASSIGNED),"");
    Register arg2_reg = info->location.register_location.register_;

    get_var_info_ref(context->vars,&info,result);
    DBG_ASSERT((info->location.type != NOT_ASSIGNED),"");

    switch( instr.type ) {
        case TAC_PTR: 
        case TAC_I64: 
        case TAC_U64: 
            sb_append(sb,"sub %s, %s\n", get_register_str(arg1_reg), get_register_str(arg2_reg));
            break;
        case TAC_F64: 
            sb_append(sb,"subsd %s, %s\n", get_register_str(arg1_reg), get_register_str(arg2_reg));
            break;
        default: PANIC();
    }

}

void gen_x86_ret(StringBuilder *sb, ProcContext *context, TacInstr instr) {
    TacVar src = instr.unary.src;
    VariableInfo info;

    if(!get_var_info(context->vars, &info, src)) PANIC();
    switch( instr.type ) {
        case TAC_PTR: 
        case TAC_I64: 
        case TAC_U64: 
            sb_append(sb,"mov %s, %s\n", get_register_str(RAX), get_location_str(info.location));
            break;
        case TAC_F64: 
            sb_append(sb,"movsd %s, %s\n", get_register_str(XMM0), get_location_str(info.location));
            break;
        default: PANIC();
    }
    sb_append(sb,"jmp L%d\n",context->return_label_number);
}

void gen_x86_add(StringBuilder *sb, ProcContext *context, TacInstr instr) {
    //result is always a new temp
    TacVar arg1 = instr.binary.arg1;
    TacVar arg2 = instr.binary.arg2;
    TacVar result = instr.binary.result;
    VariableInfo *info;

    // add reg1, reg2
    get_var_info_ref(context->vars,&info,arg1);
    DBG_ASSERT((info->location.type != NOT_ASSIGNED),"");
    Register arg1_reg = info->location.register_location.register_;

    get_var_info_ref(context->vars,&info,arg2);
    DBG_ASSERT((info->location.type != NOT_ASSIGNED),"");
    Register arg2_reg = info->location.register_location.register_;

    get_var_info_ref(context->vars,&info,result);
    DBG_ASSERT((info->location.type != NOT_ASSIGNED),"");

    switch( instr.type ) {
        case TAC_PTR: 
        case TAC_I64: 
        case TAC_U64: 
            sb_append(sb,"add %s, %s\n",   get_register_str(arg1_reg), get_register_str(arg2_reg));
            break;
        case TAC_F64: 
            sb_append(sb,"addsd %s, %s\n", get_register_str(arg1_reg), get_register_str(arg2_reg));
            break;
        default: PANIC();
    }

}

void gen_x86_mov(StringBuilder *sb, ProcContext *context, TacInstr instr, FloatDA *floats) {
    TacVar dest = instr.move.dest;
    TacVar src = instr.move.src;
    char* dest_str;
    char* src_str;
    
    VariableInfo *info;
    TacType type;

    if(get_var_info_ref(context->vars,&info,src)) {
        DBG_ASSERT((info->location.type != NOT_ASSIGNED),"");
        type = info->type;
        src_str = get_location_str(info->location);
    } else {
        static char temp_buff[64];
        switch(src.kind) {
            case VAR_CONST_UINT:
                type = TAC_U64;
                snprintf(temp_buff,64,"%u",src.uint_val);
                break;
            case VAR_CONST_INT:
                type = TAC_I64;
                snprintf(temp_buff,64,"%i",src.int_val);
                break;
            case VAR_CONST_FLOAT:
                type = TAC_F64;
                snprintf(temp_buff,64,"[float_const_%d]",floats->count);
                da_append_ref(floats,src.float_val);
                break;
            default: PANIC();
        }
        src_str = temp_buff;
    }

    if(!get_var_info_ref(context->vars,&info,dest)) PANIC();
    DBG_ASSERT((info->location.type != NOT_ASSIGNED),"");

    dest_str = get_location_str(info->location);

    switch( type ) {
        case TAC_PTR: 
        case TAC_I64: 
        case TAC_U64: 
            sb_append(sb, "mov %s, %s\n", dest_str, src_str);
            break;
        case TAC_F64: 
            sb_append(sb, "movsd %s, %s\n", dest_str, src_str);
            break;
        default: PANIC();
    }
}

int allocate_stack_space_for_locals(ProcContext *context){
    int curr_base_offset = 0; 

    // save space for non-volotile registers
    Registers regs = context->touched_registers;
    for(int i = 0; i < 32; i++) {
        Register reg = (Register)(1 << i);
        if( (regs & reg) != 0) {
            curr_base_offset += 8; // 8bytes/64bits
        }
    }

    for(int i = 0; i < context->vars.count; i++) {
        VariableInfo *curr_info = &context->vars.items[i];
        if(curr_info->key.kind != VAR_LOCAL)
            continue;
        if(curr_info->location.type != NOT_ASSIGNED)
            continue;

        //int arg_size = Type_size_of(arg_decl->argument_decl.type);
        int arg_size = 8;
        curr_base_offset += arg_size;

        curr_info->location = (VariableLocation){ 
            .type=STACK, 
            .stack_location.base_offset = -curr_base_offset
        };
    }
    int stack_space_needed = curr_base_offset;
    return stack_space_needed;
}

void print_var_infos(VariableInfoDA vars) {
    for(int i = 0; i < vars.count; i++) {
        VariableInfo curr_info = vars.items[i];
        printf("key: ");
        switch(curr_info.key.kind) {
            case VAR_TEMP:          printf("temp t%d",curr_info.key.temp_id);  break;
            case VAR_LOCAL:         printf("local %s",curr_info.key.ident);     break;
            case VAR_LABEL:         printf("label %s",curr_info.key.label_name);break;
            case VAR_CONST_INT:     printf("const %d",curr_info.key.int_val);   break;
            case VAR_CONST_UINT:    printf("const %d",curr_info.key.int_val);   break;
            case VAR_CONST_FLOAT:   printf("const %f",curr_info.key.float_val); break;
            default: PANIC();
        }
        printf(", location: ");
            switch( curr_info.location.type ) {
                case NOT_ASSIGNED:
                    printf("NOT_ASSIGNED"); break;
                case REGISTER:
                    printf("%s", get_register_str(curr_info.location.register_location.register_)); break;
                case STACK:
                    printf("qword [rbp%+d]", curr_info.location.stack_location.base_offset); break;
            }
        printf(", ");
        printf("type: ");
        switch(curr_info.type) {
            case TAC_VOID: printf("void"); break;
            case TAC_PTR: printf("ptr"); break;
            case TAC_I64: printf("i64"); break;
            case TAC_F64: printf("f64"); break;
            case TAC_U64: printf("u64"); break;
            default: PANIC();
        }
        printf(", first_line_used: %d, last_line_used: %d\n",curr_info.first_line_used,curr_info.last_line_used);
    }
}

void allocate_registers_for_temps(ProcContext *context, TacInstrDA instructions) {
    for(int i = 0; i < instructions.count; i++) {
        //if( i > 0 ) { //free later unused registers
        {
            int curr_line_number = i + 1;
            VariableInfo info = context->vars.items[i];
            if(info.location.type == REGISTER && info.last_line_used <= curr_line_number) {
                add_register(&context->available_registers, info.location.register_location.register_);
            }
        }
        //}

        TacInstr instr = instructions.items[i];
        // allocate register
        VariableInfo *info;
        TacVar var;
        switch(instr.op) {
            // Two operand ops
            case TAC_ADD: 
            case TAC_SUB: 
            case TAC_MUL: 
            case TAC_DIV: {
                TacVar arg1 = instr.binary.arg1; 
                TacVar result = instr.binary.result;
                DBG_ASSERT((arg1.kind == VAR_TEMP),"");

                if(!get_var_info_ref(context->vars,&info,arg1)) PANIC();
                 VariableLocation location = info->location;
                 remove_register(&context->available_registers,location.register_location.register_);

                if(!get_var_info_ref(context->vars,&info,result)) PANIC();
                 info->location = location;
            } break;

            // Three operand ops
            case TAC_CMP_EQ:
            case TAC_CMP_NE:
            case TAC_CMP_LT:
            case TAC_CMP_GT:
            case TAC_CMP_LE:
            case TAC_CMP_GE:
            case TAC_AND:     
            case TAC_OR: {
                TODO("ARE THEY THREE OPERAND OPS?");
                TacVar result = instr.binary.result;
                if(!get_var_info_ref(context->vars,&info,result)) PANIC();
                 Register reg = take_next_available_register_for_type(&context->available_registers,info->type);
                 add_register(&context->touched_registers,reg);
                 info->location = (VariableLocation) {
                    .type = REGISTER,
                    .register_location.register_ = reg,
                 };
            } break;
            case TAC_FCALL: {
                if( instr.type == TAC_VOID ) continue;
                TacVar result = instr.fcall.result; // can be void
                if(!get_var_info_ref(context->vars,&info,result)) PANIC();
                 Register reg = take_next_available_register_for_type(&context->available_registers,info->type);
                 add_register(&context->touched_registers,reg);
                 info->location = (VariableLocation) {
                    .type = REGISTER,
                    .register_location.register_ = reg,
                 };
            } break;
            case TAC_LOAD:
            case TAC_ADDR:
            case TAC_MOV: {
                TacVar dest = instr.move.dest;
                if( dest.kind == VAR_LOCAL ) continue;

                if(!get_var_info_ref(context->vars,&info,dest)) PANIC();
                 Register reg = take_next_available_register_for_type(&context->available_registers,info->type);
                 add_register(&context->touched_registers,reg);
                 info->location = (VariableLocation) {
                    .type = REGISTER,
                    .register_location.register_ = reg,
                 };
            } break;
            case TAC_RET:   // assuming src is already allocated
            case TAC_STORE: // Assuming the dest is already allocated
            case TAC_ALLOC: 
            case TAC_LABEL: 
            case TAC_JMP_IF_NOT: 
            case TAC_JMP_IF: 
            case TAC_JMP: 
                continue;
            default: PANIC();
        }
    }
}

void save_registers_to_stack(StringBuilder *sb, ProcContext context) {
    // GPRs
    int curr_base_offset = 0;
    for(int i = 0; i < 16; i++) {
        Register reg = (Register)(1 << i);
        if( (context.touched_registers & reg) != 0) {
            curr_base_offset += 8;
            sb_append(sb,"mov [rbp-%d], %s\n",curr_base_offset,get_register_str(reg));
        } 
    }
    // xmm registers
    for(int i = 16; i < 32; i++) {
        Register reg = (Register)(1 << i);
        if( (context.touched_registers & reg) != 0) {
            curr_base_offset += 8;
            sb_append(sb,"movq [rbp-%d], %s\n",curr_base_offset,get_register_str(reg));
        } 
    }
}
void populate_registers_from_stack(StringBuilder *sb, ProcContext context) {
    // xmm registers
    int curr_base_offset = 0;
    for(int i = 0; i < 16; i++) {
        Register reg = (Register)(1 << i);
        if( (context.touched_registers & reg) != 0) {
            curr_base_offset += 8;
            sb_append(sb,"mov %s, [rbp-%d]\n",get_register_str(reg),curr_base_offset);
        } 
    }
    // xmm registers
    for(int i = 16; i < 32; i++) {
        Register reg = (Register)(1 << i);
        if( (context.touched_registers & reg) != 0) {
            curr_base_offset += 8;
            sb_append(sb,"movq %s, [rbp-%d]\n",get_register_str(reg),curr_base_offset);
        } 
    }
}

void gen_x86_procedure(StringBuilder *sb, TacProc proc, FloatDA *floats) {
    VariableInfoDA variables = {0};

    int curr_base_offset = 8;
    for(int i = 0; i < proc.args.count; i++) {
        ProcArg arg = proc.args.items[i];  
        //int arg_size = Type_size_of(arg_decl->argument_decl.type);
        int arg_size = 8;
        curr_base_offset += arg_size;

        TacVar var = (TacVar){ 
            .kind=VAR_LOCAL, 
            .ident = arg.ident 
        };
        VariableLocation location = (VariableLocation){ 
            .type=STACK, 
            .stack_location.base_offset = curr_base_offset
        };
        da_append(variables,((VariableInfo){
            .key = var, 
            .type=arg.type, 
            .location = location,
            //.first_line_used = 0, // only needed for temp vars
            //.last_line_used = 0,
        }));
    }

    ProcContext context = { 
        .touched_registers = {0}, 
        .available_registers = { ~(u32)0 -RAX -RBX -RCX -RDX -RSI -RDI -RSP -RBP},
        .vars = variables,
        .return_label_number = CURR_JMP_LABEL_NUMBER++,
    };

    analyze_variable_lifetimes_and_types(proc.instructions,&context.vars);
    allocate_registers_for_temps(&context, proc.instructions);
    //print_all_regs(context.touched_registers);
    int stack_space_needed = allocate_stack_space_for_locals(&context);
    print_var_infos(context.vars);

    sb_append(sb,"%s:\n",proc.ident);
    // PROLOG
    sb_append(sb,"push rbp\n");
    sb_append(sb,"mov rbp, rsp\n");
    sb_append(sb,"sub rsp, %d\n",stack_space_needed);
    save_registers_to_stack(sb,context);

    printf(sb->buffer);

    for(int i = 0; i < proc.instructions.count; i++) {
        TacInstr curr_instr = proc.instructions.items[i];

        // ALL VARS ARE ALLOCATED
        switch(curr_instr.op) {
            case TAC_RET:       gen_x86_ret(sb, &context, curr_instr);  break;
            case TAC_MOV:       gen_x86_mov(sb, &context, curr_instr, floats);  break;
            case TAC_ADD:       gen_x86_add(sb, &context, curr_instr);  break;
            case TAC_SUB:       gen_x86_sub(sb, &context, curr_instr);  break;
            case TAC_MUL:       TODO();
            case TAC_DIV:       TODO();
            case TAC_FCALL:     gen_x86_fcall(sb, &context, curr_instr);break;
            case TAC_LABEL:     TODO();
            case TAC_JMP_IF:    TODO();
            case TAC_JMP_IF_NOT:TODO();
            case TAC_JMP:       TODO();
            case TAC_LOAD:      TODO();
            case TAC_STORE:     TODO();
            case TAC_ADDR:      TODO();
            case TAC_ALLOC:     TODO();
            case TAC_CMP_EQ:    TODO();
            case TAC_CMP_NE:    TODO();
            case TAC_CMP_LT:    TODO();
            case TAC_CMP_GT:    TODO();
            case TAC_CMP_LE:    TODO();
            case TAC_CMP_GE:    TODO();
            case TAC_AND:       TODO();
            case TAC_OR:        TODO();
            default: PANIC();
        }

    }

    // EPILOG
    sb_append(sb,"L%i:\n",context.return_label_number);
    populate_registers_from_stack(sb,context);
    // Stack frame cleanup; same as leave
    sb_append(sb,"mov rsp, rbp\n");
    sb_append(sb,"pop rbp \n");
    // return
    sb_append(sb,"ret \n");


}

void reset_globals() {
    CURR_JMP_LABEL_NUMBER = 0;
}

char* gen_x86_asm(TacProgram tac_program) {
    StringBuilder output_sb = sb_new();

    const char *nasm_header = 
        "default rel\n"
        "section .text\n"
            "global _start\n"

        "_start:\n"
        // # Call main function
        "call main\n"

        //# Exit the program
        "mov eax, 60\n"//     # syscall: exit
        "xor edi, edi\n"//    # status: 0
        "syscall\n"

        "; ===================== end of HEADER =================================\n" ;

    sb_append(&output_sb,nasm_header);
    
    for(int i = 0; i < tac_program.extern_declarations.count; i++ ) {
        sb_append(&output_sb, "extern %s\n",tac_program.extern_declarations.items[i]);
    }
    sb_append(&output_sb,"; ===================== end of EXTERN =================================\n");

    FloatDA floats = {0};
    for(int i = 0; i < tac_program.procedures.count; i++ ) {
        gen_x86_procedure(&output_sb, tac_program.procedures.items[i], &floats);
    }
    sb_append(&output_sb,"; ===================== end of TEXT =================================\n");
    for(int i = 0; i < floats.count; i++) {
       sb_append(&output_sb,"float_const_%d: dq %f\n", i, floats.items[i]);
    }
    sb_append(&output_sb,"; ===================== end of DATA =================================\n");

    return output_sb.buffer;
}
