#include <stdio.h>
#include <time.h>

#include <x86intrin.h>




/* ==== Timer ==== */

typedef unsigned char u8;

typedef struct {
    struct timespec start;
    struct timespec end;
    u8 which;
} PerformanceTimer;

PerformanceTimer timer;

void time_it() {
    if (timer.which == 0) {
        clock_gettime(CLOCK_MONOTONIC, &timer.start);
        timer.which = 1;
    } else {
        clock_gettime(CLOCK_MONOTONIC, &timer.end);
        printf("%fs\n", (timer.end.tv_sec - timer.start.tv_sec) + 1e-9 * (timer.end.tv_nsec - timer.start.tv_nsec));
        timer.which = 0;
    }
}



/* ==== Rotor ==== */

typedef struct {float s, yz, zx, xy;} Rotor3D;

void print_r3d(Rotor3D r) {
    printf("%f %f %f %f\n", r.s, r.yz, r.zx, r.xy);
}


Rotor3D r3d_mul(Rotor3D a, Rotor3D b) {
    return (Rotor3D) {
        a.s * b.s  - a.yz * b.yz - a.zx * b.zx - a.xy * b.xy,
        a.s * b.yz + a.yz * b.s  - a.zx * b.xy + a.xy * b.zx,
        a.s * b.zx + a.zx * b.s  - a.xy * b.yz + a.yz * b.xy,
        a.s * b.xy + a.xy * b.s  - a.yz * b.zx + a.zx * b.yz,
    };
}



__m128 r3d_mul_simd(__m128 a, __m128 b) {  
    
    // prepare data
    __m128 a0000 = _mm_shuffle_ps(a, a, 0x00);
    __m128 a1123 = _mm_shuffle_ps(a, a, 0xe5);
    __m128 a2231 = _mm_shuffle_ps(a, a, 0x7a);
    __m128 a3312 = _mm_shuffle_ps(a, a, 0x9f);
    __m128 b1000 = _mm_shuffle_ps(b, b, 0x01);
    __m128 b2312 = _mm_shuffle_ps(b, b, 0x9e);
    __m128 b3231 = _mm_shuffle_ps(b, b, 0x7b);

    // 4 muls
    __m128 t0    = _mm_mul_ps(a0000, b);
    __m128 t1    = _mm_mul_ps(a1123, b1000);
    __m128 t2    = _mm_mul_ps(a2231, b2312);
    __m128 t3    = _mm_mul_ps(a3312, b3231);

    // flip sign bits
    __m128 mask  = _mm_castsi128_ps(_mm_set_epi32(0, 0, 0, 0x80000000));
    __m128 t1m   = _mm_xor_ps(t1, mask); 
    __m128 t3m   = _mm_xor_ps(t3, mask);
    
    // get result
    __m128 r0    = _mm_add_ps(t0, t1m);
    __m128 r1    = _mm_add_ps(r0, t3m);
    __m128 out   = _mm_sub_ps(r1, t2);    

    return out;
}



Rotor3D r3d_mul_simd2(Rotor3D a, Rotor3D b) {  
    
    Rotor3D out;
    __m128 va    = _mm_load_ps(&a.s);
    __m128 vb    = _mm_load_ps(&b.s);
    
    // prepare data
    __m128 a0000 = _mm_shuffle_ps(va, va, 0x00);
    __m128 a1123 = _mm_shuffle_ps(va, va, 0xe5);
    __m128 a2231 = _mm_shuffle_ps(va, va, 0x7a);
    __m128 a3312 = _mm_shuffle_ps(va, va, 0x9f);
    __m128 b1000 = _mm_shuffle_ps(vb, vb, 0x01);
    __m128 b2312 = _mm_shuffle_ps(vb, vb, 0x9e);
    __m128 b3231 = _mm_shuffle_ps(vb, vb, 0x7b);

    // 4 muls
    __m128 t0    = _mm_mul_ps(a0000, vb);
    __m128 t1    = _mm_mul_ps(a1123, b1000);
    __m128 t2    = _mm_mul_ps(a2231, b2312);
    __m128 t3    = _mm_mul_ps(a3312, b3231);

    // flip sign bits
    __m128 mask  = _mm_castsi128_ps(_mm_set_epi32(0, 0, 0, 0x80000000));
    __m128 t1m   = _mm_xor_ps(t1, mask); 
    __m128 t3m   = _mm_xor_ps(t3, mask);
    
    // get result
    __m128 tr0   = _mm_add_ps(t0,  t1m);
    __m128 tr1   = _mm_add_ps(tr0, t3m);
    __m128 vo    = _mm_sub_ps(tr1, t2);    
    
    _mm_store_ps(&out.s, vo);

    return out;
}



int main() {
    

    // correctness check
    {
        Rotor3D a = {0.1, 0.2, 0.3, 0.4};
        Rotor3D b = {0.5, 0.6, 0.7, 0.8};

        Rotor3D out;
        __m128 va = _mm_load_ps(&a.s);
        __m128 vb = _mm_load_ps(&b.s);
        __m128 vo = r3d_mul_simd(va, vb);    
        _mm_store_ps(&out.s, vo);

        print_r3d(r3d_mul(a, b));
        print_r3d(out);
        print_r3d(r3d_mul_simd2(a, b));
    }

       
    const int count = 1024 * 32;

    Rotor3D* a   = malloc(sizeof(Rotor3D) * count);
    Rotor3D* b   = malloc(sizeof(Rotor3D) * count);
    Rotor3D* out = malloc(sizeof(Rotor3D) * count);

    #define simd
    for (int i = 0; i < 64; i++) {

        for (int i = 0; i < count; i++) {
            a[i] = (Rotor3D) {rand() / 32768.0, rand() / 32768.0, rand() / 32768.0, rand() / 32768.0};
            b[i] = (Rotor3D) {rand() / 32768.0, rand() / 32768.0, rand() / 32768.0, rand() / 32768.0};
        }

        time_it();
        for (int i = 0; i < count; i++) {

            #ifdef scalar
            out[i] = r3d_mul(a[i], b[i]);
            #endif

            #ifdef simd
            __m128 va = _mm_load_ps(&a[i].s);
            __m128 vb = _mm_load_ps(&b[i].s);
            __m128 vo = r3d_mul_simd(va, vb);    
            _mm_store_ps(&out[i].s, vo);
            #endif

            #ifdef simd2
            out[i] = r3d_mul_simd2(a[i], b[i]);
            #endif
        }
        time_it();
    }

}


