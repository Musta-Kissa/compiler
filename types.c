#include "types.h"
#include "my_string.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define ASSERT(expr, fmt, ...) { \
    if (!expr) { \
        printf(fmt "\n", ##__VA_ARGS__); \
        exit(-1); \
    } \
}

#define PANIC(fmt, ...) { \
    printf(fmt "\n", ##__VA_ARGS__); \
    exit(-1); \
}

Type Type_new(char* type_name, TypeKind type_kind) {
    return (Type){ .type_name=type_name,.type_kind=type_kind };
}
Type Type_get_field_type(Type type,char* field_name) {
    ASSERT( (type.type_kind == STRUCT_TYPE), "Expected STRUCT_TYPE");

    FieldListNode* curr = type.struct_type.fields;
    while( curr != NULL ) {
        if( strcmp(curr->name,field_name) == 0 ) {
            return curr->type;
        }
        curr = curr->next;
    }
    PANIC("Field not found '%s' in struct {%s}",field_name,type.type_name);
}

const char* Type_format_type_kind(Type type) {
    switch( type.type_kind ) {
        case FUNCTION_TYPE:     return "FUNCTION_TYPE";
        case STRUCT_TYPE:       return "STRUCT_TYPE";
        case PRIMITIVE_TYPE:    return "PRIMITIVE_TYPE";
        case ENUM_TYPE:         return "ENUM_TYPE";
        case UNION_TYPE:        return "UNION_TYPE";
        case POINTER_TYPE:      return "POINTER_TYPE";
    }
}
char* format_type(Type type) {
    PANIC("Not implemented");
    switch( type.type_kind ) {
        case STRUCT_TYPE:       return "STRUCT_TYPE";
        case PRIMITIVE_TYPE:    return "PRIMITIVE_TYPE";
        case ENUM_TYPE:         return "ENUM_TYPE";
        case UNION_TYPE:        return "UNION_TYPE";
            return type.type_name;
        case POINTER_TYPE:      return "POINTER_TYPE";
            return type.pointer_type.sub_type->type_name;
        case FUNCTION_TYPE:     return "FUNCTION_TYPE";
    }
}
// 0 = NOT THE SAME, 1 = THE SAME
int Type_cmp(Type* type1, Type* type2) {
    if( type1->type_kind != type2->type_kind ) {
        return 0;
    }
    switch( type1->type_kind ) {
        case PRIMITIVE_TYPE:
            if( strcmp(type1->type_name,type2->type_name) == 0 ) {
                return 1;
            } else {
                return 0;
            }
        case STRUCT_TYPE:
        case ENUM_TYPE:
        case UNION_TYPE:
            return 1;
        case POINTER_TYPE:
            return Type_cmp(type1->pointer_type.sub_type,type2->pointer_type.sub_type);
        case ARRAY_TYPE:
            return Type_cmp(type1->array_type.sub_type,type2->array_type.sub_type);
        case FUNCTION_TYPE:
            PANIC("Function types comparison is not implemented");
        default:
            PANIC("%s %d:PANICKED",__FILE__,__LINE__);
    }
}

void Type_build_type_string(StringBuilder* sb, Type* type ){
    if( type == NULL ) {
        return;
    }
    switch( type->type_kind ) {
        case STRUCT_TYPE:
        case PRIMITIVE_TYPE:
        case ENUM_TYPE:
        case UNKNOWN_TYPE:
        case UNION_TYPE:
            sb_append(sb,"%s",type->type_name);
            return;
        case POINTER_TYPE:
            sb_append(sb,"*");
            Type_build_type_string(sb,type->pointer_type.sub_type);
            return;
        case ARRAY_TYPE:
            sb_append(sb,"[]");
            Type_build_type_string(sb,type->pointer_type.sub_type);
            return;
        case FUNCTION_TYPE:
            // ( {args}+ ) -> {return_type}
            TypeListNode* arg = type->function_type.arg_types;
            sb_append(sb,"( ");
            if( arg != NULL ) {
                while(1) {
                    Type_build_type_string(sb,&arg->type);
                    arg = arg->next; 
                    if( arg == NULL) 
                        break;
                    sb_append(sb,", ");
                }
            } else {
               sb_append(sb,"( ");
            }
            sb_append(sb,") -> ");
            Type_build_type_string(sb,type->function_type.return_type);
            return;
        case NUMBER_TYPE:
            sb_append(sb,"NUMBER_TYPE");
            Type_build_type_string(sb,type->pointer_type.sub_type);
            return;
        case BOOL_TYPE:
            sb_append(sb,"BOOL_TYPE");
            Type_build_type_string(sb,type->pointer_type.sub_type);
            return;
        default:
            PANIC("%s %d:PANICKED",__FILE__,__LINE__);
    }
}
int Type_is_lvalue(Type* type){
    switch( type->type_kind ) {
        case ARRAY_TYPE:
        case STRUCT_TYPE:
        case UNION_TYPE:
        case ENUM_TYPE:
        case PRIMITIVE_TYPE:
            return true;

        case NUMBER_TYPE:
        case BOOL_TYPE:
        case POINTER_TYPE:
        case FUNCTION_TYPE:
            return false;


        case UNKNOWN_TYPE:
        default:
            PANIC("%s %d: PANICKED",__FILE__,__LINE__);
    }
}
