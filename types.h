#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

typedef struct TypeListNode TypeListNode;
typedef struct FieldListNode FieldListNode;

typedef enum TypeKind {
    FUNCTION_TYPE,
    STRUCT_TYPE,
    PRIMITIVE_TYPE,
    ENUM_TYPE,
    UNION_TYPE,
    POINTER_TYPE,
    //UNKNOWN_TYPE,
} TypeKind;

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

Type Type_new(char* type_name, TypeKind type_kind);
Type Type_get_field_type(Type type,char* field_name);
const char* Type_format_type_kind(Type type);
int Type_cmp(Type* type1, Type* type2);


#include "my_string.h"

void Type_build_type_string(StringBuilder* sb, Type* type );

#define BOOL_TYPE_IDX   0
#define FLOAT_TYPE_IDX  1
#define INT_TYPE_IDX    2
#define VOID_TYPE_IDX   3
#define STRING_TYPE_IDX 4

#define PRIMITIVE_TYPES_ARRAY() { \
    {.type_kind = PRIMITIVE_TYPE, .type_name = "bool"}, \
    {.type_kind = PRIMITIVE_TYPE, .type_name = "float"}, \
    {.type_kind = PRIMITIVE_TYPE, .type_name = "int"}, \
    {.type_kind = PRIMITIVE_TYPE, .type_name = "void"}, \
    {.type_kind = PRIMITIVE_TYPE, .type_name = "string"}, \
}; \

#endif
