#include <stdio.h>
#include <stdlib.h>
#include <math.h>




/* ==== Types ==== */

typedef unsigned char           u8;
typedef unsigned short int      u16;
typedef unsigned int            u32;
typedef unsigned long long int  u64;
typedef signed char             s8;
typedef signed short            s16;
typedef signed int              s32;
typedef signed long long int    s64;
typedef float                   f32;
typedef double                  f64;





/* ==== Reference ==== */

s32 gcd_s32(s32 a, s32 b) {
    
    a = labs(a); 
    b = labs(b); 
    
    while (b) {
        s32 t = b;
        b = a % b;
        a = t;
    }

    return a;
}

s32 lcm_s32(s32 a, s32 b) {
    return a * b / gcd_s32(a, b);
}





/* ==== Rational ==== */

typedef struct {s32 numer, denom;} Rational32; // safe range: +-[1/2^15, 2^15/1] 
typedef struct {s64 numer, denom;} Rational64; // safe range: +-[1/2^31, 2^31/1]

// Vector and Matrix
typedef struct {Rational32 x, y;}       R32Vector2;
typedef struct {Rational32 x, y, z;}    R32Vector3;
typedef struct {Rational32 x, y, z, w;} R32Vector4;

typedef struct {R32Vector2 v0, v1;}         R32Matrix2;
typedef struct {R32Vector3 v0, v1, v2;}     R32Matrix3;
typedef struct {R32Vector4 v0, v1, v2, v3;} R32Matrix4;




// f32's precision may not be enough
f64 r32_to_f64(Rational32 in) {
    return (f64) in.numer / (f64) in.denom;
}


// note: will not check for divide by 0
Rational32 r32_reduce(Rational32 in) {
    
    s32 n = in.numer;
    s32 d = in.denom;
    
    u8 s = (n > 0) == (d > 0); // final sign

    n = labs(n); 
    d = labs(d); 
    
    // inline gcd(), because we alreay got abs()
    s32 k; 
    {
        s32 gn = n;
        s32 gd = d;
        while (gd) {
            s32 t = gd;
            gd = gn % gd;
            gn = t;
        }
        k = gn;
    }
   
    n = n * s - n * !s; // move sign to n 
    
    return (Rational32) {n / k, d / k};
}

Rational32 r32_invert(Rational32 in) {
    return (Rational32) {in.denom, in.numer};
}

Rational32 r32_neg(Rational32 in) {
    return (Rational32) {-in.numer, in.denom};
}

Rational32 r32_add(Rational32 a, Rational32 b) {
    s32 an = a.numer;
    s32 ad = a.denom;
    s32 bn = b.numer;
    s32 bd = b.denom;
    return r32_reduce((Rational32) {an * bd + bn * ad, ad * bd});
}

Rational32 r32_sub(Rational32 a, Rational32 b) {
    s32 an = a.numer;
    s32 ad = a.denom;
    s32 bn = b.numer;
    s32 bd = b.denom;
    return r32_reduce((Rational32) {an * bd - bn * ad, ad * bd});
}

// todo: this looks faster than do some gcd to each side and divide first then multiply, but will overflow earlier
Rational32 r32_mul(Rational32 a, Rational32 b) {
    Rational32 out = {a.numer * b.numer, a.denom * b.denom};
    return r32_reduce(out);
}

// note: will not check for divide by 0
// todo: this looks faster than do some gcd to each side and divide first then multiply, but will overflow earlier
Rational32 r32_div(Rational32 a, Rational32 b) {
    Rational32 out = {a.numer * b.denom, a.denom * b.numer};
    return r32_reduce(out);
}

// note: will not check for mod by 0
// todo: is this right?
Rational32 r32_mod(Rational32 a, Rational32 b) {
    s32 an = a.numer;
    s32 ad = a.denom;
    s32 bn = b.numer;
    s32 bd = b.denom;
    return r32_reduce((Rational32) {(an * bd) % (bn * ad), ad * bd});
}



// todo: make these faster?

u8 r32_equal(Rational32 a, Rational32 b) {
    Rational32 result = r32_sub(a, b);
    return result.numer == 0;
}

u8 r32_less(Rational32 a, Rational32 b) {
    Rational32 result = r32_sub(a, b);
    return result.numer < 0;
}

u8 r32_greater(Rational32 a, Rational32 b) {
    Rational32 result = r32_sub(a, b);
    return result.numer > 0;
}





/* ==== Linear Algebra === */

R32Matrix4 r32m4_mul(R32Matrix4 M, R32Matrix4 N) {
 
    Rational32* m = (Rational32*) &M;
    Rational32* n = (Rational32*) &N;
    Rational32 out[4][4] = {0};

    // prevent divide by 0
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out[i][j].denom = 1; 
        }
    }

    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 4; k++) {
            for (int j = 0; j < 4; j++) {
                out[i][j] = r32_add(out[i][j], r32_mul(m[k * 4 + j], n[i * 4 + k]));
            }
        }
    }
    
    return *((R32Matrix4*) out);
}









/* ==== Utils === */

Rational32 get_random_r32(s32 range) {
    s32 n = rand() % (range * 2) - range;
    s32 d = rand() % (range * 2) - range;
    if (n == 0) n = 1;
    if (d == 0) d = 1;
    return r32_reduce((Rational32) {n, d});
}

R32Matrix4 get_random_r32m4(s32 range) {
    Rational32 out[4 * 4];
    for (int i = 0; i < 4 * 4; i++) out[i] = get_random_r32(range); 
    return *((R32Matrix4*) out);
}

void print_r32(Rational32 in) {
    printf("%8d/%-8d", in.numer, in.denom);
}

void print_r32m4(R32Matrix4 in) {
    Rational32* p = (Rational32*) &in;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            print_r32(p[i * 4 + j]);
        }
        printf("\n");
    }
    printf("\n");
}





int main() {

    printf("[Arithmetic]\n\n");   
    {

        // If we get results (a, b) at this range, care should be taken before next arithmetic. 
        // It is close to overflow/underflow, even with doing some reduce to both side of the operator first,
        // because there is still some chance to have coprimes.   
        printf("Limit:\n"); 
        {
            Rational32 a = {32769, 32768};
            Rational32 b = {65537, 32768};
            Rational32 c = r32_mul(a, b);

            print_r32(a);
            printf(" * ");
            print_r32(b);
            printf(" = ");
            print_r32(c);
            printf(" = %.12f (overflow, wrong)\n", r32_to_f64(c));
            
            f64 fa = r32_to_f64(a);
            f64 fb = r32_to_f64(b);
            printf("   %.12f *    %6.12f = %.12f\n\n", fa, fb, fa * fb);
        }

        typedef Rational32 (*Function)(Rational32, Rational32);
        Function funcs[] = {r32_add, r32_sub, r32_mul, r32_div, r32_mod};

        for (int i = 0; i < 32; i++) {

            Rational32 a = get_random_r32(10);
            Rational32 b = get_random_r32(10);
            
            u32 index = rand() % 5;
            char* op;

            switch (index) {
                case 0:  op = " + ";  break;
                case 1:  op = " - ";  break;
                case 2:  op = " * ";  break;
                case 3:  op = " / ";  break;
                case 4:  op = " %% "; break;
                default: op = "   ";  break;
            }
           
            print_r32(a);
            printf(op);
            print_r32(b);
            printf(" = ");
            print_r32(funcs[index](a, b));
            printf("\n");
        }
    }
    
    printf("\n[Linear Algebra]\n\n");   
    {
        R32Matrix4 a = get_random_r32m4(10);
        R32Matrix4 b = get_random_r32m4(10);
        R32Matrix4 c = r32m4_mul(a, b);

        printf("A:\n");   
        print_r32m4(a);
        printf("B:\n");   
        print_r32m4(b);
        printf("A * B:\n");   
        print_r32m4(c);
    }
   
}

