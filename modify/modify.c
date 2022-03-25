#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef unsigned long long int u64;
typedef unsigned char          u8;

#define string(s) (String) {(u8*) s, sizeof(s) - 1}
typedef struct {
    u8* data;
    u64 count;
} String;


void print(String s, ...) {
    for (u64 i = 0; i < s.count; i++) {
        putchar(s.data[i]);
    }
}

// dumb search, but you get the point
String find(String a, String b) {
    
    if (a.data == NULL || b.data == NULL) return (String) {0};
    
    for (u64 i = 0; i < a.count; i++) {
        if (a.data[i] == b.data[0]) {
            for (u64 j = 0; j < b.count; j++) {
                if (a.data[i + j] != b.data[j]) goto next;
            }
            return (String) {a.data + i, a.count - i};
            next: continue;
        }
    }
    
    return (String) {0};
}

String load_file(char* path) {

    FILE* f = fopen(path, "rb");
    if (!f) printf("[Error] Cannot load file %s\n", path); 

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
    if (!f) printf("[Error] Cannot open file %s\n", path); 

    fwrite(in.data, sizeof(u8), in.count, f);
    fflush(f);
    fclose(f);
}



int main() {
    
    String bin = load_file("a.exe");
    
    String pos = find(bin, string("magicstamp"));

    String s = string("Someone has modified my data!");

    memcpy(pos.data, s.data, s.count);
    
    save_file(bin, "modified.exe");

}
















