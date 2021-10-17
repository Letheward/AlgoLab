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

right now we convert our coordinate to gl_Position in the shader,
if there is a speed concern, we can swap every M[..][1] with M[..][2]
in projection matrix functions and use gl_Position directly.

*/


/* ==== Data Structures ==== */

// struct than array, now we can write Vector3 B = A; and return value from a function
// through experiment, sometime this is faster, at least as fast as array version in -O3
typedef struct {float x, y, z   ;} Vector3;
typedef struct {float x, y, z, w;} Vector4;

typedef struct {Vector3 v0, v1, v2    ;} Matrix3;
typedef struct {Vector4 v0, v1, v2, v3;} Matrix4;

typedef struct {float    yz, zx, xy;} BiVector3;
typedef struct {float s, yz, zx, xy;} Rotor3D;





/* ==== Vector and Matrix ==== */

Vector3 v3_reverse(Vector3 v) {
    return (Vector3) {-v.x, -v.y, -v.z};
}

Vector3 v3_add(Vector3 a, Vector3 b) {
    return (Vector3) {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vector3 v3_sub(Vector3 a, Vector3 b) {
    return (Vector3) {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vector3 v3_scale(Vector3 v, float s) {
    return (Vector3) {v.x * s, v.y * s, v.z * s};
}

float v3_dot(Vector3 a, Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float v3_length(Vector3 v) {
    return sqrtf(v3_dot(v, v));
}

Vector3 v3_normalize(Vector3 v) {
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

Matrix4 m4_translate(Vector3 v) {
    return (Matrix4) {
        {  1,   0,   0,   0},
        {  0,   1,   0,   0},
        {  0,   0,   1,   0},
        {v.x, v.y, v.z,   1}
    };
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




/* ==== Geometric Algebra ==== */

BiVector3 v3_wedge(Vector3 a, Vector3 b) {
    return (BiVector3) {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

Rotor3D b3_to_r3d(float s, BiVector3 b) {
    return (Rotor3D) {s, b.yz, b.zx, b.xy};
}

Rotor3D r3d_normalize(Rotor3D r) {
    float l = sqrtf(r.s * r.s + r.yz * r.yz + r.zx * r.zx + r.xy * r.xy);
    // divide by zero?
    return (Rotor3D) {r.s / l, r.yz / l, r.zx / l, r.xy / l};
}

Rotor3D r3d_reverse(Rotor3D r) {
    return (Rotor3D) {r.s, -r.yz, -r.zx, -r.xy};
}

// normalize before using this
Rotor3D r3d_from_v3_half(Vector3 a, Vector3 b) {
    BiVector3 bv = v3_wedge(a, b);
    return (Rotor3D) {1 + v3_dot(a, b) /* half angle trick */, bv.yz, bv.zx, bv.xy};
}

// todo: validate
// normalize before using this
Rotor3D r3d_from_plane_half(BiVector3 plane, float rad) {
    float s = sin(rad / 2);
    float c = cos(rad / 2);
    return (Rotor3D) {c, s * plane.yz, s * plane.zx, s * plane.xy};
}

Rotor3D r3d_mul(Rotor3D a, Rotor3D b) {
    return (Rotor3D) {
        a.s * b.s  - a.yz * b.yz - a.zx * b.zx - a.xy * b.xy,
        a.s * b.yz + a.yz * b.s  - a.zx * b.xy + a.xy * b.zx,
        a.s * b.zx + a.zx * b.s  - a.xy * b.yz + a.yz * b.xy,
        a.s * b.xy + a.xy * b.s  - a.yz * b.zx + a.zx * b.yz
    };
}

// this is the exactly same as quaternion
// R* V R
Vector3 v3_rotate(Vector3 v, Rotor3D r) {

    // temp result, a vector and a trivector
    float _x   =  v.x * r.s  - v.y * r.xy + v.z * r.zx;
    float _y   =  v.y * r.s  - v.z * r.yz + v.x * r.xy;
    float _z   =  v.z * r.s  - v.x * r.zx + v.y * r.yz;
    float _xyz = -v.x * r.yz - v.y * r.zx - v.z * r.xy;

    // trivector in result will always be 0
    return (Vector3) {
        _x * r.s - _y * r.xy + _z * r.zx - _xyz * r.yz,
        _y * r.s - _z * r.yz + _x * r.xy - _xyz * r.zx,
        _z * r.s - _x * r.zx + _y * r.yz - _xyz * r.xy
    };
}

Matrix3 r3d_to_m3(Rotor3D r) {
    return (Matrix3) {
        v3_rotate((Vector3) {1, 0, 0}, r),
        v3_rotate((Vector3) {0, 1, 0}, r),
        v3_rotate((Vector3) {0, 0, 1}, r)
    };
}

// todo: this looks perform worse than just operate on the matrix
Matrix4 r3d_to_m4(Rotor3D r) {
    Vector3 v0 = v3_rotate((Vector3) {1, 0, 0}, r);
    Vector3 v1 = v3_rotate((Vector3) {0, 1, 0}, r);
    Vector3 v2 = v3_rotate((Vector3) {0, 0, 1}, r);
    return (Matrix4) {
        { v0.x,  v0.y,  v0.z, 0},
        { v1.x,  v1.y,  v1.z, 0},
        { v2.x,  v2.y,  v2.z, 0},
        {    0,     0,     0, 1}
    };
}



/* ==== Utility ==== */

void print_m4(Matrix4 M) {
    for (int i = 0; i < 4 * 4; i++) {
        if (i > 0 && i % 4 == 0) printf("\n");
        printf("%12f", ((float*) &M)[i]);
    }
    printf("\n\n");
}

void print_v3(Vector3 v) {
    printf("%12f %12f %12f\n", v.x, v.y, v.z);
}

void print_r3d(Rotor3D r) {
    printf("%12f %12f %12f %12f\n", r.s, r.yz, r.zx, r.xy);
}

