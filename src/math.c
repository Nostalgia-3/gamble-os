#include "math.h"
#include "types.h"

#define TERMS 10 // I have no idea what this should be

f32 ceil(f32 x) {
    i32 i = (i32)x;
    if(x > i)   return (f32) i + 1.0;
    else        return (f32) i;
}

f32 floor(f32 x) {
    i32 i = (i32)x;
    return (f32)i;
}

float power(float base, int exp) {
    if(exp < 0) {
        if(base == 0)
            return -0; // Error!!
        return 1 / (base * power(base, (-exp) - 1));
    }
    if(exp == 0)
        return 1;
    if(exp == 1)
        return base;
    return base * power(base, exp - 1);
}

int fact(int n) {
    return n <= 0 ? 1 : n * fact(n-1);
}

float sine(int deg) {
    deg %= 360; // make it less than 360
    float rad = deg * PI / 180;
    float sin = 0;

    int i;
    for(i = 0; i < TERMS; i++) { // That's Taylor series!!
        sin += power(-1, i) * power(rad, 2 * i + 1) / fact(2 * i + 1);
    }
    return sin;
}

float cosine(int deg) {
    deg %= 360; // make it less than 360
    float rad = deg * PI / 180;
    float cos = 0;

    int i;
    for(i = 0; i < TERMS; i++) { // That's also Taylor series!!
        cos += power(-1, i) * power(rad, 2 * i) / fact(2 * i);
    }
    return cos;
}

static u32 seed = 0xcafebabe; /* anything nonzero will do */

void set_seed(u32 s) {
    if (!s)
        seed = 0xcafebabe;
    else
        seed = s;
}

int fibonacci_lfsr(void) {
    /* taps in positions 32, 31, 28 */
    u32 bitIn = (seed ^ (seed >> 1) ^ (seed >> 4)) & 1;
    int bitOut = seed & 1;
    seed = (seed >> 1) | (bitIn << 31);
    return bitOut;
}

int galois_lfsr(void) {
    int bitOut = seed & 1;
    seed = (seed >> 1) ^ (seed & 1? 0xc8000000ul : 0);
    return bitOut;
}