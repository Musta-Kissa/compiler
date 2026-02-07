#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

typedef struct TypeListNode TypeListNode;
typedef struct FieldListNode FieldListNode;

typedef enum TypeKind {
    FUNCTION_TYPE,

    STRUCT_TYPE,
    ENUM_TYPE,
    UNION_TYPE,

    POINTER_TYPE,
    ARRAY_TYPE,

    BOOL_TYPE,
    VOID_TYPE,
    UNKNOWN_TYPE,

    INTIGER_TYPE,
    FLOAT_TYPE,
} TypeKind;

typedef enum VariableSize {
    BITS_8 = 8,
    BITS_16 = 16,
    BITS_32 = 32,
    BITS_64 = 64,
} VariableSize;

typedef struct Type {
    TypeKind type_kind;
    const char* type_name;
    union {
        struct FunctionType {
            struct Type* return_type;
            TypeListNode* arg_types;
        } function_type;
        struct StructType{
            FieldListNode* fields;
        } struct_type;
        struct PointerType{
            struct Type* sub_type;
        } pointer_type;
        struct ArrayType{
            struct Type* sub_type;
            long length; // 0 = len not specified
        } array_type;
        struct IntigerType{
            VariableSize size;
        } intiger_type;
        struct FloatType{
            VariableSize size;
        } float_type;
        struct BoolType{
            VariableSize size;
        } bool_type;
    };
} Type;

struct FieldListNode {
    Type type;
    char* name;
    FieldListNode* next; // Can be NULL
};

struct TypeListNode {
    Type type;
    TypeListNode* next; // Can be NULL
};

Type* Type_alloc_type(Type type);
Type Type_new(char* type_name, TypeKind type_kind);
Type Type_get_field_type(Type type,char* field_name);
const char* Type_format_type_kind(Type type);
int Type_cmp(Type* type1, Type* type2);
int Type_is_lvalue(Type* type);
int Type_size_of(Type* type);


#include "my_string.h"

void Type_build_type_string(StringBuilder* sb, Type* type );

#define VOID_TYPE_IDX       0
#define BOOL_TYPE_IDX       1
#define INTIGER_TYPE_IDX    2
#define FLOAT_TYPE_IDX      3

#define PRIMITIVE_TYPES_ARRAY() { \
    {.type_kind = VOID_TYPE, .type_name = "void"}, \
    {.type_kind = BOOL_TYPE, .type_name = "bool", .bool_type.size = BITS_32 }, \
    {.type_kind = INTIGER_TYPE, .type_name = "int", .intiger_type.size = BITS_32 }, \
    {.type_kind = FLOAT_TYPE, .type_name = "float", .float_type.size = BITS_32 }, \
}; \

#endif
