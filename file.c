#include <stdio.h>
#include <stdlib.h>

void* load_file(char* path, long long* out_length) {

    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("Cannot Load %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* buffer = malloc(length);
    fread(buffer, 1, length, f);
    fclose(f);

    *out_length = length;
    return buffer;
}

char* load_file_to_string(char* path) {

    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("Cannot Load %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer     = malloc(length + 1);
    buffer[length]   = '\0';
    fread(buffer, 1, length, f);
    fclose(f);

    return buffer;
}



int main(int arg_count, char** args) {
    
    char* name;
    if (arg_count > 1) {
        name = args[1];
    } else {
        name = "file.c";
    }

    long long length = 0;
    unsigned char* file = load_file(name, &length);
    
    printf("Length: %d\n", length);
    printf("Content:\n");
    for (int i = 0; i < length; i++) {
        if (i % 16 == 0) printf("\n");
        printf("%2x ", file[i]);
    }
    
    return 0;
}