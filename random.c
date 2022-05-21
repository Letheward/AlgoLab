#include <stdio.h>
#include <stdlib.h>
#include <math.h>




/* ==== Types ==== */

typedef unsigned long long int u64;
typedef unsigned int           u32;
typedef signed   long long int s64;
typedef signed   int           s32;

typedef float  f32;
typedef double f64;





/* ==== Seeds ==== */

u64 seed_lcg           = 1;
u64 seed_xorshift      = 1;
u64 seed_xorshift_star = 1;
u64 seed_wy            = 1;
u32 seed_whisky        = 1;





/* ==== Algorithms ==== */

// note: this only goes up to 32768 in some libc
u64 c_random() {  
    return (u64) rand(); 
}

// [lcg](https://en.wikipedia.org/wiki/Linear_congruential_generator)
// note: it will be perfect when the table size is 2^n
u64 lcg() {

    const u64 a = 6364136223846793005;
    const u64 c = 1442695040888963407;
	
    u64* seed = &seed_lcg;

    *seed = *seed * a + c;
    
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





/* ==== Algorithms ==== */

void print_csv(u64 (*random)(), char* name, u64 table_count, s64 iteration) {
    
    s64* table = calloc(table_count, sizeof(s64));

    f64 highest = 0;
    f64 acc     = 0;
    
    for (int i = 0; i < table_count * iteration; i++)  table[random() % table_count]++;

    for (int i = 0; i < table_count; i++) {
        f64 error = abs(table[i] - iteration) / (f64) iteration;
        acc += error;
        if (error > highest) highest = error;
    }

    free(table);
    
    printf("%s, %llu, %lld, %f%%, %f%%\n", name, table_count, iteration, highest * 100, acc * 100 / (f64) table_count);
}



int main() {
    
    srand(1);

    printf("name, table_count, iteration, highest, average\n");

    #define print_csv_helper(f) print_csv(f, #f, i, j) 
    
    for (u64 i = 2; i < 32768; i++) {
        for (u64 j = 128; j < 1024 * 64; j *= 2) {
            
            print_csv_helper(c_random); 
            print_csv_helper(lcg); 
            print_csv_helper(xorshift); 
            print_csv_helper(xorshift_star); 
            print_csv_helper(wyrand); 
            print_csv_helper(whisky); 
            
        }
    }

}

