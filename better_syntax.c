/* Types */ 

// types in binary 
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
typedef long double             f128;

#define for_array(array) for (int i = 0; i < sizeof(array) / sizeof(array[0]); i++)
