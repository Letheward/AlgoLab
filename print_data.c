#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define print_data(data) _print_data(&data, sizeof(data));
void _print_data(void* data, int size) {
    
    unsigned char* p = data;

    printf("Hex (Little Endian):\n");
    for (int i = 0; i < size; i++) {
        if (i % 8 == 0 && i > 0) putchar('\n');
        printf("%-3.2x", p[i]);
    }

    printf("\n\nBinary (Little Endian):\n");
    for (int i = 0; i < size; i++) {
        if (i % 8 == 0 && i > 0) putchar('\n');
        for (int j = 0; j < 8; j++) {
            putchar((p[i] & (0x1 << j)) ? '1': '0');
        }
        putchar(' ');
    }

    printf("\n\nBinary (Big Endian):\n");
    for (int i = size - 1; i >= 0; i--) {
        if ((i + 1) % 8 == 0 && i < size - 1) putchar('\n');
        for (int j = 7; j >= 0; j--) {
            putchar((p[i] & (0x1 << j)) ? '1': '0');
        }
        printf(" ");
    }

    printf("\n\n\n");
}


int main() {

    int    i       = 15;
    char   c       = 'a';
    double TAU     = 6.283185307179586476925286766559;
    float  Tau     = 6.283185307179586476925286766559;
    float  nums[3] = {3.14, 6.28, 2.718};
    int*   ints    = malloc(16 * sizeof(int));

    print_data(TAU);
    print_data(Tau);
    print_data(i);
    print_data(c);
    _print_data(nums, 3 * sizeof(float));
    _print_data(ints, 16);

    int k = 0x40c90fdb;
    float tau = *(float*) &k;
    printf("%-24.24f\n", tau);

    return 0;
}
