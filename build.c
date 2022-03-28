/* 

## Usage

using this like a script:
~~~
gcc build.c -o build && ./build
~~~

just using it (after compiling):
~~~
./build
~~~

typical "CMakeLists.txt" is ~2000 lines, "configure" is ~30000 lines (and written in bash)
why should you learn and maintain (sometimes not even one) crappy language(s), 
instead of just using your much powerful (and actually cleaner) main language?



## Structs

you can make a struct like {src, flags, linklib, output}
and write a source code change check function to incremental build, etc

we are just compiling all files and run (except this file itself)

these structs below are just for demonstrating, using these is overkill 
and too complex for the task in this file

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
    .flags = "-std=c11 -pedantic -Wall -static -fopenmp"
};

char* source_files[] = {
    "AoS_vs_SoA.c",
    "better_syntax.c",
    "const.c",
    "file.c",
    "matrix.c",
    "parse.c",
    "pass_by_value.c",
    "print.c",
    "probability.c",
    "string.c",
    "macro.c",
    "base64.c",
    "allocator.c",
    // "NoCS.c", // kinda slow
    "sort.c",
    "crazy_macro.c",
    "index_array.c",
    "all.c",
    "gibberish_code_generator.c",
    "simd.c",
    // "word.c", // too slow on some terminals (looking at you, Microsoft!)
};


void compile_and_run(Platform p, Compiler c, char* src) {
    
    char* px     = p.binary_postfix;
    char* method = p.invoke_method;
    char* cname  = c.invoke_name;
    char* flags  = c.flags;

    int count = 2 + 10 + strlen(src) + strlen(px) + strlen(cname) + strlen(flags);
    char* command = malloc(sizeof(char) * count); // slow, but who cares

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