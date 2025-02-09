#ifndef MATH_H
#define MATH_H

#include "types.h"

#define PI 3.1415926535897932384650288

f32 ceil(f32 x);
f32 floor(f32 x);

double abs(double x);

float power(float base, int exp);
int fact(int n);

float sine(int deg);
float cosine(int deg);

float sqrt(float v);

#define sin(deg) sine(deg)
#define cos(deg) cosine(deg)

void set_seed(u32 s);

int fibonacci_lfsr(void);
int galois_lfsr(void);

#endif