#pragma once

#include "PR/ultratypes.h"

#define LIMIT(var, min, max) {\
    if (var < min) {\
        var = min;\
    } else if (var > max) {\
        var = max;\
    }\
}

f32 lerp_float(f32 tValue, f32 start, f32 end);

f32 ease_in_quad(f32 tValue);
f32 ease_in_cubic(f32 tValue);
f32 ease_in_quart(f32 tValue);

f32 ease_out_quad(f32 tValue);
f32 ease_out_cubic(f32 tValue);
f32 ease_out_quart(f32 tValue);

f32 ease_in_out_quad(f32 tValue);
f32 ease_in_out_cubic(f32 tValue);
f32 ease_in_out_quart(f32 tValue);
