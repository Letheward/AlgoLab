#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>


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




/* ==== Macros ==== */

#define length_of(array)   (sizeof(array) / sizeof(array[0]))
#define string(s)          (String) {(u8*) s, sizeof(s) - 1}
#define array(Type, array) (Array(Type)) {array, length_of(array)} 

#define Array(Type) Array_ ## Type
#define Define_Array(Type) \
typedef struct {           \
    Type* data;            \
    u64   count;           \
} Array(Type)              \

Define_Array(u64);
Define_Array(String);





/* ==== Timer ==== */

typedef struct {
    struct timespec start;
    struct timespec end;
    u8 which;
} PerformanceTimer;

PerformanceTimer timer;

void time_it() {
    if (timer.which == 0) {
        clock_gettime(CLOCK_MONOTONIC, &timer.start);
        timer.which = 1;
    } else {
        clock_gettime(CLOCK_MONOTONIC, &timer.end);
        printf("%fs\n", (timer.end.tv_sec - timer.start.tv_sec) + 1e-9 * (timer.end.tv_nsec - timer.start.tv_nsec));
        timer.which = 0;
    }
}




/* ==== Helper ==== */

Array(u64) random_array(u64 count, u64 range) {
    u64* data = malloc(sizeof(u64) * count);
    for (u64 i = 0; i < count; i++)  data[i] = rand() % range;
    return (Array(u64)) {data, count};
}

void print_array(Array(u64) in) {
    for (u64 i = 0; i < in.count; i++) {
        printf("%llu, ", in.data[i]);
    }
    printf("\n");
}

void print(String s) {
    for (u64 i = 0; i < s.count; i++) putchar(s.data[i]);
}





/* ==== Algorithms ==== */

void bubble_sort(Array(u64) in) {

    while (1) {

        u8 found = 0;

        for (u64 i = 0; i < in.count - 1; i++) {

            u64 a = in.data[i    ];
            u64 b = in.data[i + 1];

            if (a > b) {
                in.data[i    ] = b;
                in.data[i + 1] = a;
                found = 1;
            }
        }

        if (!found) break;
    }
}

// modify more 
void selection_sort(Array(u64) in) {

    for (u64 i = 0; i < in.count - 1; i++) {

        u64 min = i;
        for (u64 j = i + 1; j < in.count; j++) {
            if (in.data[min] > in.data[j])  min = j; 
        }
        
        if (min != i) {
            u64 a = in.data[i  ];
            u64 b = in.data[min];
            in.data[i  ] = b;
            in.data[min] = a;
        }
    }
}


// todo: validate, make u64 work
void quick_sort_helper(u64* in, s64 start, s64 end) {

    if (start >= end || start < 0) return; 

    u64 pivot = in[end];

    s64 i = start;
    for (s64 j = start; j < end; j++) {
        if (pivot > in[j]) {
            u64 temp = in[i];
            in[i]    = in[j];
            in[j]    = temp;
            i++;
        }
    }

    u64 temp = in[i];
    in[i]    = in[end];
    in[end]  = temp;

    quick_sort_helper(in, start, i - 1);
    quick_sort_helper(in, i + 1, end);
}

void quick_sort(Array(u64) in) {
    quick_sort_helper(in.data, 0, (s64) in.count - 1);
}


int u64_compare_c(const void* a, const void* b) {
    u64 ra = *(u64*) a; 
    u64 rb = *(u64*) b;
    if (ra > rb) return  1;
    if (ra < rb) return -1;
    return 0;
}




int main() {

/*/
    String s_inputs[] = {
        string("some"),
        string("test"),
        string("strings"),
        string("for"),
        string("sorting"),
        string("algorithms"),
    };
/*/
   
/*/
    printf("\n");
    for (u64 i = 0; i < length_of(s_inputs); i++) {
        print(s_inputs[i]);
        printf("\n");
    }
    printf("\n");
/*/


/*/
    {
        struct timespec seed;
        clock_gettime(CLOCK_MONOTONIC, &seed);
        srand(seed.tv_nsec);
    }
/*/

    for (u64 i = 0; i < 32; i++) {

        Array(u64) input = random_array(1024 * 64, 32768);
        
        //print_array(input);
       
        time_it();
        
        //bubble_sort(input);
        //selection_sort(input);
        quick_sort(input);
        //qsort(input.data, input.count, sizeof(u64), u64_compare_c);
        
        time_it();

        //print_array(input);
        
        free(input.data);
    }



}


