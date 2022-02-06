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

typedef unsigned char           bool; // always u8




/* ==== Macros ==== */

#define true  1
#define false 0

#define length_of(array) (sizeof(array) / sizeof(array[0]))
#define string(s) (String) {(u8*) s, sizeof(s) - 1}
#define array(Type, array) (Array(Type)) {array, length_of(array)} // a little bit confusing

#define Array(Type) Array_ ## Type
#define Define_Array(Type) \
typedef struct {           \
    Type* data;            \
    u64   count;           \
} Array(Type)              \




/* ==== Types ==== */

// also used for Array(u8), to avoid "converting back and forth" API
typedef struct {
    u8* data;
    u64 count;
} String;

typedef struct {
    u8* data;
    u64 size;
    u64 allocated;
    u64 highest;
} ArenaBuffer;

typedef struct {
    ArenaBuffer temp_buffer;
    void*       (*alloc)(u64);
} GlobalRuntime;

GlobalRuntime runtime;




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





/* ==== Temp Allocator ==== */

void* temp_alloc(u64 count) {

    ArenaBuffer* a = &runtime.temp_buffer;
    
    u64 current = a->allocated;
    u64 wanted  = current + count;
    
    assert(wanted < a->size);
    
    if (wanted > a->highest) a->highest = wanted; // check the highest here, maybe slow?
    a->allocated = wanted;

    return a->data + current;
}

void temp_reset() {
    ArenaBuffer* a = &runtime.temp_buffer;
    a->allocated = 0;
    memset(a->data, 0, a->highest); // do we need this?
}

void temp_info() {
    ArenaBuffer* a = &runtime.temp_buffer;
    printf(
        "Temp Buffer Info:\n"
        "Data:      %p\n"
        "Size:      %lld\n"
        "Allocated: %lld\n"
        "Highest:   %lld\n\n",
        a->data, a->size, a->allocated, a->highest
    );
}





/* ==== String Operations ==== */

void print(String s, ...) {
    for (u64 i = 0; i < s.count; i++) putchar(s.data[i]);
}

String concat(int count, ...) {
    
    assert(count <= 32);
    String strings[32];
    {
        va_list args;
        va_start(args, count);
        for (int i = 0; i < count; i++) strings[i] = va_arg(args, String);
        va_end(args);
    }

    String out = {0};
    for (int i = 0; i < count; i++) out.count += strings[i].count;

    out.data = runtime.alloc(out.count);
    
    u64 counter = 0;
    for (int i = 0; i < count; i++) {
        String s = strings[i];
        for (int j = 0; j < s.count; j++) {
            out.data[counter] = s.data[j];
            counter++;
        }
    }

    return out;
}

String random_string(u64 size) {
    u8* data = runtime.alloc(size);
    for (u64 i = 0; i < size; i++) {
        data[i] = rand() % 95 + ' ';
    }
    return (String) {data, size};
}




int main() {

    // setup runtime
    {
        u64 size = 1024 * 256; // cache size
        runtime.temp_buffer.data = calloc(size, sizeof(u8));
        runtime.temp_buffer.size = size;
        runtime.alloc = malloc;
    }

    String junk;

    printf("Temp Allocator:      ");
    time_it();    
    {
        runtime.alloc = temp_alloc;
        
        String a = random_string(1024);
        String b = random_string(1024);
        String c = random_string(1024);

        String s1 = concat(3, a, b, c);
        String s2 = concat(3, b, c, a);
        String s3 = concat(3, c, a, b);
        
        junk = concat(3, s1, s2, s3);
        
        temp_reset();
    }
    time_it();    
    
    printf("malloc() and free(): ");
    time_it();    
    {
        runtime.alloc = malloc;

        String a = random_string(1024);
        String b = random_string(1024);
        String c = random_string(1024);

        
        String s1 = concat(3, a, b, c);
        String s2 = concat(3, b, c, a);
        String s3 = concat(3, c, a, b);
        
        junk = concat(3, s1, s2, s3);
        
        free(a.data);
        free(b.data);
        free(c.data);

        free(s1.data);
        free(s2.data);
        free(s3.data);

        free(junk.data);
    }
    time_it();    

}


