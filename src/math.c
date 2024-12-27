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