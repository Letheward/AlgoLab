#include <stdio.h>
#include <stdlib.h>


typedef unsigned long long int u64;


typedef struct {
    char* name;
    int   w, h;
} Thing;


typedef struct {
    Thing* data;
    u64*   indices;
    u64    count;
    u64    allocated;
} IndexArray;


/*

another style, for "inherited" array

typedef struct {
    Array(Thing) base;
    u64*         indices;
    u64          allocated;
}

*/


IndexArray make_index_array(u64 count) {
    return (IndexArray) {
        .data      = calloc(count, sizeof(Thing)),
        .indices   = calloc(count, sizeof(u64)),
        .count     = 0,
        .allocated = count,
    };
}


// for demo, manual inline usage looks better
Thing get_element(IndexArray a, u64 i) {
    return a.data[a.indices[i]];
}


void print_array(IndexArray a) {
    for (u64 i = 0; i < a.count; i++) {
        Thing e = a.data[a.indices[i]];
        printf("%s %d %d\n", e.name, e.w, e.h);
    }
    printf("\n\n");
}



int main() {


    IndexArray a = make_index_array(1024);

    a.data[0] = (Thing) {"Apple", 2, 3};
    a.data[1] = (Thing) {"Ball",  5, 1};
    a.data[2] = (Thing) {"Cake",  4, 9};
    a.data[3] = (Thing) {"Desk",  3, 0};
    
    a.count = 4;

    {
        a.indices[0] = 0;
        a.indices[1] = 1;
        a.indices[2] = 2;
        a.indices[3] = 3;

        print_array(a);
    }


    {
        a.indices[0] = 1;
        a.indices[1] = 3;
        a.indices[2] = 0;
        a.indices[3] = 2;

        print_array(a);
    }


    // unordered "for all" operation, 
    // then it's like normal array,
    // assuming elements are dense.
    for (u64 i = 0; i < a.count; i++) {
        a.data[i].w += 1;
        a.data[i].h += 1;
    }

    print_array(a);

}


