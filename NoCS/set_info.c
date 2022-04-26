#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>




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
    s32* data;
    u64  count;
} Set;

// todo: merge this with Set?
typedef struct {
    u8  data[12];
    s32 count;
    s32 value;
} SetInfo;

Define_Array(SetInfo);


// todo: cleanup
Set temp_set_from_set_info(SetInfo info) {
    Set set = {temp_alloc(info.count * sizeof(int)), info.count};
    for (int i = 0; i < info.count; i++) set.data[i] = info.data[i];
    return set;
}


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
Array(SetInfo) get_pure_tertian_sets(Array(SetInfo) sets) {

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

    out.count = acc;

    return out;   
}

// todo: maybe slow
Array(SetInfo) get_sets_by_PV(Array(SetInfo) sets, int value) {

    u64 count = 0;
    for (u64 i = 0; i < sets.count; i++) {
        if (sets.data[i].value == value) count++;
    }
    
    Array(SetInfo) out = {temp_alloc(count * sizeof(SetInfo)), count};
    
    u64 acc = 0;
    for (u64 i = 0; i < sets.count; i++) {
        if (sets.data[i].value == value) {
            out.data[acc] = sets.data[i];
            acc++;
        }
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

int compare_count_ascend(const void* a, const void* b) {
    SetInfo* sa = (SetInfo*) a;
    SetInfo* sb = (SetInfo*) b;
    int ac = sa->count;
    int bc = sb->count;
    if      (ac > bc) return  1;
    else if (ac < bc) return -1;
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
00 ff 03 00      name

    note on            note off 
00  90 48 66   83 60   80 48 66        
00  90 48 66   83 60   80 48 66 
00  90 48 66   83 60   80 48 66 
00  90 48 66   83 60   80 48 66 

00  ff 2f 00     end

*/

u16 swap_endian_u16(u16 in) {
    return (in & 0x00ff) << 8 | (in & 0xff00) >> 8;
}

u32 swap_endian_u32(u32 in) {
    return (
        (in & 0x000000ff) << 24 |
        (in & 0x0000ff00) << 8  |
        (in & 0x00ff0000) >> 8  |
        (in & 0xff000000) >> 24 
    );
}


// not used for now
String get_global_tick(int tick_per_whole_note) {

    String out = {temp_alloc(2), 2};
    u16 tick = tick_per_whole_note / 4; 

    *((u16*) out.data) = swap_endian_u16(tick);
    *out.data &= 0x7f; // set top bit, only bpm for now
    
    return out;
}

// not used for now
String get_note_length(int tick, int numer, int denom) {

    String out = {temp_alloc(4), 4};
    u32 dt = tick * numer / denom;
    
    u32 buffer = dt & 0x7f;

    // todo: clean up
    while (dt >>= 7) {
        buffer <<= 8;
        buffer |= (dt & 0x7f) | 0x80;
    }
    
    int i;
    for (i = 0; i < 4; i++) {
        out.data[i] = buffer; 
        if (buffer & 0x80) buffer >>= 8;
        else break;
    }
    out.count = i + 1;
    
    return out;
}


u64 append_note(StringBuilder* builder, u8 note) {
    u64 count = 0;
    u8  on_off[] = {0x00,  0x90, note, 0x66,   0x83, 0x60,   0x80, note, 0x66};
    count += builder_append(builder, data_string(on_off));
    return count;
}

u64 append_set_arp(StringBuilder* builder, Set set) {
    u64 count = 0;
    for (int i = 0; i < set.count; i++) {
        u8 note[] = {
            0x00, 
            0x90, 60 + set.data[i], 0x66,
            0x83, 0x60,
            0x80, 60 + set.data[i], 0x66,
        };
        count += builder_append(builder, data_string(note));
    }
    return count;
}

u64 append_set_chord(StringBuilder* builder, Set set) {

    if (!set.count) return 0; // maybe add rest?
    
    u64 count = 0;
    for (int i = 0; i < set.count; i++) {
        u8 on[] = {0x00,    0x90, 60 + set.data[i], 0x66};
        count += builder_append(builder, data_string(on));
    }

    u8 dt[] = {0x83, 0x60,   0x80, 60 + set.data[0], 0x66};
    count += builder_append(builder, data_string(dt));

    for (int i = 1; i < set.count; i++) {
        u8 off[] = {0x00,    0x80, 60 + set.data[i], 0x66};
        count += builder_append(builder, data_string(off));
    }

    return count;
}



void save_midi_for_sets(Array(SetInfo) sets, u64 (*append)(StringBuilder*, Array(SetInfo)), char* name) {

    StringBuilder builder = builder_init();
    
    u64 stop_at = 0; // for MTrk chunk's length
    {
        stop_at += builder_append(&builder, string("MThd"));
        u8 info[] = {
            0x00, 0x00, 0x00, 0x06,
            0x00, 0x01,
            0x00, 0x01,
            0x01, 0xe0, // 480
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
        count += append(&builder, sets);
        count += builder_append(&builder, data_string(end));
        
        u32* p = (u32*) (builder.base.data + stop_at); // for MTrk chunk's length       
        *p = swap_endian_u32((u32) count);
    }
    
    save_file(builder.base, name);
    builder_free(&builder); 
}



/* ---- save_midi_for_sets()'s "lambda functions" ---- */

u64 for_sets_do_nothing_but_append_sample_code(StringBuilder* builder, Array(SetInfo) sets) {
    
    u64 count = 0;

    int dorian[] = {0, 2, 3, 5, 7, 9, 10};
    int Cmin9[]  = {0, 3, 7, 10, 12 + 2};
    int Dmin9[]  = {2, 5, 9, 12, 12 + 4};
    
    count += append_note(builder, 60 - 2);
    count += append_note(builder, 60 + 2);
    count += append_set_arp(builder, make_set(dorian));
    count += append_set_chord(builder, make_set(Cmin9));
    count += append_set_chord(builder, make_set(Cmin9));
    count += append_set_chord(builder, make_set(Dmin9));
    count += append_set_chord(builder, make_set(Dmin9));
    
    return count;
}

u64 for_sets_append_arp(StringBuilder* builder, Array(SetInfo) sets) {
    u64 count = 0;
    for (u64 i = 0; i < sets.count; i++) {
        count += append_set_arp(builder, temp_set_from_set_info(sets.data[i]));
    }
    return count;
}

u64 for_sets_append_tertian_form_chord(StringBuilder* builder, Array(SetInfo) sets) {
    
    u64 count = 0;
    for (u64 i = 0; i < sets.count; i++) {
         
        SetInfo* set = &sets.data[i];
        if (set->count % 2 == 0)  continue;
        
        int temp[12];
        for (int j = 0; j < set->count; j++) {
            int k = j * 2 % set->count;
            temp[j] = set->data[k]; 
        }
        
        for (int j = 0; j < set->count - 1; j++) {
            int a = temp[j];
            int b = temp[j + 1];
            if (b <= a) temp[j + 1] += 12;
        }
        
        count += append_set_chord(builder, (Set) {temp, set->count});
    }

    return count;
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
    save_midi_for_sets(all_sets, for_sets_append_arp, "all_sets.mid");

    /* ---- Polarity Value ---- */
    {
        int weighting[12] = {0, 0, 5, 4, 3, 2, 1, -5, -4, -3, -2, -1};
        //int weighting[12] = {0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1};
        translate_IC7_order_to_index_order(weighting);
        fill_all_polarity_values(all_sets, weighting);

        qsort(all_sets.data, all_sets.count, sizeof(SetInfo), compare_count_ascend_then_value_descend);
        
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
        Array(SetInfo) pts = get_pure_tertian_sets(all_sets);
        printf("\nPure Tertian Sets\n");
        print_tertian_form(pts);
        
        save_midi_for_sets(pts, for_sets_append_arp, "pts.mid");
        save_midi_for_sets(pts, for_sets_append_tertian_form_chord, "pts_tertian.mid");
    }
    
}


