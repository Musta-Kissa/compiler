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
    printf("\033[1;31m" fmt "\033[0m" "\n", ##__VA_ARGS__); \
    __builtin_trap(); \
    exit(-1); \
}

Type Type_new(char* type_name, TypeKind type_kind) {
    return (Type){ .type_name=type_name,.type_kind=type_kind };
}
Type* Type_get_field_type(Type* type,char* field_name) {
    ASSERT( (type->type_kind == STRUCT_TYPE), "Expected STRUCT_TYPE");

    FieldListNode* curr = type->struct_type.fields;
    while( curr != NULL ) {
        if( strcmp(curr->name,field_name) == 0 ) {
            return curr->type;
        }
        curr = curr->next;
    }
    PANIC("Field not found '%s' in struct {%s}",field_name,type->type_name);
}

const char* Type_format_type_kind(Type type) {
    switch( type.type_kind ) {
        case FUNCTION_TYPE:     return "FUNCTION_TYPE";
        case STRUCT_TYPE:       return "STRUCT_TYPE";
        case ENUM_TYPE:         return "ENUM_TYPE";
        case UNION_TYPE:        return "UNION_TYPE";
        case POINTER_TYPE:      return "POINTER_TYPE";
        case ARRAY_TYPE:        return "ARRAY_TYPE";
        case BOOL_TYPE:         return "BOOL_TYPE";
        case VOID_TYPE:         return "VOID_TYPE";
        case UNKNOWN_TYPE:      return "UNKNOWN_TYPE";
        case INTIGER_TYPE:      return "INTIGER_TYPE";
        case FLOAT_TYPE:        return "FLOAT_TYPE";
        default:
            PANIC("Unhandled Case")
    }


}
Type* Type_alloc_type(Type type) {
    Type* out = (Type*)malloc(sizeof(Type));
    *out = type;
    return out;
}
char* format_type(Type type) {
    PANIC("Not implemented");
    switch( type.type_kind ) {
        case STRUCT_TYPE:       return "STRUCT_TYPE";
        case ENUM_TYPE:         return "ENUM_TYPE";
        case UNION_TYPE:        return "UNION_TYPE";
            return type.type_name;
        case POINTER_TYPE:      return "POINTER_TYPE";
            return type.pointer_type.sub_type->type_name;
        case FUNCTION_TYPE:     return "FUNCTION_TYPE";
        default:
            PANIC("%s %d:Unhandled Case",__FILE__,__LINE__);
    }
}
// 0 = NOT THE SAME, 1 = THE SAME 
int Type_cmp(Type* type1, Type* type2) {
    if( type1->type_kind != type2->type_kind ) {
        return 0;
    }
    switch( type1->type_kind ) {
        case STRUCT_TYPE:
        case ENUM_TYPE:
        case UNION_TYPE:
            PANIC("%s %d:NOT IMPLEMENTED",__FILE__,__LINE__);
        case POINTER_TYPE:
            return Type_cmp(type1->pointer_type.sub_type,type2->pointer_type.sub_type);
        case ARRAY_TYPE:
            PANIC("%s %d:UNREACHABLE",__FILE__,__LINE__);
        case FUNCTION_TYPE:
            PANIC("Function types comparison is not implemented");
        case BOOL_TYPE:
        case VOID_TYPE:
        case INTIGER_TYPE:
        case FLOAT_TYPE:
            return 1;
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
        //case PRIMITIVE_TYPE:
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
            if(type->array_type.length == -1 ) {
                sb_append(sb,"[]");
            } else {
                sb_append(sb,"[%d]",type->array_type.length);
            }
            Type_build_type_string(sb,type->pointer_type.sub_type);
            return;
        case FUNCTION_TYPE:
            // ( {args}+ ) -> {return_type}
            TypeListNode* arg = type->function_type.arg_types;
            sb_append(sb,"( ");
            if( arg != NULL ) {
                while(1) {
                    Type_build_type_string(sb,arg->type);
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
        case BOOL_TYPE:
            sb_append(sb,"BOOL_TYPE");
            return;
        case VOID_TYPE:
            sb_append(sb,"VOID_TYPE");
            return;
        case INTIGER_TYPE:
            sb_append(sb,"INTIGER_TYPE");
            return;
        case FLOAT_TYPE:
            sb_append(sb,"FLOAT_TYPE");
            return;
        default:
            PANIC("%s %d:Unhandled Case %s",__FILE__,__LINE__,Type_format_type_kind(*type));
    }
}

int Type_size_of(Type* type) {
    switch( type->type_kind ) {
        case INTIGER_TYPE:
            return type->intiger_type.size / 8;
        case FLOAT_TYPE:
            return type->float_type.size / 8;
        case BOOL_TYPE:
            return type->bool_type.size / 8;
        case POINTER_TYPE:
            return 8;
        default:
            PANIC("%s %d: Not suported",__FILE__,__LINE__);
    }
}
