#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


// Data Types
typedef double Vector2[2];
typedef double Vector3[3];
typedef double Vector4[4];

typedef double Matrix2[2][2];
typedef double Matrix3[3][3];
typedef double Matrix4[4][4];

typedef double BiVector3[3];  //    yz zx xy
typedef double Rotor3[4];     // s  yz zx xy

typedef double Quaternion[4]; // 1  i  j  k


/* ==== Common Functions ==== */

void mat4_identity(Matrix4 M) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            M[i][j] = (i == j ? 1 : 0);
        }
    }
}

void mat4_copy(Matrix4 M, Matrix4 N) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            M[i][j] = N[i][j];
        }
    }
}

void mat4_mul_to(Matrix4 M, Matrix4 X) {
    Matrix4 temp = {0};
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 4; k++) {
            for (int j = 0; j < 4; j++) {
                temp[i][j] += M[k][j] * X[i][k];
            }
        }
    }
    mat4_copy(M, temp);
}


/* ==== Geometric Algebra Version ==== */

void rotor3_mul(Rotor3 r, Rotor3 p, Rotor3 q) {
    r[0] = p[0] * q[0] - p[1] * q[1] - p[2] * q[2] - p[3] * q[3];
    r[1] = p[1] * q[0] + p[0] * q[1] + p[3] * q[2] - p[2] * q[3];
    r[2] = p[2] * q[0] + p[0] * q[2] - p[3] * q[1] + p[1] * q[3];
    r[3] = p[3] * q[0] + p[0] * q[3] + p[2] * q[1] - p[1] * q[2];
}

void rotor3_mul_to(Rotor3 p, Rotor3 q) {

    Rotor3 r;
    rotor3_mul(r, p, q);

    p[0] = r[0];
    p[1] = r[1];
    p[2] = r[2];
    p[3] = r[3];
}


/* ==== Quaternion Version ==== */




int main() {

    Rotor3 a = {1, 0, 0, 0};
    Rotor3 b = {0, 0, 0, 1};

    printf("%12f %12f %12f %12f\n", a[0], a[1], a[2], a[3]);
    rotor3_mul_to(a, b);
    printf("%12f %12f %12f %12f\n", a[0], a[1], a[2], a[3]);
    
    return 0;
}