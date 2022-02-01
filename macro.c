/* ==== Setup ==== */

//#define macro_debug

#ifndef macro_debug

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#endif

typedef unsigned char          u8;
typedef unsigned short         u16;
typedef unsigned int           u32;
typedef unsigned long long int u64;
typedef float                  f32;
typedef double                 f64;

typedef struct {
    u8* data;
    u64 count;
} String;




/* ==== Macros ==== */

#define Array(Type) Array_ ## Type 
#define Define_Array(Type) \
    typedef struct {       \
        Type* data;        \
        u64   count;       \
    } Array(Type)          \


#define Vector(size) Vector ## size
#define Define_Vector(size) \
    typedef struct {        \
        f32 data[size];     \
    } Vector(size)          \


#define Matrix(size) Matrix ## size
#define Define_Matrix(size)    \
    typedef struct {           \
        f32 data[size * size]; \
    } Matrix(size)             \


#define length_of(array)  (sizeof(array) / sizeof(array[0]))
#define string(s) (String) {(u8*) s, sizeof(s) - 1}
#define array(Type, array) (Array(Type)) {array, length_of(array)}





/* ==== Usage ==== */

Define_Array(String);
Define_Vector(4);
Define_Matrix(4);

Define_Array(u32);
Define_Array(Array_u32); // cannot use nested array, there are maybe hacks to do this, but that lose the point

/*/ 
Array(Array_u32) make_nested() {
}
/*/

Vector4 v4_make() {
    return (Vector4) {0};
}


#ifndef macro_debug

void print(String s, ...) {
    for (u64 i = 0; i < s.count; i++) {
        putchar(s.data[i]);
    }
}

String string_join(Array(String) s, String seperator) {

    String out = {0};

    for (int i = 0; i < s.count; i++)  out.count += s.data[i].count;
    out.count += seperator.count * (s.count - 1);        
    out.data = malloc(out.count); // we should probably use better allocator for this

    u64 cursor = 0;
    for (int i = 0; i < s.count; i++) {
       
        if (i != 0) {
            for (int j = 0; j < seperator.count; j++) {
                out.data[cursor + j] = seperator.data[j];
            }
            cursor += seperator.count;
        }

        String it = s.data[i];
        for (int j = 0; j < it.count; j++) {
            out.data[cursor + j] = it.data[j];
        }
        cursor += it.count;
    }

    return out;
}

#endif






int main() {
    
    String strings[] = {
        string("this"),
        string("is"),
        string("a"),
        string("string"),
    };

    String s = string_join(array(String, strings), string(" --- "));
    print(s);

    return 0;
}



/*
    
    // These Became Bad


    // maybe just write it
    ~~~
    Define_Array(u8);
    typedef Array(u8) String;
    ~~~


    // doesn't look like a function
    ~~~
    #define make_vector(size) \
        Vector(size) make_v ## size
    
    make_vector(3) () {
        return (Vector3) {0};
    }
    ~~~


    // it's longer to type and add another level of indirection to trace what it's actually doing
    ~~~
    #define Define_Container(Type) Define_ ## Type 

    Define_Container(Array)(String);
    Define_Container(Vector)(f32, 4);
    ~~~


    // for small matrix size copying code is not a problem 
    // for large matrix size just write it using pointers 
    ~~~
    #define matrix_identity(size)                          \
        Matrix(size) out = {0};                            \
        for (int i = 0; i < size; i++) {                   \
            for (int j = 0; j < size; j++) {               \
                if (i == j) out.data[i * size + j] = 1.0;  \
            }                                              \
        }                                                  \
        return out;                                        \

    Matrix3 m3_identity() { matrix_identity(3) }
    Matrix4 m4_identity() { matrix_identity(4) }
    Matrix5 m5_identity() { matrix_identity(5) }

    #undef matrix_identity
    ~~~

*/
