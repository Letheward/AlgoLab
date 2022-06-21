#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>




/* ==== Types ==== */

typedef unsigned char           u8;
typedef unsigned short int      u16;
typedef unsigned int            u32;
typedef unsigned long long int  u64;
typedef signed char             s8;
typedef signed short            s16;
typedef signed int              s32;
typedef signed long long int    s64;
typedef float                   f32;
typedef double                  f64;

typedef struct {
    u8* data;
    u64 count;
} String;




/* ==== Macros ==== */

#define string(s)            (String) {(u8*) s, sizeof(s) - 1}
#define length_of(array)     (sizeof(array) / sizeof(array[0]))
#define array(Type, c_array) (Array(Type)) {c_array, length_of(c_array)}

#define Array(Type) Array_ ## Type
#define Define_Array(Type) \
typedef struct {           \
    Type* data;            \
    u64   count;           \
} Array(Type)              \

Define_Array(String);







/* ==== Temp Allocator ==== */

typedef struct {
    u8* data;
    u64 size;
    u64 allocated;
    u64 highest;
} ArenaBuffer;

struct {
    ArenaBuffer   temp_buffer;
    void*         (*alloc)(u64);
    Array(String) command_line_args;
} runtime;


void* temp_alloc(u64 count) {

    ArenaBuffer* a = &runtime.temp_buffer;
    
    u64 current = a->allocated;
    u64 wanted  = current + count;
    
    assert(wanted < a->size);
    
    if (wanted > a->highest) a->highest = wanted; // check the highest here, maybe slow?
    a->allocated = wanted;

    return a->data + current;
}

void temp_free(u64 size) {
    runtime.temp_buffer.allocated -= size;
}

void temp_reset() {
    ArenaBuffer* a = &runtime.temp_buffer;
    a->allocated = 0;
    memset(a->data, 0, a->highest); // do we need this?
}

void temp_info() {
    ArenaBuffer* a = &runtime.temp_buffer;
    printf(
        "\nTemp Buffer Info:\n"
        "Data:      %p\n"
        "Size:      %lld\n"
        "Allocated: %lld\n"
        "Highest:   %lld\n\n",
        a->data, a->size, a->allocated, a->highest
    );
}








/* ==== String: Basic ==== */

void upper_case_first_char(String in) {
    in.data[0] += 'A' - 'a';
}

String string_advance(String s, u64 p) {
    return (String) {s.data + p, s.count - p};
}

String string_copy(String s) {
    u8* data = runtime.alloc(s.count);
    for (u64 i = 0; i < s.count; i++) data[i] = s.data[i]; // todo: speed
    return (String) {data, s.count};
}

u8 string_equal(String a, String b) {
    if (a.count != b.count) return 0;
    if (a.data  == b.data ) return 1;
    for (u64 i = 0; i < a.count; i++) {
        if (a.data[i] != b.data[i]) return 0; // todo: speed
    }
    return 1;
}

String string_find(String a, String b) {
    
    if (!a.count || !b.count || !a.data || !b.data || (b.count > a.count)) return (String) {0};

    for (u64 i = 0; i < a.count - b.count + 1; i++) {
            
        for (u64 j = 0; j < b.count; j++) {
            if (a.data[i + j] != b.data[j]) goto next;
        }
        
        return (String) {a.data + i, a.count - i};
        next: continue;
    }
    
    return (String) {0};
}

String string_find_and_skip(String a, String b) {
    String result = string_find(a, b);
    if (!result.count) return result; 
    return (String) {result.data + b.count, result.count - b.count};
}

String string_concat(Array(String) strings) {
    
    u64 count = 0;
    for (u64 i = 0; i < strings.count; i++) {
        count += strings.data[i].count;
    }

    String out = {
        .data  = runtime.alloc(count),
        .count = count,
    };
    
    u64 counter = 0;
    for (u64 i = 0; i < strings.count; i++) {
        String s = strings.data[i];
        for (u64 j = 0; j < s.count; j++) {
            out.data[counter + j] = s.data[j];
        }
        counter += s.count;
    }
    
    return out;
}

// todo: cleanup, avoid two loops, handle multiple space, end with separator, etc.
Array(String) string_split(String s, String separator) {
    
    Array(String) out = {0};
    u64 count = 1;
    
    {
        String pos = s;
        while (1) {
            pos = string_find_and_skip(pos, separator);
            if (!pos.count) break;
            count++;
        }
    }

    // will still allocate if count is 1
    out.data  = runtime.alloc(sizeof(String) * count); 
    out.count = count;
    
    {
        String pos = s;
        for (u64 i = 0; i < count; i++) {
            String next = string_find(pos, separator);
            out.data[i] = (String) {pos.data, pos.count - next.count}; // if next.count is 0 then this is the end
            if (next.count) pos = string_advance(next, separator.count);
        }
    }

    return out;
}

String string_join(Array(String) s, String seperator) {
    
    u64 result_count = 0;
    for (u64 i = 0; i < s.count; i++) result_count += s.data[i].count;   
    result_count += seperator.count * (s.count - 1);
    
    u8* data = runtime.alloc(result_count); 

    u64 start = 0;
    for (u64 i = 0; i < s.count; i++) {
        for (u64 j = 0; j < s.data[i].count; j++) data[start + j] = s.data[i].data[j];
        start += s.data[i].count;
        for (u64 i = 0; i < seperator.count; i++) data[start + i] = seperator.data[i];
        start += seperator.count;
    }
   
    return (String) {data, result_count};
}

String string_replace(String s, String a, String b) {
    Array(String) chunks = string_split(s, a);
    if (chunks.count < 2) return s;
    String result = string_join(chunks, b);
    return result;
}





/* ==== Print ==== */

// basic print
void print_string(String s) {
    for (int i = 0; i < s.count; i++) putchar(s.data[i]);
}

// todo: not robust, no escape with @, need more testing, deal with switching back allocator
void print(String s, ...) {
    
    void* (*old_alloc)(u64) = runtime.alloc; // ehh....
    runtime.alloc = temp_alloc;
    
    Array(String) chunks = string_split(s, string("@"));
   
    runtime.alloc = old_alloc;
    
    if (chunks.count < 2) { print_string(s); return; }
    
    u64 arg_count = chunks.count - 1;
    va_list args;
    va_start(args, s);

    print_string(chunks.data[0]);
    for (u64 i = 0; i < arg_count; i++) {
        print_string(va_arg(args, String)); // not safe, but this is C varargs, what can you do 
        print_string(chunks.data[i + 1]);
    }

    va_end(args);
}










/* ==== Main Program ==== */


String alphabet[4];
String atomic_types[11];

char* bullshit_catalog[] = {
    "object",
    "class",
    "interface",
    "builder",
    "string",
    "friend",
    "factory",
    "tester",
    "counter",
    "converter",
    "observer",
    "initializer",
    "coverage",
    "pointer",
    "hash",
    "queue",
    "stack",
    "list",
    "vector",
    "pair",
    "tuple",
    "align",
    "concept",
    "decl",
    "dynamic",
    "explicit",
    "exporter",
    "mutable",
    "namespace",
    "private",
    "protected",
    "public",
    "synchronized",
    "template",
    "virtual",
    "ordered",
    "unordered",
    "regex",
    "mutex",
    "lock",
    "unlock",
};

// leak, use temp_alloc() for all for now
String get_random_word_from_bullshit_catalog() {
    char* s = bullshit_catalog[rand() % length_of(bullshit_catalog)];
    return string_copy((String) {(u8*) s, strlen(s)});
}

String get_random_word(int length) {
    
    String out = {temp_alloc(length), length};
    
    // 0 1 2 3 for q a n z
    u8 now  = 0; 
    u8 next = 0;

    now = rand() % 2;

    for (int i = 0; i < length; i++) {
        
        String a = alphabet[now];
        out.data[i] = a.data[rand() % a.count];
        
        switch (now) {
            case 0: next = 1;              break;
            case 1: next = rand() % 2 + 2; break;
            case 2: 
            case 3: next = rand() % 2;     break;
        }

        now = next;
    }
    
    switch (out.data[length - 1]) {
        case 'a': case 'e': case 'i': case 'o': case 'u': case 'y': break;
        default: {
            String a = alphabet[1];
            out.data[length - 1] = a.data[rand() % a.count];
            break;
        }
    }

    return out;
}


typedef enum {
    STYLE_SNAKE,         // snake_case               
    STYLE_CAMEL,         // camelCase                
    STYLE_PASCAL,        // PascalCase                
    STYLE_PASCAL_SNAKE,  // Pascal_Snake_Case               
} CodeStyle;

// leak, use temp_alloc() for all for now
String get_random_identifier(int count, int word_min, int word_max, CodeStyle style) {
    
    u8 more = (style == STYLE_SNAKE || style == STYLE_PASCAL_SNAKE) && count > 1;
    if (more) count += count - 1;

    Array(String) ss = {temp_alloc(sizeof(String) * count), count};
    //int rand_length = word_max - word_min;

    for (int i = 0; i < count; i++) {

        if (more && (i % 2)) {
            ss.data[i] = string("_");
            continue;
        }

        ss.data[i] = get_random_word_from_bullshit_catalog();
        //ss.data[i] = get_random_word(rand() % rand_length + word_min);

        if (style != STYLE_SNAKE) {
            if (style == STYLE_CAMEL && i == 0) continue;
            upper_case_first_char(ss.data[i]);
        }
    }
    
    return string_concat(ss);
}


typedef struct {
    String        name;
    Array(String) fields; // interleaved type name type name ...
} TypeInfo;


void print_type_to_code(TypeInfo type) {
    
    u64 full_count = type.fields.count / 2;
    print(string("typedef struct {\n"));
    for (u64 i = 0; i < full_count; i++) {
        print(
            string("    @ @;\n"),
            type.fields.data[i * 2    ],
            type.fields.data[i * 2 + 1]
        );
    }
    print(string("} @;\n\n"), type.name);
}

// leak, use temp_alloc() for all for now
TypeInfo get_random_struct_using_name_pool(u64 field_count, Array(String) pool) {
    
    TypeInfo out;
    out.name = get_random_identifier(rand() % 3 + 2, 3, 7, STYLE_PASCAL);

    u64 count = field_count;
    out.fields = (Array(String)) {
        runtime.alloc(sizeof(String) * count * 2),
        count * 2,
    };
    
    for (u64 i = 0; i < count; i++) {
        out.fields.data[i * 2    ] = pool.data[rand() % pool.count];
        out.fields.data[i * 2 + 1] = get_random_identifier(rand() % 3 + 1, 3, 7, STYLE_CAMEL);
    }

    return out;
}




int main() {

    // setup runtime
    {
        u64 size = 1024 * 256;
        runtime.temp_buffer.data = calloc(size, sizeof(u8));
        runtime.temp_buffer.size = size;
        runtime.alloc = malloc;
    }
    
    // generate random seed
    {
        struct timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);
        srand(start.tv_nsec);
    }

    // gcc won't let us use literals in -pedantic
    {
        alphabet[0] = string("qwrtypsdfghjklzxcvbnm");
        alphabet[1] = string("aeiouy");
        alphabet[2] = string("nmr");
        alphabet[3] = string("zxcvtpdgkb");

        atomic_types[0]  = string("u8");
        atomic_types[1]  = string("u16");
        atomic_types[2]  = string("u32");
        atomic_types[3]  = string("u64");
        atomic_types[4]  = string("s8");
        atomic_types[5]  = string("s16");
        atomic_types[6]  = string("s32");
        atomic_types[7]  = string("s64");
        atomic_types[8]  = string("f32");
        atomic_types[9]  = string("f64");
        atomic_types[10] = string("String");
    }

    printf(
        "typedef unsigned char           u8;\n"
        "typedef unsigned short int      u16;\n"
        "typedef unsigned int            u32;\n"
        "typedef unsigned long long int  u64;\n"
        "typedef signed char             s8;\n"
        "typedef signed short            s16;\n"
        "typedef signed int              s32;\n"
        "typedef signed long long int    s64;\n"
        "typedef float                   f32;\n"
        "typedef double                  f64;\n"
        "\n"
        "typedef struct {\n"
        "    u8* data;\n"
        "    u64 count;\n"
        "} String;\n\n"
    );
    
    u64 count = 32;
    Array(String) type_name_pool = {malloc(sizeof(String) * count), count};
    TypeInfo*     types          =  malloc(sizeof(TypeInfo) * count);

    runtime.alloc = temp_alloc;
    
    for (u64 i = 0; i < count; i++) {
        types[i] = get_random_struct_using_name_pool(rand() % 5 + 2, array(String, atomic_types));
        type_name_pool.data[i] = types[i].name;
    }
    
    for (u64 i = 0; i < count; i++) print_type_to_code(types[i]);
    
    for (u64 i = 0; i < 64; i++) {
        print_type_to_code(get_random_struct_using_name_pool(rand() % 5 + 2, type_name_pool));
    }
    
    temp_reset();    

    printf(
        "int main() {\n"
        "\n"
        "\n"
        "}\n"
    );
}

