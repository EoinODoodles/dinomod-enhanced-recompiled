#pragma once

#include "PR/ultratypes.h"
#include "sys/math.h"

#define LIMIT(var, min, max) {\
    if (var < min) {\
        var = min;\
    } else if (var > max) {\
        var = max;\
    }\
}

extern f32 sqrtf(f32 num);

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

void rotate_point_by_angle_2D(f32 x, f32 y, f32* ox, f32* oy, s16 theta);

f32 angle16_to_degrees(s32 angle16);
f32 angle16_to_radians(s32 angle16);
s32 degrees_to_angle16(f32 degrees);
s32 radians_to_angle16(f32 radians);

void matrix_multiply(MtxF* mA, MtxF* mB, MtxF* mO);
void matrix_diPrintf(MtxF* mtx);
void matrix_recomp_printf(MtxF* mtx);
void rotation_from_matrix(MtxF* mtx, s16* yaw, s16* pitch, s16* roll, f32 normalisingScale);
