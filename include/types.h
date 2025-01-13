#ifndef TYPES_H
#define TYPES_H

#define TRUE 1
#define FALSE 0

typedef unsigned char   bool;

typedef __UINT8_TYPE__  u8;
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
typedef __UINT64_TYPE__ u64;

typedef __INT8_TYPE__   i8;
typedef __INT16_TYPE__  i16;
typedef __INT32_TYPE__  i32;
typedef __INT64_TYPE__  i64;

typedef float           f32;
typedef double          f64;

typedef __SIZE_TYPE__   size_t;

typedef typeof((int*)0 - (int*)0) ptrdiff_t;
typedef u32 wint_t;

#endif