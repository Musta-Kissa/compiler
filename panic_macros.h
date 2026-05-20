#define ASSERT(expr, fmt, ...) { \
    if (!expr) { \
        printf("\033[1;31m%s %d: " fmt "\033[0m" "\n",__FILE__,__LINE__, ##__VA_ARGS__); \
        __builtin_trap(); \
    } \
}
#ifdef DEBUG_RUNTIME_CHECKS
    #define DBG_ASSERT(expr, fmt, ...) { \
        if (!expr) { \
            printf("\033[1;31m%s %d: " fmt "\033[0m" "\n",__FILE__,__LINE__, ##__VA_ARGS__); \
            __builtin_trap(); \
        } \
    }
#else
    #define DBG_ASSERT(expr, fmt, ...) ((void)0)
#endif

#define PANIC(fmt, ...) { \
    printf("\033[1;31m%s %d: " fmt "\033[0m" "\n",__FILE__,__LINE__, ##__VA_ARGS__); \
    __builtin_trap(); \
    exit(-1); \
}
#define TODO(fmt, ...) { \
    printf("\033[1;33m%s %d: " "TODO: " fmt "\033[0m" "\n",__FILE__,__LINE__, ##__VA_ARGS__); \
    __builtin_trap(); \
    exit(-1); \
}
