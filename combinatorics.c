#include <stdio.h>
#include <stdlib.h>

typedef unsigned char          u8;
typedef unsigned short         u16;
typedef unsigned int           u32;
typedef unsigned long long int u64;
typedef signed char            s8;
typedef signed short           s16;
typedef signed int             s32;
typedef signed long long int   s64;


// print all combinations of set of cardinality n (n up to 63)
void combination(int n) {
    
    if (n < 0 || n > 63) return;

    const u64 one  = 1;
    const u64 done = one << n;

    u64 binary = 0;
    while (binary < done) { 

        printf("{");
        for (int i = 0; i < n; i++) {
            if (binary & (one << i)) { // unpack bits 
                printf("%d, ", i);
            }
        }
        printf("}\n");

        binary++;
    }
}


// print all permutations of set of cardinality n (n up to 63)
// heap's algorithm, from wikipedia: https://en.wikipedia.org/wiki/Heap%27s_algorithm
// todo: understand this, refactor
void permutation(int n) {

    if (n < 0 || n > 63) return;
    
    int set[64];
    int c  [64] = {0};
    for (int i = 0; i < n; i++) set[i] = i;
    
    // output
    printf("{");
    for (int i = 0; i < n; i++) {
        printf("%d, ", set[i]);
    }
    printf("}\n");

    int i = 0;
    while (i < n) {

        if (c[i] < i) {
            
            // swap
            if (i % 2 == 0) {
                int a = set[0];
                int b = set[i];
                set[0] = b;
                set[i] = a;
            } else {
                int a = set[c[i]];
                int b = set[i];
                set[c[i]] = b;
                set[i]    = a;
            }

            // output
            printf("{");
            for (int i = 0; i < n; i++) {
                printf("%d, ", set[i]);
            }
            printf("}\n");

            c[i] += 1;
            i = 0;

        } else {
       
            c[i] = 0;
            i += 1;
        }
    }
}


// print all w length words of an alphabet of cardinality n (w up to 63, n up to max(int))
void word(int w, int n) {

    if (w < 0 || w > 63) return;
    int digits[64] = {0};

    while (digits[w] == 0) { // if overflow to w digit then done
        
        printf("{");
        for (int j = w - 1; j >= 0; j--) printf("%d, ", digits[j]);
        printf("}\n");
        
        // adder, note: little endian order, so we need to print backwards
        digits[0]++; 
        for (int i = 0; i < w; i++) {
            if (digits[i] >= n) {
                digits[i] -= n;
                digits[i + 1]++;
            }
        }
    }
}


int main() {

    /*
        the printed "sets" can be use for indices into an array,
        so if you want to enumerate all possible outcomes 
        following these math structures using an array of elements,
        you can just take these algorithms, and modify all the printf()
        to read/write array, store the indices, etc.
    */

    printf("\nCombination:\n");
    combination(3);

    printf("\nPermutation:\n");
    permutation(3);

    printf("\nWord:\n");
    word(3, 2);

}


