#ifndef TYPES_H
#define TYPES_H

#define TRUE 1
#define FALSE 0

typedef unsigned char   bool;

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef long long unsigned int u64;

typedef signed char     i8;
typedef signed short    i16;
typedef signed int      i32;
typedef long long int   i64;

typedef float           f32;
typedef double          f64;

typedef unsigned int    size_t;

typedef typeof((int*)0 - (int*)0) ptrdiff_t;

typedef u32 wint_t;

#endif