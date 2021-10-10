#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

typedef struct {
    char*  name;
    double probability;
    int    hit;
} Data;

Data data[] = {
    {"Apple",    0.03 , 0},
    {"Ball",     0.01 , 0},
    {"Car",      0.02 , 0},
    {"Desk",     0.005, 0},
    {"Elevator", 0.015, 0},
    {"Forge",    0.02 , 0},
    {"God",      0.02 , 0},
    {"Horse",    0.22 , 0},
    {"It",       0.02 , 0},
    {"Just",     0.02 , 0},
    {"",         0.00 , 0}
};

double* probability_array; 

void hit_by_chance(double p, int count) {
    int index = 0;
    while (index < count - 1 && p >= probability_array[index]) index++;
    data[index].hit++;
}

void generate_array(int count) { 
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += data[i].probability;
        probability_array[i] = sum;
    }
}


int main() {

    // get random seed
    struct timespec time_info;
    clock_gettime(CLOCK_MONOTONIC, &time_info);
    int seed = time_info.tv_nsec;
    srand(seed);
    printf("seed: %d\n\n", seed);

    // get helper array
    int count = sizeof(data) / sizeof(data[0]);
    probability_array = malloc(count * sizeof(Data));
    generate_array(count);

    for (int i = 0; i < 10000000; i++) {
        double p = (double) rand() / 32768.0;
        hit_by_chance(p, count);
    }
    for (int i = 0; i < count; i++) {
        printf("%10s: %10d\n", data[i].name, data[i].hit);
    }

    return 0;
}
