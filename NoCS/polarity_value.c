#include <stdio.h>
#include <stdlib.h>




/* ==== Macros and Types ==== */

typedef unsigned char          u8;
typedef unsigned int           u32;
typedef unsigned long long int u64;

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
    int value;
} SetInfo;

Define_Array(SetInfo);




/* ==== Main ==== */

void print_set_PV_from_weighting(Set set, int* weighting) {
    
    printf("Set {");
    
    int value = 0;
    for (int j = 0; j < set.count; j++) {
        int e = set.data[j];
        if (j && !e) continue;
        printf("%d, ", e);
        value += weighting[e];
    }

    printf("}  Value %d \n", value);
}

void print_set_info(Array(SetInfo) info) {
    for (int i = 0; i < info.count; i++) {
        printf("Set {");
        for (int j = 0; j < 12; j++) {
            u8 v = info.data[i].data[j];
            if (j > 0 && v == 0) continue;
            printf("%d, ", v);
        }
        printf("}  Value: %d\n", info.data[i].value);
    }
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



Array(SetInfo) get_all_set_info(int* weighting) {
    
    Array(SetInfo) info = {
        .data  = calloc(2048, sizeof(SetInfo)),
        .count = 2048,
    };

    u32 binary = 0;
    while (binary < (1 << 11)) {

        SetInfo* p = &info.data[binary];

        // unpack bits and get PVs
        int c = 0;
        for (int i = 0; i < 11; i++) {
            if (binary & (1 << i)) {
                p->data[c + 1] = i + 1;
                p->value += weighting[i + 1];
                c++;
            }
        }

        binary++;
    }
    
    return info;
}



/*
    0 6   7 2 9 4 11   5 10 3 8  1
    to
    0 1   2 3 4 5 6    7 8  9 10 11
*/
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


int main() {

    int weighting[12] = {0, 0, 5, 4, 3, 2, 1, -5, -4, -3, -2, -1};
    //int weighting[12] = {0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1};
    //int weighting[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    //int weighting[12] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    translate_IC7_order_to_index_order(weighting);

    Array(SetInfo) info = get_all_set_info(weighting);
    qsort(info.data, info.count, sizeof(SetInfo), compare_value_descend);
    
    print_set_info(info);
    print_PV_count_table(info, weighting); // todo: potential bug here for using other weighting

    /* 
    
    // example of printing one set
    int set[] = {0, 2, 4, 6, 9};

    print_set_PV_from_weighting(make_set(set), weighting);
    
    */

}


