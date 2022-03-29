#include <stdio.h>
#include <time.h>
#include <assert.h>

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
typedef struct {__m128 s, yz, zx, xy;} Rotor3D_4;

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


Rotor3D_4 r3d_mul_4(Rotor3D_4 a, Rotor3D_4 b) {
    return (Rotor3D_4) {
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
    __m128 r0    = _mm_add_ps(t0, t1m);
    __m128 r1    = _mm_add_ps(r0, t3m);
    __m128 vo    = _mm_sub_ps(r1, t2);    
    
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
        print_r3d(r3d_mul_simd2(a, b));
        print_r3d(out);
    }

    {
        float as [4] = {0.1, 0, 0, 0};
        float ayz[4] = {0.2, 0, 0, 0};
        float azx[4] = {0.3, 0, 0, 0};
        float axy[4] = {0.4, 0, 0, 0};

        float bs [4] = {0.5, 0, 0, 0};
        float byz[4] = {0.6, 0, 0, 0};
        float bzx[4] = {0.7, 0, 0, 0};
        float bxy[4] = {0.8, 0, 0, 0};
 
        float os [4];
        float oyz[4];
        float ozx[4];
        float oxy[4];
   
        Rotor3D_4 va;
        Rotor3D_4 vb;
        Rotor3D_4 vo;

        va.s  = _mm_load_ps(as);
        va.yz = _mm_load_ps(ayz);
        va.zx = _mm_load_ps(azx);
        va.xy = _mm_load_ps(axy);

        vb.s  = _mm_load_ps(bs);
        vb.yz = _mm_load_ps(byz);
        vb.zx = _mm_load_ps(bzx);
        vb.xy = _mm_load_ps(bxy);
        
        vo = r3d_mul_4(va, vb);

        _mm_store_ps(os,  vo.s);
        _mm_store_ps(oyz, vo.yz);
        _mm_store_ps(ozx, vo.zx);
        _mm_store_ps(oxy, vo.xy);

        printf("%f %f %f %f\n", os[0], oyz[0], ozx[0], oxy[0]);
    }

       
    const int count = 1024 * 32;

    #define SoA

    #ifdef AoS
    Rotor3D* a   = malloc(sizeof(Rotor3D) * count);
    Rotor3D* b   = malloc(sizeof(Rotor3D) * count);
    Rotor3D* out = malloc(sizeof(Rotor3D) * count);
    #endif
    

    #ifdef SoA
    int soa_count = count / 4;
    Rotor3D_4* va   = malloc(sizeof(Rotor3D_4) * soa_count);
    Rotor3D_4* vb   = malloc(sizeof(Rotor3D_4) * soa_count);
    Rotor3D_4* vout = malloc(sizeof(Rotor3D_4) * soa_count);
    
    assert((sizeof(Rotor3D) * count) == (sizeof(Rotor3D_4) * soa_count));
    #endif

    for (int i = 0; i < 16; i++) {


        #ifdef AoS
        for (int i = 0; i < count; i++) {
            a[i] = (Rotor3D) {rand() / 32768.0, rand() / 32768.0, rand() / 32768.0, rand() / 32768.0};
            b[i] = (Rotor3D) {rand() / 32768.0, rand() / 32768.0, rand() / 32768.0, rand() / 32768.0};
        }

        #define simd
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
        #endif


        #ifdef SoA
        for (int i = 0; i < soa_count; i++) {

            float as [4];
            float ayz[4];
            float azx[4];
            float axy[4];

            float bs [4];
            float byz[4];
            float bzx[4];
            float bxy[4];

            for (int j = 0; j < 4; j++) {

                as [j] = rand() / 32768.0;
                ayz[j] = rand() / 32768.0;
                azx[j] = rand() / 32768.0;
                axy[j] = rand() / 32768.0;

                bs [j] = rand() / 32768.0;
                byz[j] = rand() / 32768.0;
                bzx[j] = rand() / 32768.0;
                bxy[j] = rand() / 32768.0;
            }

            va[i].s  = _mm_load_ps(as);
            va[i].yz = _mm_load_ps(ayz);
            va[i].zx = _mm_load_ps(azx);
            va[i].xy = _mm_load_ps(axy);

            vb[i].s  = _mm_load_ps(bs);
            vb[i].yz = _mm_load_ps(byz);
            vb[i].zx = _mm_load_ps(bzx);
            vb[i].xy = _mm_load_ps(bxy);
        }

        time_it();
        for (int i = 0; i < soa_count; i++) {
            vout[i] = r3d_mul_4(va[i], vb[i]);
        }
        time_it();
        #endif

    }

}


