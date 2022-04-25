#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>




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




/* ==== Macros ==== */

#define string(s)            (String) {(u8*) s, sizeof(s) - 1}
#define data_string(s)       (String) {(u8*) s, sizeof(s)}
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

// todo: slow
void print_data(String s) {
    for (u64 i = 0; i < s.count; i++) {
        if (i % 16 == 0 && i) printf("\n");
        printf("%.2x ", s.data[i]);
    }
}








/* ==== StringBuilder: Basic ==== */

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

void print(String s) {
    for (int i = 0; i < s.count; i++) putchar(s.data[i]);
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





/* ==== Set Info ==== */

#define make_set(c_array) (Set) {c_array, length_of(c_array)}

typedef struct {
    int* data;
    u64  count;
} Set;

typedef struct {
    u8  data[12];
    int count;
    int value;
} SetInfo;

Define_Array(SetInfo);


Array(SetInfo) get_all_set_info() {
    
    Array(SetInfo) sets = {
        .data  = calloc(2048, sizeof(SetInfo)),
        .count = 2048,
    };

    const u32 done = 1 << 11;
    u32 binary     = 0;

    while (binary < done) {

        SetInfo* p = &sets.data[binary];

        int c = 1;
        for (int i = 0; i < 11; i++) {
            if (binary & (1 << i)) { // unpack bits 
                p->data[c] = i + 1;
                c++;
            }
        }
        p->count = c;

        binary++;
    }
    
    return sets;
}

void fill_all_polarity_values(Array(SetInfo) sets, int* weighting) {
    for (u64 i = 0; i < sets.count; i++) {
        SetInfo* p = &sets.data[i];
        for (int i = 0; i < p->count; i++) {
            p->value += weighting[p->data[i]]; // we need calloc() in the get_all_set_info() for this
        }
    }
}

void print_set_info(Array(SetInfo) sets) {

    for (int i = 0; i < sets.count; i++) {
        
        SetInfo* set = &sets.data[i];
        
        printf("Set {");
        for (int j = 0; j < set->count - 1; j++) {
            printf("%d, ", set->data[j]);
        }
        printf("%d}  Value: %d\n", set->data[set->count - 1], set->value);
    }
}

void print_set_PV_from_weighting(Set set, int* weighting) {
    
    printf("Set {");
    
    int value = 0;
    for (int j = 0; j < set.count - 1; j++) {
        int e = set.data[j];
        printf("%d, ", e);
        value += weighting[e];
    }

    int e = set.data[set.count - 1];
    value += weighting[e];

    printf("%d}  Value %d \n", e, value);
}

void print_PV_count_table(Array(SetInfo) sets, int* weighting) {
    
    int upper = 0;    
    int lower = 0;    

    for (int i = 0; i < 12; i++) {
        int v = weighting[i];
        if (v > 0) upper += v;
        if (v < 0) lower += v;
    }

    int  range = upper - lower + 1;
    int* table = calloc(range, sizeof(int));

    for (u64 i = 0; i < sets.count; i++) {
        int value = sets.data[i].value;
        table[value - lower]++;
    }

    printf("\n   PV  count\n");    
    for (int i = range - 1; i >= 0; i--) {
        printf("%5d  %5d\n", i + lower, table[i]);
    }

    free(table);
}

void print_tertian_form(Array(SetInfo) sets) {

    for (int i = 0; i < sets.count; i++) {
        
        SetInfo* set = &sets.data[i];
        if (set->count % 2 == 0)  continue;
        
        int temp[12];
        for (int j = 0; j < set->count; j++) {
            int k = j * 2 % set->count;
            temp[j] = set->data[k]; 
        }
        
        printf("Set {");
        for (int j = 0; j < set->count - 1; j++)  printf("%d, ", set->data[j]);
        printf("%d} ", set->data[set->count - 1]);
        
        printf("Tertian {");
        for (int j = 0; j < set->count - 1; j++)  printf("%d, ", temp[j]);
        printf("%d}\n", temp[set->count - 1]);
        
    }
}

// todo: handle non-generated sets, cleanup and refactor
Array(SetInfo) get_all_pure_tertian_set(Array(SetInfo) sets) {

    Array(SetInfo) out = {temp_alloc(28 * sizeof(SetInfo)), 28};
    
    int acc = 0;
    for (int i = 0; i < sets.count; i++) {
        
        SetInfo* set = &sets.data[i];
        if (set->count % 2 == 0)  continue;
        
        int temp[12];
        for (int j = 0; j < set->count; j++) {
            int k = j * 2 % set->count;
            temp[j] = set->data[k]; 
        }
        
        // check pure
        {
            for (int j = 0; j < set->count - 1; j++) {
                int diff = temp[j + 1] - temp[j];
                if (diff < 0) diff += 12;
                if (!(diff == 3 || diff == 4)) goto next;
            }

            int diff = 12 - temp[set->count - 1];
            if (!(diff == 3 || diff == 4)) goto next;
        }
        
        out.data[acc] = *set;
        acc++;
        next: continue;
    }

    return out;   
}




/* ---- Set Info: Helpers ---- */

//  0 6   7 2 9 4 11   5 10 3 8  1
//  to
//  0 1   2 3 4 5 6    7 8  9 10 11
void translate_IC7_order_to_index_order(int* weighting) {

    int copy[12];
    for (int i = 0; i < 12; i++) copy[i] = weighting[i];
    
    weighting[0 ]  = copy[0];
    weighting[6 ]  = copy[1];
    weighting[7 ]  = copy[2];
    weighting[2 ]  = copy[3];
    weighting[9 ]  = copy[4];
    weighting[4 ]  = copy[5];
    weighting[11]  = copy[6];
    weighting[5 ]  = copy[7];
    weighting[10]  = copy[8];
    weighting[3 ]  = copy[9];
    weighting[8 ]  = copy[10];
    weighting[1 ]  = copy[11];
}

int compare_value_descend(const void* a, const void* b) {
    SetInfo* sa = (SetInfo*) a;
    SetInfo* sb = (SetInfo*) b;
    int av = sa->value;
    int bv = sb->value;
    if      (av < bv) return  1;
    else if (av > bv) return -1;
    else              return  0;
}

int compare_count_descend(const void* a, const void* b) {
    SetInfo* sa = (SetInfo*) a;
    SetInfo* sb = (SetInfo*) b;
    int ac = sa->count;
    int bc = sb->count;
    if      (ac < bc) return  1;
    else if (ac > bc) return -1;
    else              return  0;
}

int compare_count_ascend_then_value_descend(const void* a, const void* b) {
    
    SetInfo* sa = (SetInfo*) a;
    SetInfo* sb = (SetInfo*) b;
    
    int ac = sa->count;
    int bc = sb->count;
    int av = sa->value;
    int bv = sb->value;
    
    if (ac > bc) return  1;
    if (ac < bc) return -1;
    
    // if count equals...
    if      (av < bv) return  1;
    else if (av > bv) return -1;
    else              return  0;
}







/* ==== MIDI Exporter ==== */

/*

A simple .mid file
 
4d 54 68 64      MThd
00 00 00 06      length
00 01            
00 01            
01 e0 

4d 54 72 6b      MTrk
00 00 00 2c      length
00 ff 03 00 

    note on            note off 
00  90 48 66   83 60   80 48 66        
00  90 48 66   83 60   80 48 66 
00  90 48 66   83 60   80 48 66 
00  90 48 66   83 60   80 48 66 

00  ff 2f 00     end

*/

u32 swap_endian_u32(u32 in) {
    return (
        (in & 0x000000ff) << 24 |
        (in & 0x0000ff00) << 8  |
        (in & 0x00ff0000) >> 8  |
        (in & 0xff000000) >> 24 
    );
}

String get_note(u8 note) {
    
    String out = {temp_alloc(9), 9};
    
    out.data[0] = 0x00;
    
    out.data[1] = 0x90;
    out.data[2] = note;
    out.data[3] = 0x66;

    out.data[4] = 0x83;
    out.data[5] = 0x60;

    out.data[6] = 0x80;
    out.data[7] = note;
    out.data[8] = 0x66;

    return out;   
}

String get_set_arp(Set set) {
    
    String out = {temp_alloc(9 * set.count), 9 * set.count};
    
    for (int i = 0; i < set.count; i++) {
        out.data[9 * i + 0] = 0x00;
        
        out.data[9 * i + 1] = 0x90;
        out.data[9 * i + 2] = 60 + set.data[i];
        out.data[9 * i + 3] = 0x66;

        out.data[9 * i + 4] = 0x83;
        out.data[9 * i + 5] = 0x60;

        out.data[9 * i + 6] = 0x80;
        out.data[9 * i + 7] = 60 + set.data[i];
        out.data[9 * i + 8] = 0x66;
    }

    return out;
}

String get_set_chord(Set set) {
    
    u64 count = 4 * 2 * set.count + 1; 
    String out = {temp_alloc(count), count};
    
    u64 acc = 0;
    for (int i = 0; i < set.count; i++) {
      
        out.data[4 * i + 0] = 0x00;
        
        out.data[4 * i + 1] = 0x90;
        out.data[4 * i + 2] = 60 + set.data[i];
        out.data[4 * i + 3] = 0x66;
     
        acc += 4;
    }

    out.data[acc    ] = 0x83;
    out.data[acc + 1] = 0x60;
 
    out.data[acc + 2] = 0x80;
    out.data[acc + 3] = 60 + set.data[0];
    out.data[acc + 4] = 0x66;

    acc += 5;

    for (int i = 0; i < set.count - 1; i++) {
      
        out.data[acc + 4 * i + 0] = 0x00;
        
        out.data[acc + 4 * i + 1] = 0x80;
        out.data[acc + 4 * i + 2] = 60 + set.data[i + 1];
        out.data[acc + 4 * i + 3] = 0x66;
    }

    return out;
}


void save_midi_sample_code() {

    StringBuilder builder = builder_init();
    
    u64 stop_at = 0; // for MTrk chunk's length
    {
        stop_at += builder_append(&builder, string("MThd"));
        u8 info[] = {
            0x00, 0x00, 0x00, 0x06,
            0x00, 0x01,
            0x00, 0x01,
            0x01, 0xe0,
        };
        stop_at += builder_append(&builder, data_string(info));
    }
 
    {
        stop_at += builder_append(&builder, string("MTrk"));

        u8 length[4]; // stop at
        u8 info[] = {0x00, 0xff, 0x03, 0x00};
        u8 end[]  = {0x00, 0xff, 0x2f, 0x00};
        
        u64 count   = 0;
       
        builder_append(&builder, data_string(length));

        count += builder_append(&builder, data_string(info));

        int dorian[] = {0, 2, 3, 5, 7, 9, 10};
        int Cmin9[]  = {0, 3, 7, 10, 12 + 2};
        int Dmin9[]  = {-2, 1, 5, 8, 12};
        
        count += builder_append(&builder, get_set_arp(make_set(dorian)));

        count += builder_append(&builder, get_set_chord(make_set(Cmin9)));
        count += builder_append(&builder, get_set_chord(make_set(Cmin9)));
        count += builder_append(&builder, get_set_chord(make_set(Dmin9)));
        count += builder_append(&builder, get_set_chord(make_set(Dmin9)));
        
        count += builder_append(&builder, data_string(end));
        
        u8* p = builder.base.data + stop_at; // for MTrk chunk's length
        *((u32*) p) = swap_endian_u32((u32) count);
    }
    
    save_file(builder.base, "test.mid");
    builder_free(&builder); 
}


void save_midi_for_sets(Array(SetInfo) sets, char* name) {

    StringBuilder builder = builder_init();
    
    u64 stop_at = 0; // for MTrk chunk's length
    {
        stop_at += builder_append(&builder, string("MThd"));
        u8 info[] = {
            0x00, 0x00, 0x00, 0x06,
            0x00, 0x01,
            0x00, 0x01,
            0x01, 0xe0,
        };
        stop_at += builder_append(&builder, data_string(info));
    }
 
    {
        stop_at += builder_append(&builder, string("MTrk"));

        u8 length[4] = {0}; // stop at
        u8 info[]    = {0x00, 0xff, 0x03, 0x00};
        u8 end[]     = {0x00, 0xff, 0x2f, 0x00};
       
        u64 count = 0;
        
        builder_append(&builder, data_string(length));

        count += builder_append(&builder, data_string(info));

        for (u64 i = 0; i < sets.count; i++) {

            // todo: clean up
            int set[12] = {0};
            SetInfo* si = &sets.data[i]; 
            for (int i = 0; i < si->count; i++) set[i] = (int) si->data[i];

            count += builder_append(&builder, get_set_arp((Set) {set, si->count}));
        }
        count += builder_append(&builder, data_string(end));
        
        u8* p = builder.base.data + stop_at; // for MTrk chunk's length       
        *((u32*) p) = swap_endian_u32((u32) count);
    }
    
    save_file(builder.base, name);
    builder_free(&builder); 
}







/* ==== Main Program ==== */

int main() {


    // setup runtime
    {
        u64 size = 1024 * 256;
        runtime.temp_buffer.data = calloc(size, sizeof(u8));
        runtime.temp_buffer.size = size;
        runtime.alloc = malloc;
    }

    Array(SetInfo) all_sets = get_all_set_info();
    save_midi_for_sets(all_sets, "all_sets.mid");    

    /* ---- Polarity Value ---- */
    {
        int weighting[12] = {0, 0, 5, 4, 3, 2, 1, -5, -4, -3, -2, -1};
        //int weighting[12] = {0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1};
        translate_IC7_order_to_index_order(weighting);
        fill_all_polarity_values(all_sets, weighting);

        qsort(all_sets.data, all_sets.count, sizeof(SetInfo), compare_value_descend);
        printf("All Sets by Polarity Value\n");
        print_set_info(all_sets);
        printf("\nPolarity Value Count Table\n");
        print_PV_count_table(all_sets, weighting); // todo: potential bug here for using other weighting
        
        /*
        // example of printing one set
        int set[] = {0, 2, 4, 6, 9};
        print_set_PV_from_weighting(make_set(set), weighting);
        */
    }

    /* ---- Tertian Form ---- */
    {
        Array(SetInfo) pts = get_all_pure_tertian_set(all_sets);
        printf("\nPure Tertian Sets\n");
        print_tertian_form(pts);
        save_midi_for_sets(pts, "pts.mid");    
    }

}




