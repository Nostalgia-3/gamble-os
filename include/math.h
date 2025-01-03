#ifndef MATH_H
#define MATH_H

#include "types.h"

f32 ceil(f32 x);
f32 floor(f32 x);

void set_seed(u32 s);

int fibonacci_lfsr(void);
int galois_lfsr(void);

#endif