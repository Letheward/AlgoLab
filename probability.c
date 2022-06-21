#include <stdio.h>
#include <stdlib.h>




/* ==== Macros and Types ==== */

#define length_of(array) (sizeof(array) / sizeof(array[0]))

typedef unsigned char           u8;
typedef unsigned short int      u16;
typedef unsigned int            u32;
typedef unsigned long long int  u64;
typedef signed char             s8;
typedef signed short int        s16;
typedef signed int              s32;
typedef signed long long int    s64;
typedef float                   f32;
typedef double                  f64;


typedef struct {
    f64 probability;
    u64 hit;
} Test_Data;




/* ==== Main Algorithms ==== */

/*

    How this works:

    For probabilities of every item, we embed them into a helper array, e.g.:

    | Index       | 0        | 1   | 2       |      |
    | Sum         | 0.4      | 0.5 | 0.75    |      |
    | Probability | 0.4      | 0.1 | 0.25    |      |
                  <-------------- P = 1 ------------>
    
    In hit_by_chance(), given a evenly distributed probability in [0, 1], 
    it has 0.4 chance of land on the 0 slot, 0.1 chance on 1 slot, and so on.

    If the probability lands on the empty slot (which is the probability of not on 0, 1, 2), we don't hit.

*/

f64* get_helper_array(Test_Data* data, u64 count) { 
   
    f64* out = malloc(sizeof(f64) * count);
    f64  sum = 0.0;
    
    for (u64 i = 0; i < count; i++) {
        sum += data[i].probability;
        out[i] = sum;
    }
    
    if (sum > 1.0) { 
        printf("[Error] Total probability is larger than 1.\n"); 
        exit(1);
    }

    return out;
}

void hit_by_chance(f64 probability, Test_Data* data, f64* helper, u64 count) {
    u64 index = 0;
    while (probability > helper[index] && index < count) index++;
    if (index != count) data[index].hit++; 
}




int main() {

    Test_Data data[] = {
        {0.4 , 0},
        {0.10, 0},
        {0.25, 0},
    };

    u64  count  = length_of(data);
    f64* helper = get_helper_array(data, count);

    // test
    for (int i = 0; i < 1000000; i++) {
        f64 p = (f64) rand() / (f64) RAND_MAX;
        hit_by_chance(p, data, helper, count);
    }

    printf("Helper:\n");
    for (u64 i = 0; i < count; i++) {
        printf("%f ", helper[i]);
    }

    printf("\n\nResult:\n");
    for (u64 i = 0; i < count; i++) {
        printf("%3llu: %12llu\n", i, data[i].hit);
    }

}

