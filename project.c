/*

How I manage my C projects

folder structure

- project
    - src
        - main.c
    - out (or bin)
        - temp
        - res
        - exe
    - lib
        - hdr
    - doc
    - build
    - readme
    - scm stuff


src: only .c files (unity build, just #include "foo.c")
    - if need to do increment build, put .h to lib/hdr/my_app etc.
out: resource, data, output binary and temp .o files
    - basically, copy this out, cleanup temp files, then just distribute
doc: documents
lib: binary library and .h files
    hdr: .h files
build: build script (or a folder containing them), in .sh or .bat (or other shell script)
readme: one file abstract, in .md or .txt
scm stuff: .gitignore, .git and etc

*/




/* ==== main.c ==== */


// libc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// third party libs
#define FOO_CRAP_MACRO_STATE
#include "libfoo.h"
#include "libbar.h"
#include "libbaz.h"

// other .c files
#include "my_foo.c"
#include "my_bar.c"
#include "my_baz.c"


int main() {

    // do stuff

    return 0;
}


/* build.sh
 
gcc -lfoo -lbar -lbaz -L lib -I lib/hdr main.c ... -o my_app



*/


