#include <stdio.h>

typedef struct {
    float x;
    float y;
    float z;
} Vector3;





/* ==== const is better ==== */

#define TAU 6.28
const float Tau = 6.28; // this is faster

static inline float degree_to_radian(float degree) {
    return degree * Tau / 360.0;    // 10.459s in -O0, 4.408s in -O3
    // return degree * TAU / 360.0; // 13.513s in -O0, 7.400s in -O3
}

void test_const() {

    float angle = 1.0;

    for (int i = 0; i < 1024; i++) {
        for (int j = 0; j < 1024; j++) {
            for (int k = 0; k < 1024; k++) {
                angle = degree_to_radian(angle);
            }
        }
    }
    
    printf("%f\n", angle); // make sure compiler don't optimize out
    
}



/* ==== const is not better ==== */


/* 
    in every function result code is exactly the same
    but it's good to use with "in" pointers to speak intention
    and prevent writing to "in"
*/


float mul_add(float a, float b, float c) {
    return a * b + c;
}

// this is hard to read
float mul_add_const(const float a, const float b, const float c) {
    return a * b + c;
}


void square(float* a, float* b) {
    *a = *b * *b;
}

void square_const(float* a, const float* b) {
    *a = *b * *b;
}

void v3_normalize(Vector3* out, Vector3* v) {
    *out = (Vector3) {v->x, v->y, v->z};
}

void v3_normalize_const(Vector3* out, const Vector3* v) {
    *out = (Vector3) {v->x, v->y, v->z};
}



int main() {

    return 0;
}
