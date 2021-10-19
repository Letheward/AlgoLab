#include <stdio.h>
#include <stdlib.h>

/* Types */ 

// types in binary 
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
typedef long double             f128;

#define for_array(it, array) for (int it = 0; it < sizeof(array) / sizeof(array[0]); it++)
#define for_range(name, a, b) for (int name = a; i < b; name++)
#define match(x, stuff) case (x): {stuff; break;}
#define alloc(Type, name, count) Type* name = malloc(sizeof(Type) * count)


int main() {

    for_range (i, 0, 10) {
        switch (i) {
            match (0, printf("0000\n"))
            match (1, printf("1111\n"))
        }
    }
    
    printf("\n");

    int stack_array[10];
    for_array (i, stack_array) {
        printf("%d\n", stack_array[i]);
    }
    
    printf("\n");

    alloc (int, heap_array, 10);
    for_range (i, 0, 10) {
        printf("%d\n", heap_array[i]);
    }

    return 0;
}