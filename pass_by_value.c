#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {float    x,  y,  z;} Vector3;
typedef struct {Vector3 v0, v1, v2;} Matrix3;

typedef float vec3[3];
typedef float mat3[3][3];

Vector3 v3_add(Vector3 a, Vector3 b) {
    return (Vector3) {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
    };
}

Matrix3 m3_fake_add(Matrix3 a, Matrix3 b) {
    return (Matrix3) {
        {a.v0.x + b.v0.x, a.v0.y + b.v0.y, a.v0.z + b.v0.z},
        {a.v1.x + b.v1.x, a.v1.y + b.v1.y, a.v1.z + b.v1.z},
        {a.v2.x + b.v2.x, a.v2.y + b.v2.y, a.v2.z + b.v2.z}
    };
}

void vec3_add(vec3 out, vec3 a, vec3 b) {
    out[0] = a[0] + b[0];    
    out[1] = a[1] + b[1];    
    out[2] = a[2] + b[2];    
}

static inline void vec3_copy(vec3 a, vec3 b) {
    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
}

void mat3_fake_add(mat3 out, mat3 a, mat3 b) {
    
    out[0][0] = a[0][0] + b[0][0];
    out[0][1] = a[0][1] + b[0][1];
    out[0][2] = a[0][2] + b[0][2];

    out[1][0] = a[1][0] + b[1][0];
    out[1][1] = a[1][1] + b[1][1];
    out[1][2] = a[1][2] + b[1][2];
    
    out[2][0] = a[2][0] + b[2][0];
    out[2][1] = a[2][1] + b[2][1];
    out[2][2] = a[2][2] + b[2][2];
}

static inline void mat3_copy(mat3 a, mat3 b) {
    a[0][0] = b[0][0];
    a[0][1] = b[0][1];
    a[0][2] = b[0][2];

    a[1][0] = b[1][0];
    a[1][1] = b[1][1];
    a[1][2] = b[1][2];
    
    a[2][0] = b[2][0];
    a[2][1] = b[2][1];
    a[2][2] = b[2][2];
}

int main() {
    
    struct timespec start, end;

    Vector3 V  = {0};
    Vector3 VA = {1, 2, 3};
    Vector3 VB = {4, 5, 6};

    Matrix3 M = {0};
    Matrix3 A = {
        {1, 2, 3},
        {1, 2, 3},
        {1, 2, 3},
    };
    Matrix3 B = {
        {4, 5, 6},
        {4, 5, 6},
        {4, 5, 6},
    };
    
    vec3 v  = {0};
    vec3 va = {1, 2, 3};
    vec3 vb = {4, 5, 6};
    
    mat3 m = {0};
    mat3 a = {
        {1, 2, 3},
        {1, 2, 3},
        {1, 2, 3},
    };
    mat3 b = {
        {4, 5, 6},
        {4, 5, 6},
        {4, 5, 6},
    };

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 1024 * 1024 * 1024; i++) {
        
        VA = v3_add(VA, VB);
        VA = v3_add(VA, VB);
        // vec3_add(v, va, vb);
        // vec3_copy(va, v);
        // memcpy(va, v, sizeof(v));
        // vec3_add(v, va, vb);
        // memcpy(va, v, sizeof(v));
        // vec3_copy(va, v);

        A = m3_fake_add(A, B);
        A = m3_fake_add(A, B);
        // mat3_fake_add(m, a, b);
        // mat3_copy(a, m);
        // memcpy(a, m, sizeof(m));
        // mat3_fake_add(m, a, b);
        // mat3_copy(a, m);
        // memcpy(a, m, sizeof(m));
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    printf("%fs\n\n", (end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec));
    printf("%12f %12f %12f\n", A.v0.x , A.v1.y , A.v2.z );
    printf("%12f %12f %12f\n", a[0][0], a[1][1], a[2][2]);

    printf("%12f %12f %12f\n", VA.x , VA.y , VA.z );
    printf("%12f %12f %12f\n", va[0], va[1], va[2]);

    return 0;
}

/* 

result

-O0 no difference
-O1 array win
-O2 no difference
-O2 array with our copy(), struct win by large amount
-O3 no difference


*/