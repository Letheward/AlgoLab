#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>


typedef unsigned char          u8;
typedef unsigned short         u16;
typedef unsigned int           u32;
typedef unsigned long long int u64;
typedef float                  f32;
typedef double                 f64;

typedef struct {
    u8* data;
    u64 count;
} String;


#define length_of(array)  (sizeof(array) / sizeof(array[0]))
#define string(s) (String) {(u8*) s, sizeof(s) - 1}
#define array(Type, array) (Array(Type)) {array, length_of(array)}

const u8* base64_table = (u8*) "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


void print(String s, ...) {
    for (u64 i = 0; i < s.count; i++) {
        putchar(s.data[i]);
    }
}

void error(char* s, ...) {

    va_list va;
    va_start(va, s);

    printf("[Error] ");
    vprintf(s, va);

    exit(1); 
}


String load_file(char* path) {

    FILE* f = fopen(path, "rb");
    if (!f) error("Cannot load %s\n", path); 

    u8* data;
    u64 count;

    fseek(f, 0, SEEK_END);
    count = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = malloc(count);
    fread(data, 1, count, f);
    fclose(f);

    return (String) {data, count};
}


void save_file(String in, char* path) {

    FILE* f = fopen(path, "wb");
    if (!f) error("Cannot open file %s\n", path); 

    fwrite(in.data, sizeof(u8), in.count, f);
    fflush(f);
    fclose(f);
}

// modified from a [public domain library](https://github.com/badzong/base64/)
String base64_encode(String in) {
    
    // allocate buffer
    u64 count = 4 * ((in.count + 2) / 3);
    u8* data  = malloc(count);

    // convert
    u64 j = 0;
    for (u64 i = 0; i < in.count; i += 3) {
        
        u8 legal = i < in.count; // reject out of bound array read

		u32 a = in.data[i];
		u32 b = legal ? in.data[i + 1] : 0;
		u32 c = legal ? in.data[i + 2] : 0;

		u32 triple = (a << 16) + (b << 8) + c;

		data[j    ] = base64_table[(triple >> 18) & 0x3f];
		data[j + 1] = base64_table[(triple >> 12) & 0x3f];
		data[j + 2] = base64_table[(triple >>  6) & 0x3f];
		data[j + 3] = base64_table[(triple      ) & 0x3f];

        j += 4;
    }

	// padding at the end
	switch (in.count % 3) {
        case 1: data[j - 2] = '=';
        case 2: data[j - 1] = '=';
	}

    return (String) {data, count};
}




int main() {

    print(base64_encode(load_file("base64.c"))); // leak, can switch to temp allocator

}

