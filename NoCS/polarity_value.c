#include <stdio.h>
#include <stdlib.h>




/* ==== Macros and Types ==== */

typedef unsigned char          u8;
typedef unsigned int           u32;
typedef unsigned long long int u64;

#define length_of(array) (sizeof(array) / sizeof(array[0]))

#define DynamicArray(Type) DynamicArray_ ## Type
#define Define_DynamicArray(Type)           \
typedef struct {                            \
    Type* data;                             \
    u64   count;                            \
    u64   allocated;                        \
} DynamicArray(Type)                        \

typedef struct {
    int* data;
    u64  count;
} Set;

typedef struct {
    u8  data[12];
    int value;
} SetInfo;

Define_DynamicArray(SetInfo);



/* ==== Helpers ==== */

void array_add(DynamicArray(SetInfo)* a, SetInfo item) {                
                                                                         
    u64 wanted = a->count + 1;                                      
    if (wanted > a->allocated) {                                         
        a->data = realloc(a->data, wanted * 2 * sizeof(SetInfo));
        a->allocated *= 2;
    }                                                                      
                                                                           
    a->data[a->count] = item;                                    
    a->count = wanted;                                                
}                                                                          

void generate_code_by_level(int n) {
    
    printf("for (int i1 =      1; i1 < 12; i1++)\n");
    for (int i = 2; i < n; i++) {
        printf(
            "for (int i%d = i%d + 1; i%d < 12; i%d++)\n",
            i, i - 1, i, i
        );
    }
    
    printf("{\n    int data[] = {0, ");
    for (int i = 1; i < n; i++) {
        printf("i%d, ", i);
    }
    printf("};\n");
    printf(
        "    Set set = {data, length_of(data)};\n"
        "    SetInfo si = {0};\n"
        "    for (int i = 0; i < %d; i++) si.data[i] = (u8) data[i];\n"
        "    si.value = get_polarity_value_from_weighting(set, weighting);\n"
        "    array_add(&info, si);\n"
        "}\n\n",
        n
    );
}

void generate_code() {
    for (int i = 2; i < 13; i++) {
        generate_code_by_level(i);
    }
}






/* ==== The Real Thing ==== */


void print_sets(DynamicArray(SetInfo) info) {
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


// use 12 for all sets, and ignore 0 for non-0 element indices
void print_polarity_value_from_weighting(Set set, int* weighting) {
    
    printf("Set    ");
    
    int value = 0;
    for (int j = 0; j < set.count; j++) {
        int e = set.data[j];
        if (j && !e) continue;
        printf("%d ", e);
        value += weighting[e];
    }

    printf("    Value %d \n", value);
}

int get_polarity_value_from_weighting(Set set, int* weighting) {
    
    int value = 0;
    for (int j = 0; j < set.count; j++) {
        int e = set.data[j];
        if (j && !e) continue;
        value += weighting[e];
    }

    return value;
}



/*
0 6  7 2 9 4 11   5 10 3 8 1
to
0 1  2 3 4 5 6    7 8 9 10 11
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








DynamicArray(SetInfo) get_all_set_info(int* weighting) {

    DynamicArray(SetInfo) info = {
        .data      = calloc(32, sizeof(SetInfo)),
        .count     = 0,
        .allocated = 32,
    };

    for (int i1 =      1; i1 < 12; i1++)
    {
        int data[] = {0, i1, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 2; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    for (int i1 =      1; i1 < 12; i1++)
    for (int i2 = i1 + 1; i2 < 12; i2++)
    {
        int data[] = {0, i1, i2, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 3; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    for (int i1 =      1; i1 < 12; i1++)
    for (int i2 = i1 + 1; i2 < 12; i2++)
    for (int i3 = i2 + 1; i3 < 12; i3++)
    {
        int data[] = {0, i1, i2, i3, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 4; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    for (int i1 =      1; i1 < 12; i1++)
    for (int i2 = i1 + 1; i2 < 12; i2++)
    for (int i3 = i2 + 1; i3 < 12; i3++)
    for (int i4 = i3 + 1; i4 < 12; i4++)
    {
        int data[] = {0, i1, i2, i3, i4, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 5; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    for (int i1 =      1; i1 < 12; i1++)
    for (int i2 = i1 + 1; i2 < 12; i2++)
    for (int i3 = i2 + 1; i3 < 12; i3++)
    for (int i4 = i3 + 1; i4 < 12; i4++)
    for (int i5 = i4 + 1; i5 < 12; i5++)
    {
        int data[] = {0, i1, i2, i3, i4, i5, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 6; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    for (int i1 =      1; i1 < 12; i1++)
    for (int i2 = i1 + 1; i2 < 12; i2++)
    for (int i3 = i2 + 1; i3 < 12; i3++)
    for (int i4 = i3 + 1; i4 < 12; i4++)
    for (int i5 = i4 + 1; i5 < 12; i5++)
    for (int i6 = i5 + 1; i6 < 12; i6++)
    {
        int data[] = {0, i1, i2, i3, i4, i5, i6, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 7; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    for (int i1 =      1; i1 < 12; i1++)
    for (int i2 = i1 + 1; i2 < 12; i2++)
    for (int i3 = i2 + 1; i3 < 12; i3++)
    for (int i4 = i3 + 1; i4 < 12; i4++)
    for (int i5 = i4 + 1; i5 < 12; i5++)
    for (int i6 = i5 + 1; i6 < 12; i6++)
    for (int i7 = i6 + 1; i7 < 12; i7++)
    {
        int data[] = {0, i1, i2, i3, i4, i5, i6, i7, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 8; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    for (int i1 =      1; i1 < 12; i1++)
    for (int i2 = i1 + 1; i2 < 12; i2++)
    for (int i3 = i2 + 1; i3 < 12; i3++)
    for (int i4 = i3 + 1; i4 < 12; i4++)
    for (int i5 = i4 + 1; i5 < 12; i5++)
    for (int i6 = i5 + 1; i6 < 12; i6++)
    for (int i7 = i6 + 1; i7 < 12; i7++)
    for (int i8 = i7 + 1; i8 < 12; i8++)
    {
        int data[] = {0, i1, i2, i3, i4, i5, i6, i7, i8, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 9; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    for (int i1 =      1; i1 < 12; i1++)
    for (int i2 = i1 + 1; i2 < 12; i2++)
    for (int i3 = i2 + 1; i3 < 12; i3++)
    for (int i4 = i3 + 1; i4 < 12; i4++)
    for (int i5 = i4 + 1; i5 < 12; i5++)
    for (int i6 = i5 + 1; i6 < 12; i6++)
    for (int i7 = i6 + 1; i7 < 12; i7++)
    for (int i8 = i7 + 1; i8 < 12; i8++)
    for (int i9 = i8 + 1; i9 < 12; i9++)
    {
        int data[] = {0, i1, i2, i3, i4, i5, i6, i7, i8, i9, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 10; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    for (int i1 =      1; i1 < 12; i1++)
    for (int i2 = i1 + 1; i2 < 12; i2++)
    for (int i3 = i2 + 1; i3 < 12; i3++)
    for (int i4 = i3 + 1; i4 < 12; i4++)
    for (int i5 = i4 + 1; i5 < 12; i5++)
    for (int i6 = i5 + 1; i6 < 12; i6++)
    for (int i7 = i6 + 1; i7 < 12; i7++)
    for (int i8 = i7 + 1; i8 < 12; i8++)
    for (int i9 = i8 + 1; i9 < 12; i9++)
    for (int i10 = i9 + 1; i10 < 12; i10++)
    {
        int data[] = {0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 11; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    for (int i1 =      1; i1 < 12; i1++)
    for (int i2 = i1 + 1; i2 < 12; i2++)
    for (int i3 = i2 + 1; i3 < 12; i3++)
    for (int i4 = i3 + 1; i4 < 12; i4++)
    for (int i5 = i4 + 1; i5 < 12; i5++)
    for (int i6 = i5 + 1; i6 < 12; i6++)
    for (int i7 = i6 + 1; i7 < 12; i7++)
    for (int i8 = i7 + 1; i8 < 12; i8++)
    for (int i9 = i8 + 1; i9 < 12; i9++)
    for (int i10 = i9 + 1; i10 < 12; i10++)
    for (int i11 = i10 + 1; i11 < 12; i11++)
    {
        int data[] = {0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, };
        Set set = {data, length_of(data)};
        SetInfo si = {0};
        for (int i = 0; i < 12; i++) si.data[i] = (u8) data[i];
        si.value = get_polarity_value_from_weighting(set, weighting);
        array_add(&info, si);
    }

    return info;
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

//    generate_code();

    int weighting[12] = {0, 0, 5, 4, 3, 2, 1, -5, -4, -3, -2, -1};
    translate_IC7_order_to_index_order(weighting);

    DynamicArray(SetInfo) info = get_all_set_info(weighting);
    qsort(info.data, info.count, sizeof(SetInfo), compare_value_descend);

    print_sets(info);

}


