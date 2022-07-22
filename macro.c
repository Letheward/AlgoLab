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

void print_string(String s) {
    for (u64 i = 0; i < s.count; i++) putchar(s.data[i]);
}

void string_test() {
    
    printf("==== String Test ====\n");
    
    u8  a[] = {87, 111, 114, 108, 100, 33, 10};
    u32 b   = 0x74736554;

    print_string(string("Hello "));
    print_string(array_string(a));
    print_string(data_string(b));

    printf("\n\n");
}





/* ==== Raw Bits and Type-Punning ==== */

#define bit8( Type, value) ((union {Type a; u8  b;}) {.a = value}).b
#define bit16(Type, value) ((union {Type a; u16 b;}) {.a = value}).b
#define bit32(Type, value) ((union {Type a; u32 b;}) {.a = value}).b
#define bit64(Type, value) ((union {Type a; u64 b;}) {.a = value}).b

#define from_bit8( Type, value) ((union {Type a; u8  b;}) {.b = value}).a
#define from_bit16(Type, value) ((union {Type a; u16 b;}) {.b = value}).a
#define from_bit32(Type, value) ((union {Type a; u32 b;}) {.b = value}).a
#define from_bit64(Type, value) ((union {Type a; u64 b;}) {.b = value}).a


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

// Note: Do not actually use this. The error/warning messages are pretty bad, and when things go wrong in the macro, it's very hard to debug.
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


// note: this won't work when no varags are given
#define print(s, ...)                                                               \
{                                                                                   \
    String macro_internal(args)[] = {__VA_ARGS__};                                  \
    print_helper(string(s), macro_internal(args), length_of(macro_internal(args))); \
}                                                                                   \

void print_helper(String s, String* data, u64 count) {
    
    u64 acc = 0; 
    for (u64 i = 0; i < s.count; i++) {

        u8 c = s.data[i];
        if (c == '@') {
            if (i + 1 < s.count && s.data[i + 1] == '@') { // short circuit 
                putchar('@');
                i++;
            } else {
                if (acc >= count) continue; // simple safety check, we can also just return here to indicate the error
                print_string(data[acc]); 
                acc++;
            }
            continue;
        }

        putchar(c);
    }
}




// test
void safe_varargs() {

    printf("==== Safe Varargs ====\n");

    add_s32(1, 2, 3);
    add_s32(1, 2, 3, 4, 5);

    // notice the count of '@' does not match varargs count, but we didn't crash here
    print("Print @ @ @ @ using varargs.\n\n", string("some"), string("string")); 
    
    // add_s32(1, 2, 3, 4, "this will give warning");
    // print("this will give warning", 42);
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

#define Define_insertion_sort(Type)                                            \
void insertion_sort_ ## Type (Array(Type) in, u8 (*compare)(Type a, Type b)) { \
                                                                               \
    if (in.count < 2) return;                                                  \
                                                                               \
    for (u64 i = 1; i < in.count; i++) {                                       \
                                                                               \
        Type key = in.data[i];                                                 \
                                                                               \
        u64 j = i;                                                             \
        while (j > 0 && compare(key, in.data[j - 1])) {                        \
            in.data[j] = in.data[j - 1];                                       \
            j--;                                                               \
        }                                                                      \
                                                                               \
        in.data[j] = key;                                                      \
    }                                                                          \
}                                                                              \

#define array_add(T, V) _Generic((T),         \
    DynamicArray(s32)*    : array_add_s32,    \
    DynamicArray(f32)*    : array_add_f32,    \
    DynamicArray(String)* : array_add_String  \
)(T, V)                                       \

#define insertion_sort(T, C) _Generic((T), \
    Array(s32)    : insertion_sort_s32,    \
    Array(f32)    : insertion_sort_f32,    \
    Array(String) : insertion_sort_String  \
)(T, C)                                    \

#define print_array(T) _Generic((T),   \
    Array(s32)    : print_array_s32,   \
    Array(f32)    : print_array_f32,   \
    Array(String) : print_array_String \
)(T)                                   \

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
        print_string(a.data[i]);
        printf("\n");
    }
}


Define_array_add(s32)
Define_array_add(f32)
Define_array_add(String)

Define_array_init(s32)
Define_array_init(f32)
Define_array_init(String)

Define_insertion_sort(s32)
Define_insertion_sort(f32)
Define_insertion_sort(String)


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

    array_add(&c, string("Help!!!"));
    array_add(&c, string("I'm"));
    array_add(&c, string("stuck in"));
    array_add(&c, string("the programming"));
    array_add(&c, string("language C!!!"));
    array_add(&c, string("And I'm trying to do some"));
    array_add(&c, string("high level stuff!!!"));
    
    print_array(a.base);
    print_array(b.base);
    print_array(c.base);

    printf("\n");

    insertion_sort(a.base, s32_compare_ascend);
    insertion_sort(b.base, f32_compare_ascend);
    insertion_sort(c.base, string_compare_length);
    
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

