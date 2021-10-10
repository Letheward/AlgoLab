#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

char* strings[] = {
    "",
    "0",
    "      42",
    "       -11239",
    "  +3000 1234 5124 kill",
    "12390815092     ",
    "4294967296     ",
    "junk     ",
    "   apple3",
    "g1430gt3l"
};

int parse_int(int* out, const char* s) {

    char* end;
    int i = strtol(s, &end, 10);
    if (end - s == 0) return 1;
    
    *out = i;
    
    return 0;
}

// parse all numbers in string to a array, not safe
int parse_int_many(int* out, const char* s) {

    char* end;
    int i = strtol(s, &end, 10);
    if (end - s == 0) return 1;
    
    *out = i;

    parse_int_many(out + 1, end);
    
    return 0;
}

// not handling some overflow, but works
int parse_s32(int* out, const char* s) {

    int length = 0;
    int result = 0;
    int sign   = 1;

    while (*s == ' ') s++;

    switch (*s) {
        case '-': sign = -1; // fall through
        case '+': s++;
    }

    while (*s != '\0') {
        if (*s < '0' || *s > '9') break;
        s++;
        length++;
    }

    if (length == 0) return 1;
    if (length > 10) return 2;

    for (int i = 1; i <= length; i++) {
        int v = 1;
        for (int exp = 1; exp < i; exp++) v *= 10; // inline pow(), if we use libc then there is no point for doing this, just use strtol() then
        int digit = *(s - i) - 48;
        result += digit * v;
    }

    *out = result * sign;
    
    return 0;
}


int main() {
    
    int count = sizeof(strings) / sizeof(strings[0]);
    int* numbers = malloc(sizeof(int) * count);

    for (int i = 0; i < count; i++) {
        int error = parse_s32(numbers + i, strings[i]);
        switch (error) {
            case 0: printf("%d\n", numbers[i]); break;
            case 1: printf("Unable to parse \"%s\"\n", strings[i]); break;
            case 2: printf("Number too large in \"%s\"\n", strings[i]); break;
        }
    } 

    return 0;
}
