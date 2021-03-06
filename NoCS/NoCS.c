#include <stdio.h>
#include <stdlib.h>



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

#define length_of(array) (sizeof(array) / sizeof(array[0]))
#define string(s) (String) {(u8*) s, sizeof(s) - 1}
#define array(Type, array) (Array(Type)) {array, length_of(array)} // a little bit confusing

#define Array(Type) Array_ ## Type
#define Define_Array(Type) \
typedef struct {           \
    Type* data;            \
    u64   count;           \
} Array(Type)              \

Define_Array(u64);
Define_Array(String);

#undef Define_Array





/* ==== Generator Search ==== */

void generator_search() {

    printf("Generator Search:\n");

    u64 primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271};

    u64 counter = 2;
    while (1) {
        
        // u8 fail = 0;
        for (u64 i = 0; i < length_of(primes); i++) {
            u64 prime = primes[i];
            if (prime >= (counter / 2)) break;
            if (!(counter % prime == 0)) {
                //printf("Fail %llu %llu\n", counter, prime);
                goto failed; 
            }
        }
    
        printf("Found %llu\n", counter);

        failed: 
        if (counter == 1000000000) break; //printf("--- %llu ---\n", counter);
        counter += 2;
    }

}



/* ==== Tertian Walk (Need Refactoring) ==== */

/* result

                      0
                     / \
                    3   4
                   / \ / \
                  6   7   8
                 / \ / \ / 
                9  10  11   0
                 \ / \ / \
              0   1   2   3   4
                 / \ / \ / \
            3   4   5   6   7   8
               / \ / \ / \ / \
          6   7   8   9  10  11   0
             / \ /     \ / \ / \
        9  10  11   0   1   2   3   4
           / \ / \     / \ / \ / \
      0   1   2   3   4   5   6   7   8
         / \ / \ / \ / \ / \ / \ / \
    3   4   5   6   7   8   9  10  11   0
       / \ / \ / \ / \ /     \ / \ / \
  6   7   8   9  10  11   0   1   2   3   4
     / \ /     \ / \ / \     / \ / \ / \
9  10  11   0   1   2   3   4   5   6   7   8

0  3  6  9  1  4  7  10 2  5  8  11

0  4  7  10 2  5  8  11 3  6  9  1
0  4  8  11 3  7  10 2  6  9  1  5

// The Man Machine

num_list     0  3  6  9  1  4  7  10 2  5  8  11
bin_list  0  0  0  0  1  0  0  0  1  0  0  0  

>    0
value    0
list_pos 0
num      0

// Brute Force result
0  3  6  9  1  4  7  10 2  5  8  11
0  4  7  10 2  5  8  11 3  6  9  1
0  4  7  10 2  6  9  1  5  8  11 3
0  4  8  11 3  7  10 2  6  9  1  5

*/






/*  === Original ===
#include <stdio.h>

const MODNUM = 12;

int note = 0;
buffer = [];

int left(buffer) {
    return 12 - sizeof(buffer);
}

void walk(note) {
    for (i = 0; i < left(buffer); i++) {
        note = mod(note + 3 or 4, 12);
        if buffer_has(note) {
            clear(buffer);
            note = 0;
        }
        if (i == 11) {
            print(buffer);
        } else {
            walk(note);
        }
    }
}

void main() {
    walk(note);
}
*/

/* === Draft ===


// == Model 1 ==

build a size 12 num_list 0 0 0 0 0 0 0 0 0 0 0 0
build a size 12 bin_list 0 0 0 0 0 0 0 0 0 0 0 0

write 0 to num_list position 0
write 0 to bin_list position 0

list_position affect num_list and bin_list

num is mod 12 number

start list_position = 0
start num = 0

generate:
    list_position + 1
    if try to get list_position 12:
        print num_list
        list_position = 0
        num = 0
        generate
    
    if not try to get list_position 12:
        if value in bin_list position is 1:
            generate
        
        try = num + 3

        if before and num_list position have try:
            try = num + 4
            
            if before and num_list position have try:
                list position = 0
                num = 0
                generate
            if before and num_list position don't have try:
                num = try
                write num to num_list at list_position
                write 1 to bin_list at list_position
                generate

        if before and num_list position don't have try:
            num = try
            write num to num_list at list_position
            write 0 to bin_list at list_position
            generate

// === Model 2 ===

binary_add is like:
0 0 0 0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0 0 0 1
0 0 0 0 0 0 0 0 0 0 1 0
0 0 0 0 0 0 0 0 0 0 1 1
0 0 0 0 0 0 0 0 0 1 0 0
0 0 0 0 0 0 0 0 0 1 0 1
0 0 0 0 0 0 0 0 0 1 1 0
0 0 0 0 0 0 0 0 0 1 1 1
...

build a size 12 num_list 0 0 0 0 0 0 0 0 0 0 0 0
build a size 12 bin_list 0 0 0 0 0 0 0 0 0 0 0 0

list_pos = 0

num = 0
num is mod 12 number

generate:
    while list_pos < 11:
        read bin_list at (list_pos + 1) as value
        if value = 0:
            num + 3
        if value = 1:
            num + 4
        write num to num_list at (list_pos + 1)
        list_pos + 1
    if num_list don't have repeated number:
        print num_list
    num = 0
    list_pos = 0
    binary_add bin_list 1
    if bin_list at position 0 is 1:
        stop
    else:
        generate

generate

*/

/*
void bin_print(bin) {
    for (int bit = 11; bit >= 0; bit--) {
        if (((bin >> bit) & 1) == 0) {
            printf("0");
        } else {
            printf("1");
        }
    }
}
*/

// todo: namespace these

int num_list[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
int bin_list[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int list_pos = 0;
int num = 0;
int bin = 0;

void generate() {


    while (list_pos < 11) {
        if (bin_list[list_pos + 1] == 0) {
            num = (num + 3) % 12;
        } else {
            num = (num + 4) % 12;
        }
        num_list[list_pos + 1] = num;
        list_pos += 1;
    }


    int repeated = 0;
    for (int i = 0; i < 11; i++) {
        for (int j = i + 1; j < 12; j++) {
            if (num_list[i] == num_list[j]) {
                repeated = 1;
                break;
            }
        }
    }

    if (!repeated) {
        for (int i = 0; i < 12; i++) {
            printf("%i ", num_list[i]);
        }
        printf("\n");
    }
    
    // for (int i = 0; i < 12; i++) {
    //     printf("%i ", num_list[i]);
    // }
    // printf("\n");
    num = 0;
    list_pos = 0;

    // bin_add(bin);
    {
        bin += 1;
        for (int bit = 12; bit >= 0; bit--) {
            if (((bin >> bit) & 1) == 0) {
                bin_list[11 - bit] = 0;
            } else {
                bin_list[11 - bit] = 1;
            }
        }
    }


    if (bin_list[0] == 1) {
        return;
    } else {
        generate();
    }
}

void tertian_walk() {

    printf("Tertian Walk:\n");

    generate();
    // printf("%i", bin);
    // int s = 0;
    // while (s < 2047) {
    //     bin_add();
    //     s += 1;
    // }
        
    // for (int i = 0; i < 12; i++) {
    //     printf("%i ", bin_list[i]);
    // }
    // printf("\n");
    // bin_print(bin);
}





int main() {


    generator_search();
    tertian_walk();

}