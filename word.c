#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*

## Note

For large alphabet and long length (not too long, or you will get a 8G+ text file),
use pipe to store the output to a file, like:

~~~
./a > temp
~~~

or you can use this to benchmark your terminal (cmd.exe and powershell will be extremely slow)

Use `#define` to change version:

- hardcoded_version
- recursive_version
- modulo_version
- adder_version

*/

#define adder_version



/* ==== Hard Coded Version (for reference) ==== */
#ifdef hardcoded_version

void print_table(char* s, int dummy1, int dummy2) {
    
    int count = strlen(s);

    for (int i = 0; i < count; i++) {
        for (int j = 0; j < count; j++) {
            for (int k = 0; k < count; k++) {
                for (int l = 0; l < count; l++) {
                    for (int m = 0; m < count; m++) {
                        int indices[5] = {i, j, k, l, m};
                        for (int n = 0; n < 5; n++) putchar(s[indices[n]]);
                        for (int s = 0; s < 2; s++) putchar(' ');
                    }
                    putchar('\n');
                }
            }
        }
    }
}

#endif



/* ==== Bad (but fast) Recursive Version ==== */
#ifdef recursive_version

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

void print_table(char* s, int x, int space) {
    int y = strlen(s);
    Array a = {0};
    _iter(s, space, x, y, x, a);
}

#endif



/* ==== Slow Modulo Version ==== */
#ifdef modulo_version

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

#endif



/* ==== Faster (and safer) Adder Version ==== */
#ifdef adder_version

// s: alphabet, w: word length
void print_table(char* s, int w, int space) {

    int base = strlen(s);
    int digits[1024] = {0};

    // if overflow at top digit then end
    while (digits[w] == 0) {
        
        if  (digits[0] == 0)                putchar('\n');
        for (int j = w - 1; j >= 0   ; j--) putchar(s[digits[j]]);
        for (int j = 0    ; j < space; j++) putchar(' ');
        
        // inline adder in our base, note: little endian order!!!
        digits[0]++;
        for (int i = 0; i < w; i++) {
            if (digits[i] >= base) {
                digits[i] -= base;
                digits[i + 1]++;
            }
        }
    }

    putchar('\n');
}

#endif



int main() {

    char* s = "!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~";
    print_table(s, 5, 2);
    
    return 0;
}
