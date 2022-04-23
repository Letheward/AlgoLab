#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>


/*

    WARNING: PLEASE, don't write stuff like this in real codebase

*/


/* ==== Builtin Types ==== */

typedef unsigned char           u8;
typedef unsigned short int      u16;
typedef unsigned int            u32;
typedef unsigned long long int  u64;
typedef signed char             s8;
typedef signed short            s16;
typedef signed int              s32;
typedef signed long long int    s64;
typedef float                   f32;
typedef double                  f64;

typedef struct {
    u8* data; 
    u64 count;
} String;

void print(String s) {
    for (u64 i = 0; i < s.count; i++) putchar(s.data[i]);
}



#define length_of(array)    (sizeof(array) / sizeof(array[0]))
#define string(s)           (String) {(u8*) s, sizeof(s) - 1}
#define array(Type, array)  (Array(Type)) {array, length_of(array)} 

#define Array(Type) Array_ ## Type
#define Define_Array(Type) \
typedef struct {           \
    Type* data;            \
    u64   count;           \
} Array(Type)              \

#define DynamicArray(Type) DynamicArray_ ## Type
#define Define_DynamicArray(Type)           \
typedef struct {                            \
    Array(Type) base;                       \
    u64   allocated;                        \
    void* (*alloc)(u64 count);              \
    void* (*resize)(void* data, u64 count); \
    void  (*free)(void* data);              \
} DynamicArray(Type)                        \

#define array_init(Type) array_init_ ## Type
#define Define_array_init(Type)                   \
DynamicArray(Type) array_init_ ## Type () {       \
    return (DynamicArray(Type)) {                 \
        .base.data  = calloc(2, sizeof(Type)),    \
        .base.count = 0,                          \
        .allocated  = 2,                          \
        .alloc      = malloc,                     \
        .resize     = realloc,                    \
        .free       = free,                       \
    };                                            \
}                                                 \

#define array_add_(Type) array_add_ ## Type
#define array_add(T, V) _Generic((T),           \
    DynamicArray(int)*    : array_add_(int),    \
    DynamicArray(float)*  : array_add_(float),  \
    DynamicArray(String)* : array_add_(String)  \
)(T, V);                                        \

#define Define_array_add(Type)                                                   \
void array_add_ ## Type(DynamicArray(Type)* a, Type item) {                      \
                                                                                 \
    u64 wanted = a->base.count + 1;                                              \
    if (wanted > a->allocated) {                                                 \
        a->base.data = a->resize(a->base.data, a->allocated * 2 * sizeof(Type)); \
        a->allocated *= 2;                                                       \
    }                                                                            \
                                                                                 \
    a->base.data[a->base.count] = item;                                          \
    a->base.count = wanted;                                                      \
}                                                                                \

#define selection_sort_(Type) selection_sort_ ## Type
#define selection_sort(T, C) _Generic((T),   \
    Array(int)    : selection_sort_(int),    \
    Array(float)  : selection_sort_(float),  \
    Array(String) : selection_sort_(String)  \
)(T, C);                                     \

#define Define_selection_sort(Type)                                            \
void selection_sort_ ## Type (Array(Type) in, u8 (*compare)(Type a, Type b)) { \
                                                                               \
    for (u64 i = 0; i < in.count - 1; i++) {                                   \
                                                                               \
        u64 min = i;                                                           \
        for (u64 j = i + 1; j < in.count; j++) {                               \
            if (compare(in.data[j], in.data[min]))  min = j;                   \
        }                                                                      \
                                                                               \
        if (min != i) {                                                        \
            Type a = in.data[i  ];                                             \
            Type b = in.data[min];                                             \
            in.data[i  ] = b;                                                  \
            in.data[min] = a;                                                  \
        }                                                                      \
    }                                                                          \
}                                                                              \


Define_Array(int);
Define_Array(float);
Define_Array(String);

Define_DynamicArray(int);
Define_DynamicArray(float);
Define_DynamicArray(String);

Define_array_add(int)
Define_array_add(float)
Define_array_add(String)

Define_array_init(int)
Define_array_init(float)
Define_array_init(String)

Define_selection_sort(int)
Define_selection_sort(float)
Define_selection_sort(String)



#define print_array_(Type) print_array_ ## Type
#define print_array(T) _Generic((T),     \
    Array(int)    : print_array_(int),   \
    Array(float)  : print_array_(float), \
    Array(String) : print_array_(String) \
)(T);                                    \

void print_array_(int)(Array(int) a) {
    for (u64 i = 0; i < a.count; i++) printf("%d ", a.data[i]);
    printf("\n");
}

void print_array_(float)(Array(float) a) {
    for (u64 i = 0; i < a.count; i++) printf("%f ", a.data[i]);
    printf("\n");
}

void print_array_(String)(Array(String) a) {
    for (u64 i = 0; i < a.count; i++) {
        print(a.data[i]);
        printf("\n");
    }
}

#undef Define_Array
#undef Define_DynamicArray
#undef Define_array_add
#undef Define_array_init
#undef Define_selection_sort




u8 string_compare_length(String a, String b) {
    return a.count < b.count;
}

u8 int_compare_ascend(int a, int b) {
    return a < b;
}

u8 float_compare_ascend(float a, float b) {
    return a < b;
}

int main() {

    DynamicArray(int)    a = array_init(int)();
    DynamicArray(float)  b = array_init(float)();
    DynamicArray(String) c = array_init(String)();
    
    for (int i = 0; i < 32; i++)  array_add(&a, rand() % 128);
    for (int i = 0; i <  5; i++)  array_add(&b, (float) rand() / (float) rand());

    array_add(&c, string("Help!!! "));
    array_add(&c, string("I'm "));
    array_add(&c, string("stuck in "));
    array_add(&c, string("the programming "));
    array_add(&c, string("language C!!!"));
    
    print_array(a.base);
    print_array(b.base);
    print_array(c.base);

    printf("\n");

    selection_sort(a.base, int_compare_ascend);
    selection_sort(b.base, float_compare_ascend);
    selection_sort(c.base, string_compare_length);
    
    print_array(a.base);
    print_array(b.base);
    print_array(c.base);

}



