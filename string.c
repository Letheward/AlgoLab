#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>




/* ==== Macros ==== */

#define string(s)            (String) {(u8*) s, sizeof(s) - 1}
#define array_string(s)      (String) {(u8*) s, sizeof(s)}
#define data_string(s)       (String) {(u8*) &s, sizeof(s)}
#define length_of(array)     (sizeof(array) / sizeof(array[0]))
#define array(Type, c_array) (Array(Type)) {c_array, length_of(c_array)}

#define Array(Type) Array_ ## Type
#define Define_Array(Type) \
typedef struct {           \
    Type* data;            \
    u64   count;           \
} Array(Type)              \




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
    String base;
    u64    allocated;
} StringBuilder;

Define_Array(String);





/* ==== Utils ==== */

void error(char* s, ...) {

    va_list va;
    va_start(va, s);

    printf("[Error] ");
    vprintf(s, va);
    
    va_end(va);

    exit(1); 
}

/* ---- Timer ---- */

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





/* ==== Temp Allocator ==== */

typedef struct {
    u8* data;
    u64 size;
    u64 allocated;
    u64 highest;
} ArenaBuffer;

struct {
   
    ArenaBuffer   temp_buffer;
    
    struct {
        char* data;
        u64   count;
    } input_buffer;

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







/* ==== StringBuilder: Basic ==== */

// todo: can only use heap allocator now, how to change allocator?

StringBuilder builder_init() {
    return (StringBuilder) {
        .base.data  = malloc(1024),
        .base.count = 0,
        .allocated  = 1024,
    };
}

void builder_free(StringBuilder* b) {
    free(b->base.data);
}

u64 builder_append(StringBuilder* b, String s) {
    
    u64 wanted = b->base.count + s.count;
    while (wanted > b->allocated) {
        b->base.data = realloc(b->base.data, b->allocated * 2);
        b->allocated *= 2;
    }

    for (u64 i = 0; i < s.count; i++)  b->base.data[b->base.count + i] = s.data[i];
    b->base.count = wanted;

    return s.count;
}





/* ==== String: Basic ==== */

// note: not guaranteed to be safe
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

// naive search for now
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


// if we want to use varargs, just make a stack array
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
        for (u64 j = 0; j < seperator.count; j++) data[start + j] = seperator.data[j];
        start += seperator.count;
    }
   
    return (String) {data, result_count};
}

String string_replace(String s, String a, String b) {

    void* (*old_alloc)(u64) = runtime.alloc; // ehh....
    runtime.alloc = temp_alloc;
    
    Array(String) chunks = string_split(s, a);
   
    runtime.alloc = old_alloc;
    
    if (chunks.count < 2) return s;
    String result = string_join(chunks, b);
    return result;
}





/* ==== String: Utils ==== */

// todo: validate
String string_trim_space_start(String s) {
    
    u64 i = 0;
    while (i < s.count) {
        switch (s.data[i]) {
            case ' ': case '\t': case '\n': break;
            default: goto found;
        }
        i++;    
    }
    
    return (String) {0};
    found: return (String) {s.data + i, s.count - i};
}

// todo: validate
String string_trim_space_end(String s) {
    
    u64 i = s.count;
    while (i > 0) {
        i--;
        switch (s.data[i]) {
            case ' ': case '\t': case '\n': break;
            default: goto found;
        }
    }
    
    return (String) {0};
    found: return (String) {s.data, (u64) i + 1};
}

// todo: inline these calls?
String string_trim_space(String s) {
    return string_trim_space_end(string_trim_space_start(s));
}






/* ==== String: Formatting & Transform ==== */

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

// note: little endian
String format_binary(String s) {
    
    u64 row       = s.count / 8;
    u64 out_count = s.count * 9 + row;
    
    String out = {temp_alloc(out_count), out_count};
    
    u64 acc = 0;
    for (u64 i = 0; i < s.count; i++) {

        if (i % 8 == 0 && i > 0) {
            out.data[acc] = '\n';
            acc++;
        }
        
        for (u64 j = 0; j < 8; j++) {
            out.data[acc] = (s.data[i] & (0x1 << j)) ? '1': '0';
            acc++;
        }
       
        out.data[acc] = ' ';
        acc++;
    }
    
    return out;
}

// modified from a public domain library: https://github.com/badzong/base64/
// todo: validate
String base64_encode(String in) {
    
    if (!in.count || !in.data) return (String) {0};
    
    const u8* table = (u8*) "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // allocate buffer
    u64 count = 4 * ((in.count + 2) / 3);
    u8* data  = runtime.alloc(count); // we assume this will not fail for now

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
        case 1: data[j - 2] = '='; // fall-through
        case 2: data[j - 1] = '=';
    }

    return (String) {data, count};
}

// modified from a public domain library: https://github.com/badzong/base64/
// todo: cleanup, validate, handle more bad inputs
String base64_decode(String in) {

    if (!in.count || !in.data) return (String) {0};
	
    // initialize decode table
    u8 decode_table[256] = {0};
    {
        const u8* table = (u8*) "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (u64 i = 0; i < 64; i++) decode_table[table[i]] = i;
    }

	// remove trailing pad characters count, and compute out_count
    u64 real_count = in.count;
    u64 out_count;
    {
        u64 i = 0;
        while (in.data[real_count - i - 1] == '=') i++; // get pad length
        
        if (i > 2) real_count -= i - 2;
        if (real_count % 4) return (String) {0};
        
        out_count = (real_count / 4 * 3) - (i > 2 ? 2 : i); // remove at most 2 bytes.
    }
    
    // error check pass, todo: find a better way, and handle more bad inputs
    {
        u64 i = 0;
        while (i < real_count) {

            u32 a = decode_table[(u64) in.data[i++]];
            u32 b = decode_table[(u64) in.data[i++]];
            u32 c = decode_table[(u64) in.data[i++]];
            u32 d = decode_table[(u64) in.data[i++]];

            // if not zeroes at the end
            if (i != real_count) {
                if (!a || !b || !c || !d) return (String) {0}; // then they are non-base64 characters
            }
        }
    }
    
    u8* data = runtime.alloc(out_count); // we assume this will not fail for now

    u64 i = 0;
    u64 j = 0;
	while (i < real_count) {

		u32 a = decode_table[(u64) in.data[i++]];
		u32 b = decode_table[(u64) in.data[i++]];
		u32 c = decode_table[(u64) in.data[i++]];
		u32 d = decode_table[(u64) in.data[i++]];

		// zeroes at the end
		if (i == real_count) {
			if (in.data[real_count - 1] == '=') {
				d = 0;
				if (in.data[real_count - 2] == '=') c = 0;
			}
		}

		u32 triple = (a << 18) + (b << 12) + (c << 6) + d;

		data[j++] = (triple >> 16) & 0xff;
		data[j++] = (triple >>  8) & 0xff;
		data[j++] = (triple      ) & 0xff;
	}

	return (String) {data, out_count};
}

String rot13(String s) {
    
    const u8* table = (u8*) "NOPQRSTUVWXYZABCDEFGHIJKLM      nopqrstuvwxyzabcdefghijklm";
    
    String out = string_copy(s);

    for (u64 i = 0; i < out.count; i++) {
        u8 c = out.data[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            u8 d = c - 'A';        
            out.data[i] = table[d];
        }
    }

    return out;
}

// todo: validate
String rle64_encode(String s) {
    
    if (!s.count || !s.data) return (String) {0};
    
    // first pass    
    u64 item_count = 1;
    {
        u64 current_count = 0;
        u8  current = s.data[0];
        for (u64 i = 0; i < s.count; i++) {
            u8 c = s.data[i];
            if (current != c) {
                current = c;
                current_count = 1;
                item_count++;
            } else {
                current_count++;
            }
        }
    }
    
    u64 byte_count = item_count * (sizeof(u8) + sizeof(u64));
    String out = {runtime.alloc(byte_count), byte_count};

    // second pass    
    {
        u64 current_count = 0;
        u8  current = s.data[0];
        u64 acc = 0;
        for (u64 i = 0; i < s.count; i++) {
            u8 c = s.data[i];
            if (current != c) {
                
                out.data[acc]              = current;
                *(u64*) &out.data[acc + 1] = current_count;
                acc += sizeof(u8) + sizeof(u64);

                current = c;
                current_count = 1;

            } else {
                current_count++;
            }
        }
        
        // todo: figure out better ways
        out.data[acc]              = current;
        *(u64*) &out.data[acc + 1] = current_count;
        acc += sizeof(u8) + sizeof(u64);
    }

    return out;
}

// todo: validate
String rle8_encode(String s) {
    
    if (!s.count || !s.data) return (String) {0};
    
    // first pass    
    u64 item_count = 1;
    {
        u64 current_count = 0;
        u8  current = s.data[0];
        for (u64 i = 0; i < s.count; i++) {
            u8 c = s.data[i];
            if (current != c || current_count == 255) {
                current = c;
                current_count = 1;
                item_count++;
            } else {
                current_count++;
            }
        }
    }
    
    u64 byte_count = item_count * (sizeof(u8) + sizeof(u8));
    String out = {runtime.alloc(byte_count), byte_count};

    // second pass    
    {
        u64 current_count = 0;
        u8  current = s.data[0];
        u64 acc = 0;
        for (u64 i = 0; i < s.count; i++) {
            u8 c = s.data[i];
            if (current != c || current_count == 255) {
                
                out.data[acc]     = current;
                out.data[acc + 1] = (u8) current_count;
                acc += sizeof(u8) + sizeof(u8);

                current = c;
                current_count = 1;

            } else {
                current_count++;
            }
        }
        
        // todo: figure out better ways
        out.data[acc]     = current;
        out.data[acc + 1] = (u8) current_count;
        acc += sizeof(u8) + sizeof(u8);
    }

    return out;
}

// todo: validate
String rle64_decode(String s) {
    
    if (!s.count || !s.data) return (String) {0};
    
    u64 total_count = 0;
    for (u64 i = 0; i < s.count; i += sizeof(u8) + sizeof(u64)) {
        u64 count = *(u64*) &s.data[i + 1];
        total_count += count;
    }

    String out = {runtime.alloc(total_count), total_count};
    
    u64 acc = 0;
    for (u64 i = 0; i < s.count; i += sizeof(u8) + sizeof(u64)) {
        u8  c     = s.data[i];
        u64 count = *(u64*) &s.data[i + 1];
        for (u64 j = 0; j < count; j++) {
            out.data[acc] = c;
            acc++;
        }
    }
    
    return out;
}

// todo: validate
String rle8_decode(String s) {
    
    if (!s.count || !s.data) return (String) {0};
    
    u64 total_count = 0;
    for (u64 i = 0; i < s.count; i += sizeof(u8) + sizeof(u8)) {
        u64 count = s.data[i + 1];
        total_count += count;
    }

    String out = {runtime.alloc(total_count), total_count};
    
    u64 acc = 0;
    for (u64 i = 0; i < s.count; i += sizeof(u8) + sizeof(u8)) {
        u8 c     = s.data[i];
        u8 count = s.data[i + 1];
        for (u8 j = 0; j < count; j++) {
            out.data[acc] = c;
            acc++;
        }
    }
    
    return out;
}






/* ==== Standard IO ==== */

// basic print
void print_string(String s) {
    for (u64 i = 0; i < s.count; i++) putchar(s.data[i]);
}

// todo: not robust, need more testing, handle adjacent items (no space in between)
void print(String s, ...) {
    
    va_list args;
    va_start(args, s);
    
    for (u64 i = 0; i < s.count; i++) {

        u8 c = s.data[i];
        if (c == '@') {
            if (i + 1 < s.count && s.data[i + 1] == '@') { // short circuit 
                putchar('@');
                i++;
            } else {
                print_string(va_arg(args, String)); // not safe, but this is C varargs, what can you do 
            }
            continue;
        }

        putchar(c);
    }
    
    va_end(args);
}

// does not give a copy
String read_line() {
    
    char* buffer = runtime.input_buffer.data;
    u64   count  = runtime.input_buffer.count;

    fgets(buffer, count, stdin);
    String s = {(u8*) buffer, strlen(buffer)};
    
    if (s.count == 0) return (String) {0};
    if (s.data[s.count - 1] == '\n') s.count -= 1;
    
    return s;
}





/* ==== File IO ==== */

// todo: do we really need to switch allocator?
String load_file(char* path) {

    FILE* f = fopen(path, "rb");
    if (!f) error("Cannot load %s\n", path); 

    u8* data;
    u64 count;

    fseek(f, 0, SEEK_END);
    count = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = runtime.alloc(count);
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

    runtime.input_buffer.data  = calloc(8192, sizeof(u8));
    runtime.input_buffer.count = 8192;

    runtime.command_line_args = (Array(String)) {
        .data  = malloc(sizeof(String) * arg_count),
        .count = arg_count,
    };

    for (int i = 0; i < arg_count; i++) {
        runtime.command_line_args.data[i] = (String) {(u8*) args[i], strlen(args[i])};
    }
    
    setvbuf(stdout, NULL, _IONBF, 0); // force some shell to print immediately (so stdout before input will not be hidden) 

    program();
}




/* ==== Main Program ==== */

void program() {
    

    /* ---- Input ---- */
    
    // if you want then get it, no need to write a different main() and worry about namespace pollution
    Array(String) args = runtime.command_line_args; 

    String password;
    if (args.count > 1) {
        password = args.data[1];
    } else {
        print(string("Please input password:\n"));
        password = read_line();
    }


    /* ---- String Builder ---- */
    StringBuilder builder = builder_init();

    builder_append(&builder, string("Got password: "));
    builder_append(&builder, password);
    builder_append(&builder, string("\n"));

    print_string(builder.base);

    builder_free(&builder);


    /* ---- Print, Transforms, Temp Allocator ---- */
    runtime.alloc = temp_alloc; // string_replace() and rot13() will not leak because of temp allocator

    print(string_replace(string("This is a string.\n"), string("string"), string("cat")));
    
    print(
        string("She is @ meters high, likes @, and has password @.\nHer email is cat@@cat.meow\n"), 
        format_s32(42, 10), 
        string("atonal music"), 
        rot13(password) 
    );
    
    print(string("\n"));
    {
        s8  z = -1;
        f32 k =  1;
        print(
            string("Binary of -1 (s8) is @, binary of 1.0 (f32) is @.\nBinary of password \"@\" is \n@\n"), 
            format_binary(data_string(z)), 
            format_binary(data_string(k)),
            password,
            format_binary(password)
        );
    }

    temp_reset();
    temp_info();

}

