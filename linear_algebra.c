/*

==== Note ====

note: all matrix functions use column major order

in math: (by basis vector)
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

typedef struct {f32 x, y      ;} Vector2;
typedef struct {f32 x, y, z   ;} Vector3;
typedef struct {f32 x, y, z, w;} Vector4;

typedef struct {Vector2 v0, v1        ;} Matrix2;
typedef struct {Vector3 v0, v1, v2    ;} Matrix3;
typedef struct {Vector4 v0, v1, v2, v3;} Matrix4;

typedef struct {f32    yz, zx, xy;} BiVector3;
typedef struct {f32 s, yz, zx, xy;} Rotor3D;





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

Vector3 v3_scale(Vector3 v, f32 s) {
    return (Vector3) {v.x * s, v.y * s, v.z * s};
}

f32 v3_dot(Vector3 a, Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

f32 v3_length(Vector3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

// will not check divide by zero
Vector3 v3_normalize(Vector3 v) {
    f32 s = 1 / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return (Vector3) {v.x * s, v.y * s, v.z * s};
}


Matrix2 m2_identity() {
    return (Matrix2) {
        {1, 0},
        {0, 1},
    };
}

Matrix2 m2_scale(Vector2 v) {
    return (Matrix2) {
        {v.x,   0},
        {  0, v.y}
    };
}

Matrix2 m2_rotate(f32 rad) {
    f32 c = cosf(rad);
    f32 s = sinf(rad);
    return (Matrix2) {
        { c,  s},
        {-s,  c}
    };
}

// note: right to left like in math, M N means first do N then M 
//       looks backward because store in column major order
Matrix2 m2_mul(Matrix2 M, Matrix2 N) {
    f32 out[2][2] = {0};
    f32* m = (f32*) &M;
    f32* n = (f32*) &N;
    for (int i = 0; i < 2; i++) {
        for (int k = 0; k < 2; k++) {
            for (int j = 0; j < 2; j++) {
                out[i][j] += m[k * 2 + j] * n[i * 2 + k];
            }
        }
    }
    return *((Matrix2*) out);
}

Matrix4 m4_identity() {
    return (Matrix4) {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };
}

Matrix4 m4_scale(Vector3 v) {
    return (Matrix4) {
        {v.x,   0,   0, 0},
        {  0, v.y,   0, 0},
        {  0,   0, v.z, 0},
        {  0,   0,   0, 1},
    };
}

Matrix4 m4_translate(Vector3 v) {
    return (Matrix4) {
        {  1,   0,   0,   0},
        {  0,   1,   0,   0},
        {  0,   0,   1,   0},
        {v.x, v.y, v.z,   1}
    };
}

// note: right to left like in math, M N means first do N then M 
//       looks backward because store in column major order
Matrix4 m4_mul(Matrix4 M, Matrix4 N) {
    f32 out[4][4] = {0};
    f32* m = (f32*) &M;
    f32* n = (f32*) &N;
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 4; k++) {
            for (int j = 0; j < 4; j++) {
                out[i][j] += m[k * 4 + j] * n[i * 4 + k];
            }
        }
    }
    return *((Matrix4*) out);
}

Matrix4 m4_perspective(f32 FOV, f32 aspect, f32 n, f32 f) {
    const f32 a = 1.0 / tan(FOV / 2.0);
    return (Matrix4) {
        { a / aspect,                      0,  0,  0},
        {          0,      (f + n) / (f - n),  0,  1},
        {          0,                      0,  a,  0},
        {          0, -2 * (f * n) / (f - n),  0,  0}
    };
}

Matrix4 m4_orthogonal(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
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

Rotor3D b3_to_r3d(f32 s, BiVector3 b) {
    return (Rotor3D) {s, b.yz, b.zx, b.xy};
}

// will not check divide by zero
BiVector3 b3_normalize(BiVector3 b) {
    f32 s = 1 / sqrtf(b.yz * b.yz + b.zx * b.zx + b.xy * b.xy); 
    return (BiVector3) {b.yz * s, b.zx * s, b.xy * s};
}

Rotor3D r3d_reverse(Rotor3D r) {
    return (Rotor3D) {r.s, -r.yz, -r.zx, -r.xy};
}

// will not check divide by zero
Rotor3D r3d_normalize(Rotor3D r) {
    f32 s = 1 / sqrtf(r.s * r.s + r.yz * r.yz + r.zx * r.zx + r.xy * r.xy);
    return (Rotor3D) {r.s * s, r.yz * s, r.zx * s, r.xy * s}; 
}

// normalize before using this
Rotor3D r3d_from_v3(Vector3 a, Vector3 b) {
    f32       d  = v3_dot(  a, b); // half angle trick
    BiVector3 bv = v3_wedge(a, b);
    return r3d_normalize((Rotor3D) {1 + d, bv.yz, bv.zx, bv.xy}); 
}

// normalize before using this
Rotor3D r3d_from_plane_angle(BiVector3 plane, f32 rad) {
    f32 s = sinf(rad / 2);
    f32 c = cosf(rad / 2);
    return (Rotor3D) {c, s * plane.yz, s * plane.zx, s * plane.xy};
}

Rotor3D r3d_mul(Rotor3D a, Rotor3D b) {
    return (Rotor3D) {
        a.s * b.s  - a.yz * b.yz - a.zx * b.zx - a.xy * b.xy,
        a.s * b.yz + a.yz * b.s  - a.zx * b.xy + a.xy * b.zx,
        a.s * b.zx + a.zx * b.s  - a.xy * b.yz + a.yz * b.xy,
        a.s * b.xy + a.xy * b.s  - a.yz * b.zx + a.zx * b.yz,
    };
}

// this is the exactly same as quaternion
// R* V R
Vector3 v3_rotate(Vector3 v, Rotor3D r) {

    // out result, a vector and a trivector
    f32 x   =  v.x * r.s  - v.y * r.xy + v.z * r.zx;
    f32 y   =  v.y * r.s  - v.z * r.yz + v.x * r.xy;
    f32 z   =  v.z * r.s  - v.x * r.zx + v.y * r.yz;
    f32 xyz = -v.x * r.yz - v.y * r.zx - v.z * r.xy;

    // trivector part will be cancelled, left with the vector rotate twice by rotor
    return (Vector3) {
        x * r.s - y * r.xy + z * r.zx - xyz * r.yz,
        y * r.s - z * r.yz + x * r.xy - xyz * r.zx,
        z * r.s - x * r.zx + y * r.yz - xyz * r.xy,
    };
}


/*
// inline all the v3_rotate(), cancel the 0 terms and merge, we get the functions below
Matrix4 r3d_to_m4(Rotor3D r) {
    Vector3 v0 = v3_rotate((Vector3) {1, 0, 0}, r);
    Vector3 v1 = v3_rotate((Vector3) {0, 1, 0}, r);
    Vector3 v2 = v3_rotate((Vector3) {0, 0, 1}, r);
    return (Matrix4) {
        { v0.x,  v0.y,  v0.z, 0},
        { v1.x,  v1.y,  v1.z, 0},
        { v2.x,  v2.y,  v2.z, 0},
        {    0,     0,     0, 1},
    };
}
*/

Matrix3 r3d_to_m3(Rotor3D r) {
    
    f32 s_s   = r.s  * r.s;
    f32 xy_xy = r.xy * r.xy;
    f32 yz_yz = r.yz * r.yz;
    f32 zx_zx = r.zx * r.zx;

    f32 s_xy  = r.s  * r.xy;
    f32 s_zx  = r.s  * r.zx;
    f32 s_yz  = r.s  * r.yz;

    f32 yz_zx = r.yz * r.zx;
    f32 yz_xy = r.yz * r.xy;
    f32 zx_xy = r.zx * r.xy;

    return (Matrix3) {
        { s_s + yz_yz - zx_zx  - xy_xy ,           2 * ( s_xy + yz_zx),          2 * (yz_xy - s_zx )},
        {           2 * (yz_zx - s_xy ), s_s - yz_yz +  zx_zx - xy_xy ,          2 * ( s_yz + zx_xy)},
        {           2 * (s_zx  + yz_xy),           2 * (zx_xy - s_yz ), s_s - yz_yz - zx_zx + xy_xy },
    };
}

Matrix4 r3d_to_m4(Rotor3D r) {
    
    f32 s_s   = r.s  * r.s;
    f32 xy_xy = r.xy * r.xy;
    f32 yz_yz = r.yz * r.yz;
    f32 zx_zx = r.zx * r.zx;

    f32 s_xy  = r.s  * r.xy;
    f32 s_zx  = r.s  * r.zx;
    f32 s_yz  = r.s  * r.yz;

    f32 yz_zx = r.yz * r.zx;
    f32 yz_xy = r.yz * r.xy;
    f32 zx_xy = r.zx * r.xy;

    return (Matrix4) {
        { s_s + yz_yz - zx_zx  - xy_xy ,           2 * ( s_xy + yz_zx),          2 * (yz_xy - s_zx ), 0},
        {           2 * (yz_zx - s_xy ), s_s - yz_yz +  zx_zx - xy_xy ,          2 * ( s_yz + zx_xy), 0},
        {           2 * (s_zx  + yz_xy),           2 * (zx_xy - s_yz ), s_s - yz_yz - zx_zx + xy_xy , 0},
        {                             0,                             0,                            0, 1},
    };
}





/* ==== lerps ==== */

Vector3 lerp_v3(Vector3 a, Vector3 b, f32 t) {
    return (Vector3) {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
    };
}

Vector4 lerp_v4(Vector4 a, Vector4 b, f32 t) {
    return (Vector4) {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t,
    };
}


Rotor3D nlerp_r3d(Rotor3D a, Rotor3D b, f32 t) {
    
    Rotor3D out = {
        a.s  + (b.s  - a.s ) * t,
        a.yz + (b.yz - a.yz) * t,
        a.zx + (b.zx - a.zx) * t,
        a.xy + (b.xy - a.xy) * t,
    };
    
    return r3d_normalize(out);
}

// todo: is this right?
Rotor3D slerp_r3d(Rotor3D a, Rotor3D b, f32 t) {
    
    f32 rad = acosf(a.s * b.s + a.yz * b.yz + a.zx * b.zx + a.xy * b.xy);
    f32 i   = 1 / sinf(rad);
    f32 s1  = sinf(rad - t * rad);
    f32 s2  = t * rad;

    Rotor3D out = {
        i * (s1 * a.s  + s2 * b.s ), 
        i * (s1 * a.yz + s2 * b.yz), 
        i * (s1 * a.zx + s2 * b.zx), 
        i * (s1 * a.xy + s2 * b.xy),
    };

    return r3d_normalize(out); // if don't have this, after a certain angle the result is scaling up the object
}





/* ==== Utility ==== */

void print_v2(Vector2 v) {
    printf("%12f %12f\n", v.x, v.y);
}

void print_v3(Vector3 v) {
    printf("%12f %12f %12f\n", v.x, v.y, v.z);
}

void print_v4(Vector4 v) {
    printf("%12f %12f %12f %12f\n", v.x, v.y, v.z, v.w);
}

void print_b3(BiVector3 b) {
    printf("%12f %12f %12f\n", b.yz, b.zx, b.xy);
}

void print_r3d(Rotor3D r) {
    printf("%12f %12f %12f %12f\n", r.s, r.yz, r.zx, r.xy);
}

void print_m2(Matrix2 M) {
    for (int i = 0; i < 2 * 2; i++) {
        if (i > 0 && i % 2 == 0) printf("\n");
        printf("%12f", ((f32*) &M)[i]);
    }
    printf("\n\n");
}

void print_m3(Matrix3 M) {
    for (int i = 0; i < 3 * 3; i++) {
        if (i > 0 && i % 3 == 0) printf("\n");
        printf("%12f", ((f32*) &M)[i]);
    }
    printf("\n\n");
}

void print_m4(Matrix4 M) {
    for (int i = 0; i < 4 * 4; i++) {
        if (i > 0 && i % 4 == 0) printf("\n");
        printf("%12f", ((f32*) &M)[i]);
    }
    printf("\n\n");
}

