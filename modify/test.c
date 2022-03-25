#include <stdio.h>

// this will be modified by modify.c
// useful for pre-fill large amount of global data, instead of print the data to text values
// if only we have compile-time execution...
unsigned char data[1024 * 1024] = {
    'm', 'a', 'g', 'i', 'c', 's', 't', 'a', 'm', 'p',
};

int main() {
    
    printf("\n");

    printf("I'm gonna print 'magicstamp', but...\n");
    printf("%s", data);

    printf("\n");
}

