#include <stdio.h>
#include <stdlib.h>


typedef unsigned int           u32;
typedef unsigned long long int u64;

typedef struct {
    int* data;
    u64  count;
} Set;

#define length_of(array) (sizeof(array) / sizeof(array[0]))

// for now, use 12 for all sets, and ignore 0 in non-0 elements
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

int main() {


    int sets[][12] = {
        {0, 2, 4, 6, 7, 9, 11},     
        {0, 2, 4, 5, 7, 9, 11},  
        {0, 2, 4, 5, 7, 9, 10},  
        {0, 2, 3, 5, 7, 9, 10},  
        {0, 2, 3, 5, 7, 8, 10},  
        {0, 1, 3, 5, 7, 8, 10},  
        {0, 1, 3, 5, 6, 8, 10},   
    };
    
    //int weighting[12] = {0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1};
    int weighting[12] = {0, 0, 5, 4, 3, 2, 1, -5, -4, -3, -2, -1};

    translate_IC7_order_to_index_order(weighting);

    for (int i = 0; i < length_of(sets); i++) {
        Set set = {sets[i], length_of(sets[i])};
        print_polarity_value_from_weighting(set, weighting);
    }

}


