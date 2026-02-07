#include<stdlib.h>

#define make_da(type) \
int cap; \
int count; \
type* items;

#define da_append(arr,item) \
do { \
    if(arr.cap <= arr.count) { \
        if(arr.cap == 0) { \
            arr.cap = 16; \
        } else { \
            arr.cap *= 2; \
        } \
        arr.items = realloc(arr.items, arr.cap * sizeof(arr.items[0])); \
    } \
    arr.items[arr.count++] = item; \
} while(0)

#define da_append_ref(arr,item) \
do { \
    if(arr->cap <= arr->count) { \
        if(arr->cap == 0) { \
            arr->cap = 16; \
        } else { \
            arr->cap *= 2; \
        } \
        arr->items = realloc(arr->items, arr->cap * sizeof(arr->items[0])); \
    } \
    arr->items[arr->count++] = item; \
} while(0)

#define da_pop(arr) arr.items[--arr.count]

#define da_unordered_remove(arr,idx) \
do { \
    if( arr.count > 0 ) { \
        arr.items[idx] = arr.items[--arr.count]; \
    } \
} while(0)

#define da_unordered_remove_ref(arr,idx) \
do { \
    if( arr->count > 0 ) { \
        arr->items[idx] = arr->items[--arr->count]; \
    } \
} while(0)
