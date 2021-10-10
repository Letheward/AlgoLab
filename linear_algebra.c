/* 

==== Note ====

note: all matrix functions use column major order

in math:
0  4  8  12 |
1  5  9  13 | * 4
2  6  10 14 |
3  7  11 15 v

in memory (and code)
0  1  2  3
4  5  6  7
8  9  10 11
12 13 14 15
----------> * 4

our coordinate system, same every where, even for NDC

|          z  y
|          | /
|          |/
|    ------------- x
|         /|
|        / |


*/


/* ==== Data Structures ==== */

typedef struct {float x, y, z   ;} Vector3;
typedef struct {float x, y, z, w;} Vector4;

typedef struct {Vector4 v0, v1, v2, v3;} Matrix4;

typedef struct {float    yz, zx, xy;} BiVector3;
typedef struct {float s, yz, zx, xy;} Rotor3;



/* ==== Functions ==== */

Vector3 v3_add(Vector3 a, Vector3 b) {
    return (Vector3) {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}

Vector3 v3_scale(Vector3 v, const float s) {
    return (Vector3) {
        v.x * s,
        v.y * s,
        v.z * s,
    };
}

float v3_dot(const Vector3 a, const Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 v3_cross(const Vector3 a, const Vector3 b) {
    return (Vector3) {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

BiVector3 v3_wedge(const Vector3 a, const Vector3 b) {
    return (BiVector3) {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

float v3_length(const Vector3 v) {
    return sqrtf(v3_dot(v, v));
}

Vector3 v3_normalize(const Vector3 v) {
    float k = 1 / v3_length(v);
    return v3_scale(v, k);
}

Matrix4 m4_identity() {
    return (Matrix4) {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };
}

// oh god ... this looks really unsafe, but result is right
// actually the assembly op count is 
// slightly smaller than array version 
// but alloc more space in the stack 
Matrix4 m4_mul(Matrix4 M, Matrix4 X) {
    float temp[4][4] = {0};
    float* m = (float*) &M;
    float* x = (float*) &X;
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 4; k++) {
            for (int j = 0; j < 4; j++) {
                temp[i][j] += m[k * 4 + j] * x[i * 4 + k];
            }
        }
    }
    return *((Matrix4*) temp);
}

Matrix4 m4_rotate_x(Matrix4 M, float rad) {
    float c = cos(rad);
    float s = sin(rad);
    return m4_mul(
        M, 
        (Matrix4) {
            {  1,  0,  0,  0},
            {  0,  c,  s,  0},
            {  0, -s,  c,  0},
            {  0,  0,  0,  1}
        }
    );
}

Matrix4 m4_rotate_y(Matrix4 M, float rad) {
    float c = cos(rad);
    float s = sin(rad);
    return m4_mul(
        M, 
        (Matrix4) {
            {  c,  0, -s,  0},
            {  0,  1,  0,  0},
            {  s,  0,  c,  0},
            {  0,  0,  0,  1}
        }
    );
}

Matrix4 m4_rotate_z(Matrix4 M, float rad) {
    float c = cos(rad);
    float s = sin(rad);
    return m4_mul(
        M, 
        (Matrix4) {
            {  c,  s,  0,  0},
            { -s,  c,  0,  0},
            {  0,  0,  1,  0},
            {  0,  0,  0,  1}
        }
    );
}

void m4_move(Matrix4* M, float x, float y, float z) {
    M->v3.x += x;
    M->v3.y += y;
    M->v3.z += z;
}

Matrix4 m4_perspective(float FOV, float aspect, float n, float f) {
    const float a = 1.0 / tan(FOV / 2.0);
    return (Matrix4) {
        { a / aspect,                      0,  0,  0},
        {          0,      (f + n) / (f - n),  0,  1},
        {          0,                      0,  a,  0},
        {          0, -2 * (f * n) / (f - n),  0,  0}
    };
}

Matrix4 m4_orthogonal(float l, float r, float b, float t, float n, float f) {
    return (Matrix4) {
        {       2 / (r - l),                   0,                   0,  0},
        {                 0,         2 / (f - n),                   0,  0},
        {                 0,                   0,         2 / (t - b),  0},
        {-(r + l) / (r - l),  -(f + n) / (f - n),  -(t + b) / (t - b),  1}
    };
}

void print_matrix(Matrix4 M) {
    printf(
        "v0 %12f %12f %12f %12f\n"
        "v1 %12f %12f %12f %12f\n"
        "v2 %12f %12f %12f %12f\n"
        "v3 %12f %12f %12f %12f\n\n", 
        M.v0.x, M.v0.y, M.v0.z, M.v0.w,
        M.v1.x, M.v1.y, M.v1.z, M.v1.w,
        M.v2.x, M.v2.y, M.v2.z, M.v2.w,
        M.v3.x, M.v3.y, M.v3.z, M.v3.w
    );
}

