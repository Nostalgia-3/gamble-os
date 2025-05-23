#pragma once

#include <stdint.h>

typedef __UINT8_TYPE__      uint8_t;
typedef __UINT16_TYPE__     uint16_t;
typedef __UINT32_TYPE__     uint32_t;
typedef __UINT64_TYPE__     uint64_t;

typedef __INT8_TYPE__       int8_t;
typedef __INT16_TYPE__      int16_t;
typedef __INT32_TYPE__      int32_t;
typedef __INT64_TYPE__      int64_t;

typedef uint32_t            size_t;
typedef int32_t             ssize_t;

typedef uint32_t            off_t;

typedef __INTMAX_TYPE__     intmax_t;
typedef __UINTMAX_TYPE__    uintmax_t;
typedef __PTRDIFF_TYPE__    ptrdiff_t;

typedef __UINTPTR_TYPE__    uintptr_t;

#define true    1
#define false   0

typedef uint8_t             bool;

typedef struct _module      module;