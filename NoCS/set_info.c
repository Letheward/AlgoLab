#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>




/* ==== Macros ==== */

#define string(s)            (String) {(u8*) s, sizeof(s) - 1}
#define array_string(s)      (String) {(u8*) s, sizeof(s)}
#define length_of(array)     (sizeof(array) / sizeof(array[0]))
#define array(Type, c_array) (Array(Type)) {c_array, length_of(c_array)}

#define Array(Type) Array_ ## Type
#define Define_Array(Type) \
typedef struct {           \
    Type* data;            \
    u64   count;           \
} Array(Type)              \

#define sort(array, f) qsort(array.data, array.count, sizeof(array.data[0]), f)




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

typedef struct {
    
    /* ---- Set ---- */
    // we store all data in place to simplify storage, this will add some redundant space, but it's not that much
    // to convert a SetInfo to a Set, just take the data and the count, like (Set) {info.data, info.count}
    s32 data[12]; 
    s32 count;

    /* ---- Metrics ---- */
    s32 polarity_value;                // overwritable by different weighting
    s32 interval_vector_ordered[11];
    s32 interval_vector_unordered[6];

} SetInfo;

Define_Array(SetInfo);





/* ---- Set Info: Helpers ---- */

//  0 6   7 2 9 4 11   5 10 3 8  1
//  to
//  0 1   2 3 4 5 6    7 8  9 10 11
void translate_weighting_IC7_order_to_index_order(int* weighting) {

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

//  1 2 3 4  5  6  7 8  9 10 11
//  to
//  7 2 9 4 11  6  5 10 3 8  1
void translate_OIV_index_order_to_IC7_order(int* in) {

    int copy[11];
    for (int i = 0; i < 11; i++) copy[i] = in[i];
    
    in[0]  = copy[6];
    in[1]  = copy[1];
    in[2]  = copy[8];
    in[3]  = copy[3];
    in[4]  = copy[10];
    in[5]  = copy[5];
    in[6]  = copy[4];
    in[7]  = copy[9];
    in[8]  = copy[2];
    in[9]  = copy[7];
    in[10] = copy[0];
}


int compare_value_descend(const void* a, const void* b) {
    SetInfo* sa = (SetInfo*) a;
    SetInfo* sb = (SetInfo*) b;
    int av = sa->polarity_value;
    int bv = sb->polarity_value;
    if (av < bv) return  1;
    if (av > bv) return -1;
    return  0;
}

int compare_count_ascend(const void* a, const void* b) {
    SetInfo* sa = (SetInfo*) a;
    SetInfo* sb = (SetInfo*) b;
    int ac = sa->count;
    int bc = sb->count;
    if (ac > bc) return  1;
    if (ac < bc) return -1;
    return  0;
}
   
int compare_count_ascend_then_value_descend(const void* a, const void* b) {
    int count = compare_count_ascend( a, b);
    int value = compare_value_descend(a, b);
    if (count != 0) return count;
    return value;
}


int compare_UIV_descend(const void* a, const void* b) {

    SetInfo* sa = (SetInfo*) a;
    SetInfo* sb = (SetInfo*) b;
    
    s32* av = sa->interval_vector_unordered;
    s32* bv = sb->interval_vector_unordered;
    
    for (int i = 0; i < 6; i++) {
        s32 ac = av[i];
        s32 bc = bv[i];
        if (ac < bc) return  1;
        if (ac > bc) return -1;
    }

    return 0;
}

int compare_OIV_descend(const void* a, const void* b) {

    SetInfo* sa = (SetInfo*) a;
    SetInfo* sb = (SetInfo*) b;
    
    s32* av = sa->interval_vector_ordered;
    s32* bv = sb->interval_vector_ordered;
    
    for (int i = 0; i < 11; i++) {
        s32 ac = av[i];
        s32 bc = bv[i];
        if (ac < bc) return  1;
        if (ac > bc) return -1;
    }

    return 0;
}

int compare_UIV_OIV_descend(const void* a, const void* b) {
    int UIV = compare_UIV_descend(a, b);
    int OIV = compare_OIV_descend(a, b);
    if (UIV != 0) return UIV;
    return OIV;
}

int compare_OIV_UIV_descend(const void* a, const void* b) {
    int OIV = compare_OIV_descend(a, b);
    int UIV = compare_UIV_descend(a, b);
    if (OIV != 0) return OIV;
    return UIV;
}

int compare_count_ascend_UIV_OIV_descend(const void* a, const void* b) {

    int count = compare_count_ascend(a, b);
    int UIV   = compare_UIV_descend( a, b);
    int OIV   = compare_OIV_descend( a, b);

    if (count != 0) return count;
    if (UIV   != 0) return UIV;
    return OIV;
}




/* ---- Set Info: Generators and Filters ---- */

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


// weighting's length must be 12 or more
void fill_polarity_values(Array(SetInfo) sets, int* weighting) {
    for (u64 i = 0; i < sets.count; i++) {
        SetInfo* p = &sets.data[i];
        p->polarity_value = 0;
        for (int i = 0; i < p->count; i++) {
            p->polarity_value += weighting[p->data[i]]; 
        }
    }
}

void fill_interval_vectors(Array(SetInfo) sets) {
    
    for (u64 i = 0; i < sets.count; i++) {

        SetInfo* set = &sets.data[i]; 
        for (int i = 0; i < 11; i++) set->interval_vector_ordered  [i] = 0;
        for (int i = 0; i < 6;  i++) set->interval_vector_unordered[i] = 0;

        for (u64 j = 0; j < set->count; j++) {
            for (u64 k = j + 1; k < set->count; k++) {
                
                int           index = set->data[k] - set->data[j];
                int unordered_index = index > 6 ? 12 - index : index;
                
                set->interval_vector_ordered  [          index - 1] += 1;
                set->interval_vector_unordered[unordered_index - 1] += 1;
            }
        }
    }
}



Array(SetInfo) get_sets_by_size(Array(SetInfo) sets, int size) {

    u64 count = 0;
    for (u64 i = 0; i < sets.count; i++) {
        if (sets.data[i].count == size) count++;
    }
    
    if (count == 0) return (Array(SetInfo)) {0};
    Array(SetInfo) out = {temp_alloc(count * sizeof(SetInfo)), count};
    
    u64 acc = 0;
    for (u64 i = 0; i < sets.count; i++) {
        if (sets.data[i].count == size) {
            out.data[acc] = sets.data[i];
            acc++;
        }
    }

    return out;
}

Array(SetInfo) get_sets_by_PV(Array(SetInfo) sets, int value) {

    u64 count = 0;
    for (u64 i = 0; i < sets.count; i++) {
        if (sets.data[i].polarity_value == value) count++;
    }
    
    if (count == 0) return (Array(SetInfo)) {0};
    Array(SetInfo) out = {temp_alloc(count * sizeof(SetInfo)), count};
    
    u64 acc = 0;
    for (u64 i = 0; i < sets.count; i++) {
        if (sets.data[i].polarity_value == value) {
            out.data[acc] = sets.data[i];
            acc++;
        }
    }
    
    return out;
}

// todo: cleanup
Array(SetInfo) get_sparse_sets(Array(SetInfo) sets) {

    u64 count = 0;
    for (u64 i = 0; i < sets.count; i++) {
        SetInfo* set = &sets.data[i];
        
        u64 c = set->count;
        if (c < 2) {
            count++;
            goto next1;
        }

        int d0 = set->data[    1] - set->data[0];
        int d1 = set->data[c - 1] - set->data[c - 2];
        int d2 = 12 - (set->data[c - 1] - set->data[0]);
        
        if ((d0 < 2 && d2 < 2) + (d1 < 2 && d2 < 2)) goto next1;

        for (int j = 0; j < c - 2; j++) {
            int d1 = set->data[j + 1] - set->data[j    ];
            int d2 = set->data[j + 2] - set->data[j + 1];
            if (d1 < 2 && d2 < 2) goto next1;
        }
    
        count++;
        next1: continue;
    }
    
    if (count == 0) return (Array(SetInfo)) {0};
    Array(SetInfo) out = {temp_alloc(count * sizeof(SetInfo)), count};
    
    u64 acc = 0;
    for (u64 i = 0; i < sets.count; i++) {
        SetInfo* set = &sets.data[i];
        
        u64 c = set->count;
        if (c < 2) {
            out.data[acc] = *set;
            acc++;
            goto next2;
        }

        int d0 = set->data[    1] - set->data[0];
        int d1 = set->data[c - 1] - set->data[c - 2];
        int d2 = 12 - (set->data[c - 1] - set->data[0]);
        
        if ((d0 < 2 && d2 < 2) + (d1 < 2 && d2 < 2)) goto next2;

        for (int j = 0; j < c - 2; j++) {
            int d1 = set->data[j + 1] - set->data[j    ];
            int d2 = set->data[j + 2] - set->data[j + 1];
            if (d1 < 2 && d2 < 2) goto next2;
        }
    
        out.data[acc] = *set;
        acc++;
        next2: continue;
    }

    return out;
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

Array(SetInfo) get_sets_by_OIV(Array(SetInfo) sets, s32* iv) {

    const u64 iv_count = 11;
    
    u64 count = 0;
    for (u64 i = 0; i < sets.count; i++) {
        SetInfo* set = &sets.data[i]; 
        for (u64 j = 0; j < iv_count; j++) {
            if (set->interval_vector_ordered[j] != iv[j]) goto next1;
        }
        count++;
        next1: continue;
    }
    
    if (count == 0) return (Array(SetInfo)) {0};
    Array(SetInfo) out = {temp_alloc(count * sizeof(SetInfo)), count};
    
    u64 acc = 0;
    for (u64 i = 0; i < sets.count; i++) {
        SetInfo* set = &sets.data[i]; 
        for (u64 j = 0; j < iv_count; j++) {
            if (set->interval_vector_ordered[j] != iv[j]) goto next2;
            out.data[acc] = *set;
        }
        acc++;
        next2: continue;
    }
    
    return out;
}

Array(SetInfo) get_sets_by_UIV(Array(SetInfo) sets, s32* iv) {

    const u64 iv_count = 6;
    
    u64 count = 0;
    for (u64 i = 0; i < sets.count; i++) {
        SetInfo* set = &sets.data[i]; 
        for (u64 j = 0; j < iv_count; j++) {
            if (set->interval_vector_unordered[j] != iv[j]) goto next1;
        }
        count++;
        next1: continue;
    }
    
    if (count == 0) return (Array(SetInfo)) {0};
    Array(SetInfo) out = {temp_alloc(count * sizeof(SetInfo)), count};
    
    u64 acc = 0;
    for (u64 i = 0; i < sets.count; i++) {
        SetInfo* set = &sets.data[i]; 
        for (u64 j = 0; j < iv_count; j++) {
            if (set->interval_vector_unordered[j] != iv[j]) goto next2;
            out.data[acc] = sets.data[i];
        }
        acc++;
        next2: continue;
    }
    
    return out;
}





/* ---- Set Info: Print ---- */

void print_set(Set set) {
    printf("{");
    for (int i = 0; i < set.count - 1; i++) {
        printf("%d, ", set.data[i]);
    }
    printf("%d}\n", set.data[set.count - 1]);
}


// the info chosen in print_set_info()'s options need to be initialized
typedef enum {
    INFO_MANY_LINE    = 0x01,
    INFO_COUNT        = 0x02,
    INFO_POLARITY     = 0x04,
    INFO_TERTIAN      = 0x08,
    INFO_IV_ORDERED   = 0x10,
    INFO_IV_UNORDERED = 0x20,
    INFO_ALL          = 0xffff, // may change
} Print_Set_Info_Options;

// note: this is somewhat complicated, maybe just copy and using different version?
void print_set_info(Array(SetInfo) sets, Print_Set_Info_Options options) {

    for (int i = 0; i < sets.count; i++) {
        
        SetInfo* set = &sets.data[i];
        
        printf("Set {");
        for (int j = 0; j < set->count - 1; j++) {
            printf("%d, ", set->data[j]);
        }
        printf("%d}", set->data[set->count - 1]);
        
        if (options & INFO_MANY_LINE) printf("\n");

        if (options & INFO_TERTIAN) {
            
            int temp[12];
            for (int j = 0; j < set->count; j++) {
                int k = j * 2 % set->count;
                temp[j] = set->data[k]; 
            }
 
            printf("    Tertian: ");
            if (set->count % 2 == 0)  printf(" (incomplete cycle) ");
            printf("{");
            for (int j = 0; j < set->count - 1; j++)  printf("%d, ", temp[j]);
            printf("%d}", temp[set->count - 1]);
            if (options & INFO_MANY_LINE) printf("\n");
        }
 
        if (options & INFO_POLARITY) {
            printf("    Polarity Value: %d ", set->polarity_value);
            if (options & INFO_MANY_LINE) printf("\n");
        }

        if (options & INFO_IV_ORDERED) {
            printf("    Ordered   Interval Vector: ");
            for (int j = 0; j < 11; j++) {
                printf("%d ", set->interval_vector_ordered[j]);
            }
            if (options & INFO_MANY_LINE) printf("\n");
        }
        
        if (options & INFO_IV_UNORDERED) {
            printf("    Unordered Interval Vector: ");
            for (int j = 0; j < 6; j++) {
                printf("%d ", set->interval_vector_unordered[j]);
            }
            if (options & INFO_MANY_LINE) printf("\n");
        }
       
        printf("\n");
    }

    if (options & INFO_COUNT) printf("Total Count: %llu\n\n", sets.count);
}


// set's length must be under or equal to 12 and in one octave, weighting's length must be 12 or more
void print_PV_from_weighting(Set set, int* weighting) {
    
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

void print_OIV(Set set) {

    int iv[11] = {0};
    for (u64 i = 0; i < set.count; i++) {
        for (u64 j = i + 1; j < set.count; j++) {
            int index = set.data[j] - set.data[i];
           
            if (index <= 0 || index > 11) {
                error("You pass a wrong set to print_OIV(), it must be monotonic and is in one octave.\n");
            }
            
            iv[index - 1] += 1;
        }
    }

    printf("Set {");
    for (int j = 0; j < set.count - 1; j++) {
        printf("%d, ", set.data[j]);
    }
    printf("%d} ", set.data[set.count - 1]);

    printf("Ordered Interval Vector: ");
    for (int i = 0; i < 11; i++) printf("%d ", iv[i]);
    printf("\n");
}

void print_UIV(Set set) {

    int iv[6] = {0};
    for (u64 i = 0; i < set.count; i++) {
        for (u64 j = i + 1; j < set.count; j++) {
            int index = set.data[j] - set.data[i];
            if (index > 6) index = 12 - index;

            if (index <= 0 || index > 6) {
                error("You pass a wrong set to print_UIV(), it must be monotonic and is in one octave.\n");
            }
           
            iv[index - 1] += 1;
        }
    }

    printf("Set {");
    for (int j = 0; j < set.count - 1; j++) {
        printf("%d, ", set.data[j]);
    }
    printf("%d} ", set.data[set.count - 1]);

    printf("Unordered Interval Vector: ");
    for (int i = 0; i < 6; i++) printf("%d ", iv[i]);
    printf("\n");
}


// must have the info first, weighting's length must be 12 or more
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
        int value = sets.data[i].polarity_value;
        table[value - lower]++;
    }

    printf("\n   PV  count\n");    
    for (int i = range - 1; i >= 0; i--) {
        printf("%5d  %5d\n", i + lower, table[i]);
    }

    free(table);
}

// must have the info first
void print_OIV_count_table(Array(SetInfo) sets) {
    
    sort(sets, compare_OIV_descend);
    
    const int iv_count = 11;
    
    u64 count       = 1;
    u64 total_count = 1;

    for (u64 i = 1; i < sets.count; i++) {

        int* prev = sets.data[i - 1].interval_vector_ordered;
        int* now  = sets.data[i    ].interval_vector_ordered;
        
        for (int i = 0; i < iv_count; i++) {
            if (now[i] != prev[i]) goto not_same;
        }

        count++;
        continue;

        not_same:
        for (int i = 0; i < iv_count; i++) printf("%d ", prev[i]);
        printf("Count: %llu\n", count);
        count = 1;
        total_count++;
    }
    
    int* prev = sets.data[sets.count - 1].interval_vector_ordered;
    for (int i = 0; i < iv_count; i++) printf("%d ", prev[i]);
    printf("Count: %llu\n", count);
    
    printf("Total Different OIV: %llu\n\n", total_count);
}

// must have the info first
void print_UIV_count_table(Array(SetInfo) sets) {
    
    sort(sets, compare_UIV_descend);

    const int iv_count = 6;
    
    u64 count       = 1;
    u64 total_count = 1;

    for (u64 i = 1; i < sets.count; i++) {

        int* prev = sets.data[i - 1].interval_vector_unordered;
        int* now  = sets.data[i    ].interval_vector_unordered;
        
        for (int i = 0; i < iv_count; i++) {
            if (now[i] != prev[i]) goto not_same;
        }
       
        count++;
        continue;

        not_same:
        for (int i = 0; i < iv_count; i++) printf("%d ", prev[i]);
        printf("Count: %llu\n", count);
        count = 1;
        total_count++;
    }
 
    int* prev = sets.data[sets.count - 1].interval_vector_unordered;
    for (int i = 0; i < iv_count; i++) printf("%d ", prev[i]);
    printf("Count: %llu\n", count);
   
    printf("Total Different UIV: %llu\n\n", total_count);
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
    count += builder_append(builder, array_string(on_off));
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
        count += builder_append(builder, array_string(note));
    }
    return count;
}

u64 append_set_chord(StringBuilder* builder, Set set) {

    if (!set.count) return 0; // maybe add rest?
    
    u64 count = 0;
    for (int i = 0; i < set.count; i++) {
        u8 on[] = {0x00,    0x90, 60 + set.data[i], 0x66};
        count += builder_append(builder, array_string(on));
    }

    u8 dt[] = {0x83, 0x60,   0x80, 60 + set.data[0], 0x66};
    count += builder_append(builder, array_string(dt));

    for (int i = 1; i < set.count; i++) {
        u8 off[] = {0x00,    0x80, 60 + set.data[i], 0x66};
        count += builder_append(builder, array_string(off));
    }

    return count;
}



void save_midi_for_sets(Array(SetInfo) sets, u64 (*append)(StringBuilder*, Array(SetInfo)), char* name) {

    StringBuilder builder = builder_init();
    
    /* ---- Header ---- */
    u64 stop_at = 0; // for MTrk chunk's length
    {
        stop_at += builder_append(&builder, string("MThd"));
        u8 info[] = {
            0x00, 0x00, 0x00, 0x06,
            0x00, 0x01,
            0x00, 0x01,
            0x01, 0xe0, // 480
        };
        stop_at += builder_append(&builder, array_string(info));
    }
 
    /* ---- Chunk ---- */
    {
        stop_at += builder_append(&builder, string("MTrk"));

        u8 length[4] = {0}; // stop at
        u8 info[]    = {0x00, 0xff, 0x03, 0x00};
        u8 end[]     = {0x00, 0xff, 0x2f, 0x00};
       
        u64 count = 0;
        
        builder_append(&builder, array_string(length));

        count += builder_append(&builder, array_string(info));
        count += append(&builder, sets);
        count += builder_append(&builder, array_string(end));
        
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
        SetInfo* set = &sets.data[i];
        count += append_set_arp(builder, (Set) {set->data, set->count});
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
            while (temp[j + 1] <= temp[j]) temp[j + 1] += 12;
        }
        
        count += append_set_chord(builder, (Set) {temp, set->count});
    }

    return count;
}



/* ==== Main ==== */

int main() {


    /* ---- setup runtime ---- */
    {
        u64 size = 1024 * 256;
        runtime.temp_buffer.data = calloc(size, sizeof(u8));
        runtime.temp_buffer.size = size;
        runtime.alloc = malloc;
    }
    
    Array(SetInfo) all_sets = get_all_set_info();
    save_midi_for_sets(all_sets, for_sets_append_arp, "all_sets.mid");
    

    /* ---- Interval Vector ---- */
    {
        fill_interval_vectors(all_sets);

        /*/
        for (u64 i = 0; i < all_sets.count; i++) {
            translate_OIV_index_order_to_IC7_order(all_sets.data[i].interval_vector_ordered);
        }
        /*/
        
        
        /*/
        Array(SetInfo) sets = get_sparse_sets(get_sets_by_size(all_sets, 7));
        sort(sets, compare_UIV_OIV_descend);
        print_set_info(sets, INFO_IV_ORDERED | INFO_IV_UNORDERED | INFO_MANY_LINE | INFO_COUNT);
        print_set_info(get_pure_tertian_sets(all_sets), INFO_IV_ORDERED | INFO_IV_UNORDERED | INFO_MANY_LINE | INFO_COUNT);
        /*/
       
        /*/
        Array(SetInfo) sets = get_pure_tertian_sets(all_sets);
        sort(sets, compare_UIV_OIV_descend);
        print_set_info(sets, INFO_IV_ORDERED | INFO_IV_UNORDERED | INFO_MANY_LINE);
        print_UIV_count_table(sets);
        print_OIV_count_table(sets);
        /*/

        /*/
        sort(all_sets, compare_UIV_OIV_descend);
        print_set_info(all_sets, INFO_IV_ORDERED | INFO_IV_UNORDERED | INFO_MANY_LINE);
        print_UIV_count_table(all_sets);
        print_OIV_count_table(all_sets);
        /*/
        
        /*        
        // example of printing IVs of a set
        int set[] = {0, 2, 4, 6, 7, 9, 11};
        print_OIV(make_set(set));        
        print_UIV(make_set(set));        


        // example of getting sets by UIV and save them
        int uiv[6] = {3, 4, 4, 5, 3, 2};
        Array(SetInfo) sets = get_sets_by_UIV(all_sets, uiv);
        sort(sets, compare_OIV_descend);
        print_set_info(sets, INFO_IV_ORDERED | INFO_IV_UNORDERED | INFO_MANY_LINE);
        save_midi_for_sets(sets, for_sets_append_arp, "uiv_test.mid");
        */        
    }

    /* ---- Sparse Set and PTS ---- */
    {

        Array(SetInfo) sps = get_sparse_sets(all_sets);
        print_set_info(sps, INFO_IV_ORDERED | INFO_IV_UNORDERED | INFO_MANY_LINE | INFO_COUNT);
        save_midi_for_sets(sps, for_sets_append_arp, "sps.mid");

        /*
        // example of getting all the pure tertian sets and save them
        Array(SetInfo) pts = get_pure_tertian_sets(all_sets);
        printf("\nPure Tertian Sets\n");
        print_set_info(pts, INFO_TERTIAN);
        
        save_midi_for_sets(pts, for_sets_append_arp, "pts.mid");
        save_midi_for_sets(pts, for_sets_append_tertian_form_chord, "pts_tertian.mid");
        */
    }
 
    /* ---- Polarity Value ---- */
    {
        /*
        int weighting[12] = {0, 0, 5, 4, 3, 2, 1, -5, -4, -3, -2, -1};
        //int weighting[12] = {0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1};
        translate_weighting_IC7_order_to_index_order(weighting);
        fill_polarity_values(all_sets, weighting);
        
        Array(SetInfo) sets = get_sets_by_PV(get_sets_by_size(all_sets, 7), 0);
        print_set_info(sets, 0);
        
        sort(all_sets, compare_count_ascend_then_value_descend);
        //print_set_info(all_sets, INFO_POLARITY);
        
        printf("\nPolarity Value Count Table\n");
        print_PV_count_table(all_sets, weighting); // todo: potential bug here for using other weighting
        

        // example of printing one set
        int set[] = {0, 2, 4, 6, 9};
        print_PV_from_weighting(make_set(set), weighting);
        */
    }
   
    //print_set_info(all_sets, INFO_ALL);
}

