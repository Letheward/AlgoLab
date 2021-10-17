/* 

    Just copy this and run:
    ~~~
    gcc build.c -o build && ./build
    ~~~

    typical "CMakeLists.txt" is ~2000 lines, "configure" is ~30000 lines (and written in bash)
    why should you learn and maintain (sometimes not even one) crappy language(s), 
    instead of just using your much powerful (and actually cleaner) main language?

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
    you can make a struct like {src, flags, linklib, output}
    and write a source code change check function to incremental build, etc

    we are just compiling all files and run (except this file itself)

    these structs are just for demonstrating, using these is overkill 
    and too complex for the task in this file
*/

typedef struct {
    char* invoke_name;
    char* flags; 
    // ...
} Compiler; 

typedef struct {
    char* name;
    char* binary_postfix;
    char* invoke_method;
    // ...
} Platform;

Platform win = {
    .name = "Windows",
    .binary_postfix = ".exe",
    .invoke_method = "",
};

Compiler gcc = {
    .invoke_name = "gcc",
    .flags = "-std=c99 -pedantic -Wall -static"
};

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

void compile_and_run(Platform p, Compiler c, char* src) {
    
    char* px = p.binary_postfix;
    char* method = p.invoke_method;
    char* cname = c.invoke_name;
    char* flags = c.flags;

    // slow, but who cares
    int count = 2 + 10 + string_length(src) + string_length(px) + string_length(cname) + string_length(flags);
    char* command = malloc(sizeof(char) * count); 

    // get final command 
    sprintf(command, "%s %s %s && %sa%s", cname, flags, src, method, px);    
    printf("[Doing] %s\n", src);
    printf("[Command] \n%s\n", command);
    printf("[Output]\n");

    int result = system(command);
    printf("%s\n\n", result ? "[Fail]" : "[Pass]");

    free(command);
}

int main(int arg_count, char** args) {

    /* 
        get platform and compiler from args, etc 
        ...
    */
    
    int count = sizeof(source_files) / sizeof(source_files[0]);
    for (int i = 0; i < count; i++) {
        compile_and_run(win, gcc, source_files[i]);
    }

    return 0;
}