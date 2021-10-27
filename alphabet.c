/*

=====================  Note  =========================

Please, use pipe to store the output to a file, like

~~~
./a > temp
~~~

or you can use this to benchmark your terminal (cmd.exe and powershell will be extremely slow)

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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


    /* ==== Hard Coded Version For Reference ==== */
    
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


    // Punctuators
    {
        printf("Punctuators ------------------------\n\n");
        char s[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~";
        int count = sizeof(s) / sizeof(s[0]) - 1;

        for (int i = 1; i <= 3; i++) {
            printf("%d Characters:\n\n", i);
            print_table(s, i, i, count);
            printf("\n\n\n");
        }
    }

    // Identifiers
    {
        printf("Identifiers ------------------------\n\n");
        char s[] = "_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        int count = sizeof(s) / sizeof(s[0]) - 1;
        
        for (int i = 1; i <= 3; i++) {
            printf("%d Characters:\n\n", i);
            print_table(s, 1, i, count);
            printf("\n\n\n");
        }
    }

    // Custom
    {
        printf("Custom ------------------------\n\n");
        char s[] = "aeiouy";
        int count = sizeof(s) / sizeof(s[0]) - 1;

        for (int i = 1; i <= count; i++) {
            printf("%d Characters:\n\n", i);
            print_table(s, 2, i, count);
            printf("\n\n\n");
        }
    }

    return 0;
}
