#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>




/* ==== Macros ==== */

#define length_of(array)   (sizeof(array) / sizeof(array[0]))
#define string(s)          (String) {(u8*) s, sizeof(s) - 1}
#define array(Type, array) (Array(Type)) {array, length_of(array)} 
#define sort(in, compare)  qsort(in.data, in.count, sizeof(in.data[0]), compare)

#define Array(Type) Array_ ## Type
#define Define_Array(Type) \
typedef struct {           \
    Type* data;            \
    u64   count;           \
} Array(Type)              \





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

    if (in.count < 2) return;
    
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



// todo: cleanup 
void selection_sort(Array(u64) in) {

    if (in.count < 2) return;
    
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



void insertion_sort(Array(u64) in) {

    if (in.count < 2) return;
    
    for (u64 i = 1; i < in.count; i++) {
    
        u64 key = in.data[i];
    
        u64 j = i;
        while (j > 0 && in.data[j - 1] > key) {
            in.data[j] = in.data[j - 1];
            j--;
        }
    
        in.data[j] = key;
    }
}



// todo: validate, make u64 work
void quick_sort_helper(u64* in, s64 start, s64 end) {

    if (start < 0 || start >= end) return; 

    u64 pivot = in[end];

    s64 i = start;
    for (s64 j = start; j < end; j++) {

        if (pivot > in[j]) {
            
            u64 a = in[i];
            u64 b = in[j];
            in[i] = b;
            in[j] = a;
            
            i++;
        }
    }
    
    {
        u64 a   = in[i];
        u64 b   = in[end];
        in[i]   = b;
        in[end] = a;
    }

    quick_sort_helper(in, start, i - 1);
    quick_sort_helper(in, i + 1, end);
}

void quick_sort(Array(u64) in) {
    if (in.count < 2) return;
    quick_sort_helper(in.data, 0, (s64) in.count - 1);
}



// todo: validate
void heap_sort(Array(u64) in) {
    
    if (in.count < 2) return;
    
    // parent: (i - 1) / 2
    // left:   2 * i + 1
    // right:  2 * i + 2

    // heapify
    s64 start = (in.count - 2) / 2;
    while (start >= 0) {
        
        // sift down
        u64 root = start;
        u64 end  = in.count - 1;
        while (2 * root + 1 <= end) {
            
            u64 child = 2 * root + 1;
            u64 swap  = root;
            
            if (in.data[swap] < in.data[child])                         swap = child;
            if (in.data[swap] < in.data[child + 1] && child + 1 <= end) swap = child + 1;
            if (swap == root) break;
            
            u64 a = in.data[root];
            u64 b = in.data[swap];
            in.data[root] = b;
            in.data[swap] = a;
            root = swap;
        }

        start--;
    }
    
    u64 end = in.count - 1;
    while (end > 0) {
    
        u64 a = in.data[end];
        u64 b = in.data[0];
        in.data[end] = b;
        in.data[0]   = a;
    
        end--;    
        
        // sift down
        u64 root = 0;
        while (2 * root + 1 <= end) {
            
            u64 child = 2 * root + 1;
            u64 swap  = root;
            
            if (in.data[swap] < in.data[child])                         swap = child;
            if (in.data[swap] < in.data[child + 1] && child + 1 <= end) swap = child + 1;
            if (swap == root) break;
            
            u64 a = in.data[root];
            u64 b = in.data[swap];
            in.data[root] = b;
            in.data[swap] = a;
            root = swap;
        }
    }
}




int u64_compare_c(const void* a, const void* b) {
    u64 ra = *(u64*) a; 
    u64 rb = *(u64*) b;
    if (ra > rb) return  1;
    if (ra < rb) return -1;
    return 0;
}




int main() {

    
    for (u64 i = 0; i < 32; i++) {

        Array(u64) input = random_array(1024 * 64, 1024);
        
        //print_array(input);
       
        time_it();

        //sort(input, u64_compare_c);
        
        //bubble_sort(input);
        //selection_sort(input);
        //insertion_sort(input);
        //quick_sort(input);
        heap_sort(input);
        
        time_it();

        //print_array(input);
        
        free(input.data);
    }

}


