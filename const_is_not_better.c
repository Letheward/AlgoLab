#include <stdio.h>


typedef struct {
    float x;
    float y;
    float z;
} Vector3;


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




/* 
    in every function result code is exactly the same
    but it's good to use with "in" pointers to speak intention
    and prevent writing to "in"
*/



int main() {

    return 0;
}