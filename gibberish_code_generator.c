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

typedef struct {
    u8* data;
    u64 count;
    u64 allocated;
} StringBuilder;




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




/* ==== Utils ==== */

void error(char* s, ...) {

    va_list va;
    va_start(va, s);

    printf("[Error] ");
    vprintf(s, va);

    exit(1); 
}

void generate_random_seed() {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    srand(start.tv_nsec);
}





/* ==== Timer ==== */

typedef struct {
    struct timespec start;
    struct timespec end;
    u8 which;
} PerformanceTimer;

PerformanceTimer timer;

void time_it() {
    if (timer.which == 0) {
        clock_gettime(CLOCK_MONOTONIC, &timer.start);
        timer.which = 1;
    } else {
        clock_gettime(CLOCK_MONOTONIC, &timer.end);
        printf("%fs\n", (timer.end.tv_sec - timer.start.tv_sec) + 1e-9 * (timer.end.tv_nsec - timer.start.tv_nsec));
        timer.which = 0;
    }
}






/* ==== String: Basic ==== */

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

// dumb linear search for now
String string_find(String a, String b) {
    
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

String string_concat(u64 count, ...) {
    
    u64 size = sizeof(String) * count;
    Array(String) strings = {
        .data  = temp_alloc(size),
        .count = count,
    };

    String out = {0};
    {
        va_list args;
        va_start(args, count);
        for (int i = 0; i < count; i++) {
            String s = va_arg(args, String);
            strings.data[i] = s;
            out.count += s.count;
        }
        va_end(args);
    }

    out.data = runtime.alloc(out.count);
    
    u64 counter = 0;
    for (int i = 0; i < count; i++) {
        String s = strings.data[i];
        for (int j = 0; j < s.count; j++) {
            out.data[counter + j] = s.data[j];
        }
        counter += s.count;
    }
    
    return out;
}

String string_concat_array(Array(String) strings) {
    
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

// todo: avoid two loops, handle multiple space, end with separator, etc.
Array(String) string_split(String s, String separator) {
    
    Array(String) out = {0};

    String pos = s;

    u64 count = 1;
    while (pos.count > 0) {
        pos = string_find(pos, separator);
        if (!pos.data) break;
        pos = string_advance(pos, separator.count);
        count++;
    }

    out.data  = runtime.alloc(sizeof(String) * count);
    out.count = count;

    pos = s;
    
    for (u64 i = 0; i < count; i++) {
        String new = string_find(pos, separator);
        if (!new.data) {out.data[i] = pos; break;}
        out.data[i] = (String) {pos.data, new.data - pos.data};
        pos = string_advance(new, separator.count);
    };

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





/* ==== String: Formatting ==== */

String format_s32(s32 value, s32 base) {

    u8* table = (u8*) "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_";

    if (value == 0) return string("0"); // quick fix, is this fast?
    if (base < 2 || base > 64) return (String) {0};

    u8 digits[32] = {0}; // todo: speed? just write it to the malloc() buffer?
    s32 sign = value >> 31;
    s32 count;
    {
        s32 temp = sign ? -value : value;
        for (count = 0; temp > 0; count++) {
            digits[count] = table[temp % base]; // what about custom digits?
            temp /= base;
        }
    }

    u8* data = temp_alloc(32);
    if (sign) {data[0] = '-'; count++;}
    for (int i = sign ? 1 : 0; i < count; i++) data[i] = digits[count - i - 1];

    return (String) {data, count};
}

// todo: accuracy issue, handle infinity and NaN
String format_f32(float value) {

    u8* table = (u8*) "0123456789";
    
    u32 data; 
    {
        u32 * p = (u32*) &value; // workaround for aliasing warning
        data = *p; 
    }

    // get data from IEEE 754 bits
    int sign = (data >> 31);
    int exp  = (data << 1 >> 24) - 127 - 23;
    int frac = (data << 9 >> 9) | 0x800000; // add the implicit "1." in ".1010010"

    u64 temp = frac * 1000000000000; // todo: accuracy 

    if (exp < 0) for (int i = 0; i > exp; i--) temp /= 2;
    else         for (int i = 0; i < exp; i++) temp *= 2;

    u8 digits[64] = {0};
    int count; // digit count
    for (count = 0; temp > 0; count++) {
        digits[count] = table[temp % 10]; 
        temp /= 10;
    }
    
    u8* result = temp_alloc(64);
    u64 result_count = 0;
    
    if (sign) result[0] = '-';
    int dot_pos = count - 12; // todo: find out why it's 12
    
    if (dot_pos <= 0) {
        result_count = count - dot_pos + 2 + sign;
        result[sign    ] = '0'; 
        result[sign + 1] = '.';
        for (int i = 0; i < -dot_pos; i++) result[sign + 2 + i] = '0'; 
        for (int i = 0; i < count; i++) {
            result[sign - dot_pos + i + 2] = digits[count - i - 1]; 
        }
    } else {
        result_count = count + sign + 1;
        result[sign + dot_pos] = '.';
        for (int i =       0; i < dot_pos; i++) result[sign + i    ] = digits[count - i - 1];
        for (int i = dot_pos; i <   count; i++) result[sign + i + 1] = digits[count - i - 1];
    }

    return (String) {result, result_count};
}

// modified from a [public domain library](https://github.com/badzong/base64/)
String base64_encode(String in) {
    
    const u8* table = (u8*) "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // allocate buffer
    u64 count = 4 * ((in.count + 2) / 3);
    u8* data  = runtime.alloc(count);

    // convert
    u64 j = 0;
    for (u64 i = 0; i < in.count; i += 3) {
        
        u32 a = in.data[i    ];
        u32 b = in.data[i + 1];
        u32 c = in.data[i + 2];

        u32 triple = (a << 16) + (b << 8) + c;

        data[j    ] = table[(triple >> 18) & 0x3f];
        data[j + 1] = table[(triple >> 12) & 0x3f];
        data[j + 2] = table[(triple >>  6) & 0x3f];
        data[j + 3] = table[(triple      ) & 0x3f];

        j += 4;
    }

    // padding at the end
    switch (in.count % 3) {
        case 1: data[j - 2] = '=';
        case 2: data[j - 1] = '=';
    }

    return (String) {data, count};
}

// todo: not robust, no escape with @, need more testing
void print(String s, ...) {

    Array(String) chunks = string_split(s, string("@"));
    
    if (chunks.count < 2) {
        for (int i = 0; i < s.count; i++) putchar(s.data[i]);
        return;
    }
    
    u64 arg_count = chunks.count - 1;
    String* args_data = temp_alloc(sizeof(String) * arg_count);

    {
        va_list args;
        va_start(args, s);
        for (u64 i = 0; i < arg_count; i++) args_data[i] = va_arg(args, String);
        va_end(args);
    }

    String c = chunks.data[0];
    for (u64 j = 0; j < c.count; j++) putchar(c.data[j]);

    for (u64 i = 0; i < arg_count; i++) {
        String c   = chunks.data[i + 1];
        String arg = args_data[i]; 
        for (u64 j = 0; j < arg.count; j++) putchar(arg.data[j]);
        for (u64 j = 0; j < c.count;   j++) putchar(c.data[j]);
    }

    temp_free(sizeof(String) * arg_count);
}






/* ==== File IO ==== */

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




/* ==== C Interface ==== */

void program();

int main(int arg_count, char** args) {

    // setup runtime
    u64 size = 1024 * 256;
    runtime.temp_buffer.data = calloc(size, sizeof(u8));
    runtime.temp_buffer.size = size;
    runtime.alloc = malloc;
    
    runtime.command_line_args = (Array(String)) {
        .data  = malloc(sizeof(String) * arg_count),
        .count = arg_count,
    };

    for (int i = 0; i < arg_count; i++) {
        int len = strlen(args[i]);
        String* s = &runtime.command_line_args.data[i];
        s->data  = malloc(len);       
        s->count = len;
        memcpy(s->data, args[i], len);
    }

    program();
}





/* ==== Main Program ==== */


String alphabet[4];
String atomic_types[11];



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

void upper_case_first_char(String in) {
    in.data[0] += 'A' - 'a';
}


typedef enum {
    STYLE_SNAKE,         // snake_case               
    STYLE_CAMEL,         // camelCase                
    STYLE_PASCAL,        // PascalCase                
    STYLE_PASCAL_SNAKE,  // Pascal_Snake_Case               
} CodeStyle;

String get_random_identifier(int count, int word_min, int word_max, CodeStyle style) {
    
    u8 more = (style == STYLE_SNAKE || style == STYLE_PASCAL_SNAKE) && count > 1;
    if (more) count += count - 1;

    Array(String) ss = {temp_alloc(sizeof(String) * count), count};
    int rand_length = word_max - word_min;

    for (int i = 0; i < count; i++) {

        if (more && (i % 2)) {
            ss.data[i] = string("_");
            continue;
        }

        ss.data[i] = get_random_word(rand() % rand_length + word_min);

        if (style != STYLE_SNAKE) {
            if (style == STYLE_CAMEL && i == 0) continue;
            upper_case_first_char(ss.data[i]);
        }
    }
    
    return string_concat_array(ss);
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
        out.fields.data[i * 2 + 1] = get_random_identifier(rand() % 3 + 1, 3, 7, STYLE_SNAKE);
    }

    return out;
}








void program() {


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

    generate_random_seed();
    
    u64 count = 32;
    Array(String) type_name_pool = {malloc(sizeof(String) * count), count};
    TypeInfo*     types          =  malloc(sizeof(TypeInfo) * count);

    print(
        string(
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
        )
    );

    for (u64 i = 0; i < count; i++) {
        types[i] = get_random_struct_using_name_pool(rand() % 5 + 2, array(String, atomic_types));
        type_name_pool.data[i] = types[i].name;
    }
    
    for (u64 i = 0; i < count; i++) {
        print_type_to_code(types[i]);
    }
    
    temp_reset();

    runtime.alloc = temp_alloc;
    
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
