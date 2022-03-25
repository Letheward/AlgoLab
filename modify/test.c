#include <stdio.h>

unsigned char data[1024 * 1024] = {
    'm', 'a', 'g', 'i', 'c', 's', 't', 'a', 'm', 'p',
};

int main() {
    
    printf("\n");

    printf("I'm gonna print 'magicstamp', but...\n");
    printf("%s", data);

    printf("\n");
}

