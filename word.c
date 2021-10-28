/*

=====================  Note  =========================

For large alphabet and long length (not too long, or you will get a 8G+ text file),
use pipe to store the output to a file, like:

~~~
./a > temp
~~~

or you can use this to benchmark your terminal (cmd.exe and powershell will be extremely slow)

*/



/* ==== Hard Coded Version ==== */

/*
char s[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~";
int count = sizeof(s) / sizeof(s[0]) - 1;

for (int i = 0; i < count; i++) {
    for (int j = 0; j < count; j++) {
        for (int k = 0; k < count; k++) {
            printf("%c%c%c  ", s[i], s[j], s[k]);
        }
        printf("\n");
    }
}
*/


/* ==== Bad (but fast, why?) recursive version ==== */

/*

// wrapped array, allowing for pass by value and copying
typedef struct {
    int data[128]; // too high may cause stack overflow!!!
} Array;


// recursive helper, don't use outside
void _iter(char* s, int space, int x, int y, int layer, Array indices) {
    for (int i = 0; i < y; i++) {
        indices.data[x - layer] = i;
        if (layer == 0) {
            for (int i = 0; i < x; i++) {
                putchar(s[indices.data[i]]);
            }
            return;
        }
        _iter(s, space, x, y, layer - 1, indices);
        if (layer == 1) {
            for (int i = 0; i < space; i++) putchar(' ');
        }
        if (layer == 2) putchar('\n');
    }
}

void print_table(char* s, int space, int x, int y) {
    Array a = {0};
    _iter(s, space, x, y, x, a);
}

int main() {

    char s[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~";
    int count = sizeof(s) / sizeof(s[0]) - 1;

    print_table(s, 1, 5, count);

    return 0;
}
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// todo: improve performance
// s: alphabet, w: word length
void print_table(char* s, int w, int space) {

    // todo: u64 overflow
    // inline u64 pow(), get final count = base ^ w 
    typedef unsigned long long int u64;
    u64 base  = strlen(s);
    u64 count = base;
    for (int i = 1; i < w; i++) count *= base;

    for (u64 i = 0; i < count; i++) {
        
        // inline get digits of i in our base
        int indices[128] = {0};
        {
            u64 n = i;
            for (int j = w - 1; j >= 0; j--) {
                indices[j] = n % base;
                n /= base;
            }
        }

        if  (i % base == 0)             putchar('\n');
        for (int j = 0; j < w    ; j++) putchar(s[indices[j]]);
        for (int j = 0; j < space; j++) putchar(' ');
    }

    putchar('\n');
}


int main() {

    char* s = "!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~";
    print_table(s, 2, 2);

    return 0;
}
