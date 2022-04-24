#include <stdio.h>
#include <stdlib.h>




/* ==== Macros and Types ==== */

typedef unsigned char          u8;
typedef unsigned short         u16;
typedef unsigned int           u32;
typedef unsigned long long int u64;
typedef signed char            s8;
typedef signed short           s16;
typedef signed int             s32;
typedef signed long long int   s64;


#define length_of(array)  (sizeof(array) / sizeof(array[0]))
#define make_set(c_array) (Set) {c_array, length_of(c_array)}

#define Array(Type) Array_ ## Type
#define Define_Array(Type) \
typedef struct {           \
    Type* data;            \
    u64   count;           \
} Array(Type)              \

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




/* ==== Main ==== */

Array(SetInfo) get_all_set_info() {
    
    Array(SetInfo) info = {
        .data  = calloc(2048, sizeof(SetInfo)),
        .count = 2048,
    };

    const u32 done = 1 << 11;
    u32 binary     = 0;

    while (binary < done) {

        SetInfo* p = &info.data[binary];

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
    
    return info;
}

void fill_all_polarity_values(Array(SetInfo) info, int* weighting) {
    for (u64 i = 0; i < info.count; i++) {
        SetInfo* p = &info.data[i];
        for (int i = 0; i < p->count; i++) {
            p->value += weighting[p->data[i]]; // we need calloc() in the get_all_set_info() for this
        }
    }
}




void print_set_info(Array(SetInfo) info) {

    for (int i = 0; i < info.count; i++) {
        
        SetInfo* set = &info.data[i];
        
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

void print_PV_count_table(Array(SetInfo) info, int* weighting) {
    
    int upper = 0;    
    int lower = 0;    

    for (int i = 0; i < 12; i++) {
        int v = weighting[i];
        if (v > 0) upper += v;
        if (v < 0) lower += v;
    }

    int  range = upper - lower + 1;
    int* table = calloc(range, sizeof(int));

    for (u64 i = 0; i < info.count; i++) {
        int value = info.data[i].value;
        table[value - lower]++;
    }

    printf("\n   PV  count\n");    
    for (int i = range - 1; i >= 0; i--) {
        printf("%5d  %5d\n", i + lower, table[i]);
    }

    free(table);
}

// todo: handle non-generated sets, cleanup and refactor
void print_all_set_tertian_form(Array(SetInfo) info, u8 is_pure) {

    for (int i = 0; i < info.count; i++) {
        
        SetInfo* set = &info.data[i];
        if (set->count % 2 == 0)  continue;
        
        int temp[12];
        for (int j = 0; j < set->count; j++) {
            int k = j * 2 % set->count;
            temp[j] = set->data[k]; 
        }
        
        if (is_pure) {
            
            for (int j = 0; j < set->count - 1; j++) {
                int diff = temp[j + 1] - temp[j];
                if (diff < 0) diff += 12;
                if (!(diff == 3 || diff == 4)) goto next;
            }

            int diff = 12 - temp[set->count - 1];
            if (!(diff == 3 || diff == 4)) goto next;
        }
        
        printf("Set {");
        for (int j = 0; j < set->count - 1; j++)  printf("%d, ", set->data[j]);
        printf("%d} ", set->data[set->count - 1]);
        
        printf("Tertian {");
        for (int j = 0; j < set->count - 1; j++)  printf("%d, ", temp[j]);
        printf("%d}\n", temp[set->count - 1]);
        
        next: continue;
    }
}




/* ---- Helpers ---- */

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
    if (av <  bv) return  1;
    if (av == bv) return  0;
    if (av >  bv) return -1;
}

int compare_count_descend(const void* a, const void* b) {
    SetInfo* sa = (SetInfo*) a;
    SetInfo* sb = (SetInfo*) b;
    int ac = sa->count;
    int bc = sb->count;
    if (ac <  bc) return  1;
    if (ac == bc) return  0;
    if (ac >  bc) return -1;
}

int compare_count_ascend_then_value_descend(const void* a, const void* b) {
    
    SetInfo* sa = (SetInfo*) a;
    SetInfo* sb = (SetInfo*) b;
    
    int ac = sa->count;
    int bc = sb->count;
    int av = sa->value;
    int bv = sb->value;
    
    if (ac >  bc) return  1;
    if (ac <  bc) return -1;
    
    // if count equals...
    if (av <  bv) return  1;
    if (av == bv) return  0;
    if (av >  bv) return -1;
}




int main() {

    Array(SetInfo) info = get_all_set_info();

    /* ---- polarity value ---- */
    {
        /*
        int weighting[12] = {0, 0, 5, 4, 3, 2, 1, -5, -4, -3, -2, -1};
        //int weighting[12] = {0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1};
        //int weighting[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        //int weighting[12] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
        translate_IC7_order_to_index_order(weighting);
        fill_all_polarity_values(info, weighting);

        qsort(info.data, info.count, sizeof(SetInfo), compare_count_ascend_then_value_descend);
        print_set_info(info);
        print_PV_count_table(info, weighting); // todo: potential bug here for using other weighting
        
        // example of printing one set
        int set[] = {0, 2, 4, 6, 9};
        print_set_PV_from_weighting(make_set(set), weighting);
        */
    }

    /* ---- Tertian Form ---- */
    print_all_set_tertian_form(info, 1);

}


