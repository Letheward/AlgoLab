#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>




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





/* ==== Integer Algorithms ==== */

u64 seed_lcg           = 1;
u64 seed_xorshift      = 1;
u64 seed_xorshift_star = 1;
u64 seed_wy            = 1;
u32 seed_whisky        = 1;

// note: this only goes up to 32768 in some libc
u64 c_random() {  
    return (u64) rand(); 
}

// [lcg](https://en.wikipedia.org/wiki/Linear_congruential_generator)
// note: it will be perfect uniform when the table size at test is 2^n
u64 lcg() {

    const u64 a = 6364136223846793005;
    const u64 c = 1442695040888963407;
	
    u64* seed = &seed_lcg;

    *seed = *seed * a + c; // overflow so m = 2^64
    
    return *seed;
}

// [xorshift](https://en.wikipedia.org/wiki/Xorshift) 
// note: need to have a non-0 seed
u64 xorshift() {

	u64* seed = &seed_xorshift;

	u64 x = *seed;
    
    x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	
    *seed = x;

    return x;
}

// note: need to have a non-0 seed
u64 xorshift_star() {
    
    const u64 c = 0x2545f4914f6cdd1d;

    u64* seed = &seed_xorshift_star;

	u64 x = *seed;
    
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    
    *seed = x;
    
    return x * c;
}


// modified from: [wyhash](https://github.com/wangyi-fudan/wyhash)
u64 wyrand() {  

    const u64 c1 = 0xa0761d6478bd642f;
    const u64 c2 = 0xe7037ed1a0b428db;
    
    u64* seed = &seed_wy;
    
    *seed += c1;

    u64 t1 = *seed; 
    u64 t2 = *seed ^ c2; 

    t1 *= (t1 >> 32) | (t1 << 32);
    
    t2 *= (t2 >> 32) | (t2 << 32);
    t2  = (t2 >> 32) | (t2 << 32);
    
    return t1 ^ t2;
}

// modified from: [whisky](https://github.com/velipso/whisky)
u64 whisky() {
    
    u32* seed = &seed_whisky;
    *seed += 1;

    u32 i0 = *seed;
	u32 z0 = (i0 * 1831267127) ^ i0;
	u32 z1 = (z0 * 3915839201) ^ (z0 >> 20);
	u32 z2 = (z1 * 1561867961) ^ (z1 >> 24);
	
    return (u64) z2;
}





/* ==== Float Algorithms ==== */

typedef struct { u64 u, v, w; } Wichmann_Hill_State;

Wichmann_Hill_State seed_wichmann_hill = {10295, 20451, 105};

// note: this produced f64 from 0 to 1
f64 wichmann_hill() {

    Wichmann_Hill_State* s = &seed_wichmann_hill;

    s->u = (s->u * 171) % 30269;
    s->v = (s->v * 172) % 30307;
    s->w = (s->w * 170) % 30323;

    return fmod(s->u / 30269.0 + s->v / 30307.0 + s->w / 30323.0, 1.0);
}











/* ==== Test ==== */

void print_u64_test_csv(FILE* f, u64 (*random)(), char* name, u64 table_count, s64 iteration) {
    
    s64* table = calloc(table_count, sizeof(s64));

    f64 highest = 0;
    f64 acc     = 0;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < table_count * iteration; i++)  table[random() % table_count]++;

    clock_gettime(CLOCK_MONOTONIC, &end);
    f64 time = (end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec);

    for (int i = 0; i < table_count; i++) {
        f64 error = abs(table[i] - iteration) / (f64) iteration;
        acc += error;
        if (error > highest) highest = error;
    }

    free(table);
    
    fprintf(f, "%f%%, %f%%, %fs, ", highest * 100, acc * 100 / (f64) table_count, time);
}




int main() {


    #define csv_string(f) #f " (highest error), " #f " (average error), " #f " (time), " 
    #define print_helper(f) print_u64_test_csv(file, f, #f, i, 32768) 

    FILE* file = fopen("result.csv", "wb");
    
    srand(1);
    
    fprintf(
        file, 
        "table_size, "
        csv_string(c_random)
        csv_string(lcg)
        csv_string(xorshift)
        csv_string(xorshift_star)
        csv_string(wyrand)
        csv_string(whisky)
        "\n"
    );
    
    for (u64 i = 2; i < 32768; i++) {
        
        fprintf(file, "%llu, ", i);
        print_helper(c_random); 
        print_helper(lcg); 
        print_helper(xorshift); 
        print_helper(xorshift_star); 
        print_helper(wyrand); 
        print_helper(whisky); 
        fprintf(file, "\n");
        
        if (i % 10 == 0) printf("Finished %llu\n", i);
    }

}

