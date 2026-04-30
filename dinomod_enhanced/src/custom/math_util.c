#include "PR/ultratypes.h"
#include "recomputils.h"
#include "math_util.h"

f32 lerp_float(f32 tValue, f32 start, f32 end){
    if (tValue == 0) {
        return start;
    } else if (tValue == end) {
        return end;
    }
    return start + (end - start)*tValue;
}

f32 ease_in_quad(f32 x) {
    return x * x;
}
f32 ease_in_cubic(f32 x) {
    return x * x * x;
}
f32 ease_in_quart(f32 x) {
    return x * x * x * x;
}

f32 ease_out_quad(f32 x) {
    return 1 - recomp_powf(1 - x, 2);
}
f32 ease_out_cubic(f32 x) {
    return 1 - recomp_powf(1 - x, 3);
}
f32 ease_out_quart(f32 x) {
    return 1 - recomp_powf(1 - x, 4);
}

f32 ease_in_out_quad(f32 x) {
    return x < 0.5 ? 2 * x * x : 1 - recomp_powf(-2 * x + 2, 2) / 2;
}
f32 ease_in_out_cubic(f32 x) {
    return x < 0.5 ? 4 * x * x * x : 1 - recomp_powf(-2 * x + 2, 3) / 2;
}
f32 ease_in_out_quart(f32 x) {
    return x < 0.5 ? 8 * x * x * x * x : 1 - recomp_powf(-2 * x + 2, 4) / 2;
}
