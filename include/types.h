#ifndef TYPES_H
#define TYPES_H

#define TRUE 1
#define FALSE 0

#define true 1
#define false 0

#include <stdint.h>

// TODO: This should be determined by some value that determines whether it
// should compile for 32-bit or 64-bit environments
#define __SIZE_T uint32_t

typedef uint8_t		bool;

typedef uint8_t 	u8;
typedef uint16_t 	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef int8_t  	i8;
typedef int16_t		i16;
typedef int32_t 	i32;
typedef int64_t 	i64;

typedef float       f32;
typedef double      f64;

typedef __SIZE_T  	size_t;
typedef signed int  ssize_t;

typedef i32         fd_t;
typedef u64         off_t;

typedef typeof((int*)0 - (int*)0)   ptrdiff_t;
typedef u32                         wint_t;

#endif
