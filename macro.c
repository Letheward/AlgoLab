#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>




/* ==== Types ==== */

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





/* ==== Basics ==== */

// this is needed for nested concat
// so, Array(Array(String)) will first expand to Array(Array_String),
// not Array_Array(String) (wrong and will not compile)
#define macro_concat(a, b) a ## b

// utilities
#define string(s)            (String) {(u8*) s, sizeof(s) - 1}           // make a String from a c string (char*) 
#define array_string(s)      (String) {(u8*) s, sizeof(s)}               // make a String from an array, like u8 data[] = {1, 2, 3, 4, 5}
#define data_string(s)       (String) {(u8*) &s, sizeof(s)}              // make a String from a value, like float v = 1.37;
#define length_of(array)     (sizeof(array) / sizeof(array[0]))
#define array(Type, c_array) (Array(Type)) {c_array, length_of(c_array)} // convert C stack array to Array(Type)

#define Array(Type) macro_concat(Array_, Type)
#define Define_Array(Type) \
typedef struct {           \
    Type* data;            \
    u64   count;           \
} Array(Type)              \

#define DynamicArray(Type) macro_concat(DynamicArray_, Type)
#define Define_DynamicArray(Type) \
typedef struct {                  \
    Array(Type) base;             \
    u64         allocated;        \
} DynamicArray(Type)              \

// Array() cannot handle something like Thing*, so this is for that 
#define Pointer(Type) macro_concat(Pointer_Of_, Type)
#define Define_Pointer(Type) typedef Type* Pointer(Type);




/* ==== Strings ==== */

void print(String s) {
    for (u64 i = 0; i < s.count; i++) putchar(s.data[i]);
}

void string_test() {
    
    printf("==== String Test ====\n");
    
    u8  a[] = {87, 111, 114, 108, 100, 33, 10};
    u32 b   = 0x74736554;

    print(string("Hello "));
    print(array_string(a));
    print(data_string(b));

    printf("\n\n");
}





/* ==== Raw Bits and Type-Punning ==== */

#define bit8( Type, name) ((union {Type a; u8  b;}) {.a = name}).b
#define bit16(Type, name) ((union {Type a; u16 b;}) {.a = name}).b
#define bit32(Type, name) ((union {Type a; u32 b;}) {.a = name}).b
#define bit64(Type, name) ((union {Type a; u64 b;}) {.a = name}).b

#define from_bit8( Type, name) ((union {Type a; u8  b;}) {.b = name}).a
#define from_bit16(Type, name) ((union {Type a; u16 b;}) {.b = name}).a
#define from_bit32(Type, name) ((union {Type a; u32 b;}) {.b = name}).a
#define from_bit64(Type, name) ((union {Type a; u64 b;}) {.b = name}).a


// test
void raw_bits_and_type_punning() {

    printf("==== Raw Bits and Type-Punning ====\n");
    
    // common usage
    {
        f32 a = 42;
        printf("%.8x\n", bit32(f32, a));
        
        // literal also works
        printf("%.8x\n",    bit32(f32, 1));
        printf("%.16llx\n", bit64(f64, 1));

        printf("%f\n", from_bit32(f32, 0x42280000));
    }

    // partial data
    {
        f32 a = 42;
        printf("%p\n", (void*) &a);
        printf("%.8x\n", bit32(f32*, &a));
    }

    printf("\n");
}




/* ==== Nested Types ==== */

// test
void nested_types() {

    printf("==== Nested Types ====\n");

    Define_Pointer(String);
    Define_Array(Pointer(String));
    Define_Array(Array(Pointer(String)));
    Define_DynamicArray(Array(Pointer(String)));

    DynamicArray(Array(Pointer(String))) test = {0};
    printf("%llu\n\n", test.allocated); // just to shut off unused warning
}






/* ==== Safe Varargs ==== */

// Note: Do not actually use this.
// Just make array functions and write stack array manually at call site (then it can also return values)

#define macro_internal(name)  __do_not_use__macro_internal__ ## name
#define add_s32(...)                                                       \
{                                                                          \
    int macro_internal(args)[] = {__VA_ARGS__};                            \
    add_s32_helper(macro_internal(args), length_of(macro_internal(args))); \
}                                                                          \

// can not return values
void add_s32_helper(s32* data, u64 count) {
    s32 n = 0;
    for (u64 i = 0; i < count; i++) {
        n += data[i];
    }
    printf("%d\n", n);
}


// test
void safe_varargs() {

    printf("==== Safe Varargs ====\n");

    add_s32(1, 2, 3);
    add_s32(1, 2, 3, 4, 5);
    // add_s32(1, 2, 3, 4, "this will give warning");

    printf("\n");
}







/* ==== Dynamic Array, Generics, and Sort ==== */

Define_Array(s32);
Define_Array(f32);
Define_Array(String);

Define_DynamicArray(s32);
Define_DynamicArray(f32);
Define_DynamicArray(String);


// WARNING: 
// these are really hard to read and maintain, 
// and will blow up compile time just like C++ templates,
// DO NOT use.

#define array_init(Type) array_init_ ## Type ()
#define Define_array_init(Type)                   \
DynamicArray(Type) array_init_ ## Type () {       \
    return (DynamicArray(Type)) {                 \
        .base.data  = calloc(32, sizeof(Type)),   \
        .base.count = 0,                          \
        .allocated  = 32,                         \
    };                                            \
}                                                 \

#define Define_array_add(Type)                                                   \
void array_add_ ## Type(DynamicArray(Type)* a, Type item) {                      \
                                                                                 \
    u64 wanted = a->base.count + 1;                                              \
    if (wanted > a->allocated) {                                                 \
        a->base.data = realloc(a->base.data, a->allocated * 2 * sizeof(Type));   \
        a->allocated *= 2;                                                       \
    }                                                                            \
                                                                                 \
    a->base.data[a->base.count] = item;                                          \
    a->base.count = wanted;                                                      \
}                                                                                \

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


#define array_add(T, V) _Generic((T),         \
    DynamicArray(s32)*    : array_add_s32,    \
    DynamicArray(f32)*    : array_add_f32,    \
    DynamicArray(String)* : array_add_String  \
)(T, V);                                      \

#define selection_sort(T, C) _Generic((T), \
    Array(s32)    : selection_sort_s32,    \
    Array(f32)    : selection_sort_f32,    \
    Array(String) : selection_sort_String  \
)(T, C);                                   \

#define print_array(T) _Generic((T),   \
    Array(s32)    : print_array_s32,   \
    Array(f32)    : print_array_f32,   \
    Array(String) : print_array_String \
)(T);                                  \

void print_array_s32(Array(s32) a) {
    for (u64 i = 0; i < a.count; i++) printf("%d ", a.data[i]);
    printf("\n");
}

void print_array_f32(Array(f32) a) {
    for (u64 i = 0; i < a.count; i++) printf("%f ", a.data[i]);
    printf("\n");
}

void print_array_String(Array(String) a) {
    for (u64 i = 0; i < a.count; i++) {
        print(a.data[i]);
        printf("\n");
    }
}


Define_array_add(s32)
Define_array_add(f32)
Define_array_add(String)

Define_array_init(s32)
Define_array_init(f32)
Define_array_init(String)

Define_selection_sort(s32)
Define_selection_sort(f32)
Define_selection_sort(String)


u8 string_compare_length(String a, String b) {
    return a.count < b.count;
}

u8 s32_compare_ascend(s32 a, s32 b) {
    return a < b;
}

u8 f32_compare_ascend(f32 a, f32 b) {
    return a < b;
}


// test
void dynamic_array_and_generics() {

    printf("==== Dynamic Array and Generics ====\n");

    DynamicArray(s32)    a = array_init(s32);
    DynamicArray(f32)    b = array_init(f32);
    DynamicArray(String) c = array_init(String);
    
    for (int i = 0; i < 128; i++) array_add(&a, rand() % 128);
    for (int i = 0; i <  5; i++)  array_add(&b, (f32) rand() / (f32) rand());

    array_add(&c, string("Help!!! "));
    array_add(&c, string("I'm "));
    array_add(&c, string("stuck in "));
    array_add(&c, string("the programming "));
    array_add(&c, string("language C!!!"));
    
    print_array(a.base);
    print_array(b.base);
    print_array(c.base);

    printf("\n");

    selection_sort(a.base, s32_compare_ascend);
    selection_sort(b.base, f32_compare_ascend);
    selection_sort(c.base, string_compare_length);
    
    print_array(a.base);
    print_array(b.base);
    print_array(c.base);
}




/* ==== Advanced Evil Black Magic ==== */

/*

See: 

- [Cloak's wiki](https://github.com/pfultz2/Cloak/wiki/C-Preprocessor-tricks,-tips,-and-idioms) 
- [Metalang99](https://github.com/Hirrolot/metalang99)


*/




int main() {
    
    string_test();
    raw_bits_and_type_punning();
    nested_types();   
    safe_varargs();
    dynamic_array_and_generics();
}

