#include <stdint.h>
#include <stdio.h>
#include "panic_macros.h"
#include "types.h"
#include "registers.h"

#include "stdlib.h"

Registers Registers_new() {
    return ~(u32)0;
}


char* get_register_str(Register reg) {
    switch( reg ){
        default:  return "NOT_A_REGISTER";
        case RAX: return "rax";
        case RBX: return "rbx";
        case RCX: return "rcx";
        case RDX: return "rdx";
        case RSI: return "rsi";
        case RDI: return "rdi";
        case RSP: return "rsp";
        case RBP: return "rbp";
        case R8:  return "r8";
        case R9:  return "r9";
        case R10: return "r10";
        case R11: return "r11";
        case R12: return "r12";
        case R13: return "r13";
        case R14: return "r14";
        case R15: return "r15";

        case XMM0: return "XMM0";
        case XMM1: return "XMM1";
        case XMM2: return "XMM2";
        case XMM3: return "XMM3";
        case XMM4: return "XMM4";
        case XMM5: return "XMM5";
        case XMM6: return "XMM6";
        case XMM7: return "XMM7";
        case XMM8: return "XMM8";
        case XMM9: return "XMM9";
        case XMM10: return "XMM10";
        case XMM11: return "XMM11";
        case XMM12: return "XMM12";
        case XMM13: return "XMM13";
        case XMM14: return "XMM14";
        case XMM15: return "XMM15";
    }
}

/*
// assuming the str is no longer then 4 chars
u64 encode_string(const char *str) {
    u64 encoded = 0;
    for(int i = 0; str[i] != '\0' ; i++){
        encoded |= (uint64_t)(unsigned char)str[i]; 
        encoded <<= 8;
    }
    return encoded;
}
Register register_from_str(char* reg_str) {
    u64 encoded_str = encode_string(reg_str);
    switch( encoded_str ){
        default:         return 0;
        case 1918990336: return RAX;
        case 1919055872: return RBX;
        case 1919121408: return RCX;
        case 1919186944: return RDX;
        case 1920166144: return RSI;
        case 1919183104: return RDI;
        case 1920167936: return RSP;
        case 1919053824: return RBP;

        case 7485440:    return R8;
        case 7485696:    return R9;
        case 1915826176: return R10;
        case 1915826432: return R11;
        case 1915826688: return R12;
        case 1915826944: return R13;
        case 1915827200: return R14;
        case 1915827456: return R15;
    }
}
*/

Register take_next_available_register(Registers* regs) {
    if( regs == 0 ) return 0;

    for(int i = 0; i < 16; i++) {
        Register reg = (Register)(1 << i);
        if( (*regs & reg) != 0) {
            *regs &= ~reg;
            return reg;
        }
    }
    return 0;
}

Register take_next_available_register_for_type(Registers* regs, Type* type) {
    if( regs == 0 ) return 0;

    switch( type->type_kind ) {
        case POINTER_TYPE:
        case INTIGER_TYPE: {
            for(int i = 0; i < 16; i++) {
                Register reg = (Register)(1 << i);
                if( (*regs & reg) != 0) {
                    *regs &= ~reg;
                    return reg;
                }
            }
            return 0;
        }
        case FLOAT_TYPE: {
            for(int i = 16; i < 32; i++) {
                Register reg = (Register)(1 << i);
                if( (*regs & reg) != 0) {
                    *regs &= ~reg;
                    return reg;
                }
            }
            return 0;
        }
        default: PANIC("TYPE NOT SUPPORTED");
    }
}

inline void remove_register(Registers* regs, Register reg) {
    *regs &= ~reg;
}
inline void add_register(Registers* regs, Register reg) {
    *regs |= reg;
}




//=====================================================================


void print_all_regs(Registers regs) {
    for(int i = 0; i < 16; i++) {
        Register reg = (Register)(1 << i);
        printf("%s ",get_register_str(reg));
        if( (regs & reg) != 0) {
            printf("■ |");
        } else {
            printf("□ |");
        }
    }
    printf("\n");
}

/*
int main() {
    printf("%d\n",sizeof(Register)); 

    Registers regs = ~0;
    print_all_regs(regs);

    remove_register(&regs,register_from_str("rbx"));
    remove_register(&regs,register_from_str("rdx"));
    print_all_regs(regs);

    while(1) {
        Register reg = take_next_available_register(&regs);
        if(reg == 0) {
            break;
        }
        printf("%s\n",get_register_str(reg));
        print_all_regs(regs);
    }
    printf("====================\n");
    add_register(&regs,register_from_str("r8"));
    print_all_regs(regs);
    Register reg = take_next_available_register(&regs);
    printf("%s\n",get_register_str(reg));
    print_all_regs(regs);
}
*/
