#include <stdio.h>
#include <stdlib.h>

char* source_files[] = {
    "AoS_vs_SoA.c",
    "better_syntax.c",
    "const_is_better.c",
    "const_is_not_better.c",
    "file.c",
    "geometric_algebra.c",
    "linear_algebra.c",
    "matrix.c",
    "parse.c",
    "pass_by_value.c",
    "print.c",
    "probability.c",
    "project.c",
    "tertian_walk.c",
    "",
};

int string_length(char* s) {
    int i = 0;
    while (s[i] != '\0') i++;
    return i;
}

void compile_and_run(char* src) {
    
    char command[] = "gcc %s && a.exe";
    int count = string_length(src) + string_length(command);
    char* result = malloc(sizeof(char) * count); // slow, but who cares

    printf("\n[Compile and Running: %s]\n", src);
    printf("[Output]\n");

    sprintf(result, command, src); 
    system(result);
    
    free(result);
}

// gcc build.c -o build && ./build
int main() {

    int count = sizeof(source_files) / sizeof(source_files[0]);
    for (int i = 0; i < count; i++) {
        compile_and_run(source_files[i]);
    }

    return 0;
}