#include "math.h"
#include "types.h"

f32 ceil(f32 x) {
    i32 i = (i32)x;
    if(x > i)   return (f32) i + 1.0;
    else        return (f32) i;
}

f32 floor(f32 x) {
    i32 i = (i32)x;
    return (f32)i;
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