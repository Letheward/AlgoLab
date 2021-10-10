#include <stdio.h>

#define TAU 6.28
const float Tau = 6.28; // this is faster

static inline float degree_to_radian(float degree) {
    return degree * Tau / 360.0;    // 10.459s in -O0, 4.408s in -O3
    // return degree * TAU / 360.0; // 13.513s in -O0, 7.400s in -O3
}

int main() {

    float angle = 1.0;

    for (int i = 0; i < 1024; i++) {
        for (int j = 0; j < 1024; j++) {
            for (int k = 0; k < 1024; k++) {
                angle = degree_to_radian(angle);
            }
        }
    }
    
    printf("%f\n", angle); // make sure compiler don't optimize out
    
    return 0;
}