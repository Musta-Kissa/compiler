#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include "types.h"

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i8 b8;
typedef i32 b32;

typedef u32 Registers;

typedef enum {
    RAX = 1 << 0,
    RBX = 1 << 1,
    RCX = 1 << 2,
    RDX = 1 << 3,
    RSI = 1 << 4,
    RDI = 1 << 5,
    RSP = 1 << 6,
    RBP = 1 << 7,
    R8 =  1 << 8,
    R9 =  1 << 9,
    R10 = 1 << 10,
    R11 = 1 << 11,
    R12 = 1 << 12,
    R13 = 1 << 13,
    R14 = 1 << 14,
    R15 = 1 << 15,

    XMM0 = 1 << 16,
    XMM1 = 1 << 17,
    XMM2 = 1 << 18,
    XMM3 = 1 << 19,
    XMM4 = 1 << 20,
    XMM5 = 1 << 21,
    XMM6 = 1 << 22,
    XMM7 = 1 << 23,
    XMM8 = 1 << 24,
    XMM9 = 1 << 25,
    XMM10 = 1 << 26,
    XMM11 = 1 << 27,
    XMM12 = 1 << 28,
    XMM13 = 1 << 29,
    XMM14 = 1 << 30,
    XMM15 = 1 << 31,
} Register;

const inline Registers Registers_new();
u64 encode_string(const char *str);
char* get_register_str(Register reg);
// assuming the str is no longer then 4 chars
//Register register_from_str(char* reg_str);
Register take_next_available_register(Registers* regs);
Register take_next_available_register_for_type(Registers* regs, TacType type);
void remove_register(Registers* regs, Register reg);
void add_register(Registers* regs, Register reg);
void print_all_regs(Registers regs);

#endif
