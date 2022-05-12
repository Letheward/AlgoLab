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

String concat(u64 count, ...) {
    
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

String concat_array(Array(String) strings) {
    
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

String format_u32(u32 value, s32 base) {

    u8* table = (u8*) "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_";

    if (value == 0) return string("0"); // quick fix, is this fast?
    if (base < 2 || base > 64) return (String) {0};

    u8 digits[32] = {0}; // todo: speed? just write it to the malloc() buffer?
    s32 count;
    {
        u32 temp = value;
        for (count = 0; temp > 0; count++) {
            digits[count] = table[temp % base]; // what about custom digits?
            temp /= base;
        }
    }

    u8* data = temp_alloc(32);
    for (int i = 0; i < count; i++) data[i] = digits[count - i - 1];

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





#define HashTable(K, V) HashTable__ ## K ## __ ## V
#define Define_HashTable(K, V) \
typedef struct {               \
                               \
    K*   keys;                 \
    V*   values;               \
    u32* hashes;               \
    u8*  occupied;             \
                               \
    u64  count;                \
    u64  allocated;            \
                               \
} HashTable(K, V)              \

#define table_init(K, V) table_init__ ## K ## __ ## V
#define table_free(K, V) table_free__ ## K ## __ ## V

Define_HashTable(String, String);


u32 get_hash_djb2(String s) {
    
    u32 hash = 5381;
    for (u64 i = 0; i < s.count; i++) {
        hash += (hash << 5) + s.data[i];
    }
    
    return hash;
}

HashTable(String, String) table_init(String, String)(u64 size) {
    return (HashTable(String, String)) {
        .keys      = calloc(size, sizeof(String)),
        .values    = calloc(size, sizeof(String)),
        .hashes    = calloc(size, sizeof(u32)),
        .occupied  = calloc(size, sizeof(u8)),
        .count     = 0,
        .allocated = size,
    };
}

void table_free(String, String)(HashTable(String, String)* table) {
    free(table->keys);
    free(table->values);
    free(table->hashes);
    free(table->occupied);
}


void table_put(HashTable(String, String)* table, String key, String value) {

    u32 hash  = get_hash_djb2(key);
    u64 index = hash % table->allocated;
    
    while (table->occupied[index]) {
/*/
        print(
            string("put: Collision of \"@\" and \"@\", hashes @ and @\n"), 
            key, table->keys[index], format_u32(hash, 16), format_u32(table->hashes[index], 16)
        );
/*/
        index = (index + hash) % table->allocated;
    }

    table->keys    [index] = key;
    table->values  [index] = value;
    table->hashes  [index] = hash;
    table->occupied[index] = 1;

    table->count++;
}

String table_get(HashTable(String, String)* table, String key) {

    u32 hash  = get_hash_djb2(key);
    u64 index = hash % table->allocated;
    
    u64 count = 0;
    while (!string_equal(key, table->keys[index])) {
/*/
        print(
            string("get: Collision of \"@\" and \"@\", hashes @ and @\n"), 
            key, table->keys[index], format_u32(hash, 16), format_u32(table->hashes[index], 16)
        );
/*/
        index = (index + hash) % table->allocated; 
        count++;
        if (count >= table->allocated) return string("");
    }

    return table->values[index];
}


void print_table(HashTable(String, String) table) {
    printf("\n");
    for (int i = 0; i < table.allocated; i++) {
        printf("%4d ", i);
        if (table.occupied[i]) {
            print(table.keys[i]);
            printf(" - ");
            print(table.values[i]);
        }
        printf("\n");
    }
    printf("\n");
}



String linear_search(Array(String) keys, Array(String) values, String key) {

    for (u64 i = 0; i < keys.count; i++) {
        if (string_equal(key, keys.data[i])) return values.data[i];
    }
    
    return string("");
}


// todo: fix bugs, make resize work


/* ==== Main Program ==== */

void program() {
    
    String search = string("SDL");
    Array(String) args = runtime.command_line_args;
    if (args.count > 1) search = args.data[1];


    HashTable(String, String) table = table_init(String, String)(512);

    String data = load_file("data.txt");
    Array(String) lines = string_split(data, string("\n"));

    runtime.alloc = temp_alloc;
    {
        for (int i = 0; i < lines.count; i++) {
            if (!lines.data[i].count) continue;
            Array(String) kv = string_split(lines.data[i], string(" - ")); 
            if (kv.count > 1) table_put(&table, kv.data[0], kv.data[1]);
        }
        
        time_it();
        String result = table_get(&table, search);
        time_it();

        print(result);
        printf("\n");
    }
    temp_reset();

    {
        Array(String) keys   = {temp_alloc(lines.count * sizeof(String)), lines.count}; 
        Array(String) values = {temp_alloc(lines.count * sizeof(String)), lines.count};
        
        u64 count = 0;
        for (int i = 0; i < lines.count; i++) {
            if (!lines.data[i].count) continue;
            Array(String) kv = string_split(lines.data[i], string(" - "));
            if (kv.count > 1) {
                keys.data[count]   = kv.data[0];
                values.data[count] = kv.data[1];
            }
            count++;
        }
        
        keys.count = count;
        values.count = count;

        time_it();
        String result = linear_search(keys, values, search);
        time_it();

        print(result);
        printf("\n");
    }
    temp_reset();



//    print_table(table);







}





