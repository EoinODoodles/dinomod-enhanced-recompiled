#include "recomputils.h"
#include "math_util.h"
#include "string_util.h"
#include "PR/ultratypes.h"
#include "macros.h"
#include "sys/math.h"
#include "sys/print.h"

/** Linear interpolation from a start value to an end value, blending between them using a tValue (from 0 to 1). */
f32 lerp_float(f32 tValue, f32 start, f32 end){
    if (tValue == 0) {
        return start;
    } else if (tValue == 1) {
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

/** Rotate a point in a plane, around the origin. */
void rotate_point_by_angle_2D(f32 x, f32 y, f32* ox, f32* oy, s16 theta){
    f32 sinTheta = fsin16_precise(-theta);
    f32 cosTheta = fcos16_precise(-theta);

    *ox = x*cosTheta - y*sinTheta;
    *oy = x*sinTheta + y*cosTheta;
}

f32 cosf(f32 angle) {
    return sinf((M_PI_F/2) + angle);
}

/** Converts an angle from degrees into radians */
f32 degrees_to_radians(f32 degrees) {
    return degrees * (M_PI_F/180.0f);
}

/** Converts an angle from radians into degrees */
f32 radians_to_degrees(f32 radians) {
    return radians * (180.0f / M_PI_F);
}

/** Converts from the game's 16-bit angle system to degrees */
f32 angle16_to_degrees(s32 angle16) {
    CIRCLE_WRAP(angle16);
    return (f32)angle16*180.0f/((f32)M_180_DEGREES);
}

/** Converts from the game's 16-bit angle system to radians */
f32 angle16_to_radians(s32 angle16) {
    CIRCLE_WRAP(angle16);
    return (f32)angle16*M_PI_F/((f32)M_180_DEGREES);
}

/** Converts from degrees into the game's 16-bit angle system */
s32 degrees_to_angle16(f32 degrees) {
    f32 angleF = degrees*M_180_DEGREES/180.0f;
    s32 angle16 = (s32)angleF;
    CIRCLE_WRAP(angle16);
    return angle16;
}

/** Converts from radians into the game's 16-bit angle system */
s32 radians_to_angle16(f32 radians) {
    f32 angleF = radians*M_180_DEGREES/M_PI_F;
    s32 angle16 = (s32)angleF;
    CIRCLE_WRAP(angle16);
    return angle16;
}

/**
  * Calculates the cross product of two vectors.
  * The out vector can be one of the input vectors, since a temporary vector is used.
  */
void vec3_cross(Vec3f* vA, Vec3f* vB, Vec3f* vO) {
    Vec3f result;

    result.x = vA->y * vB->z - vA->z * vB->y;
    result.y = vA->z * vB->x - vA->x * vB->z;
    result.z = vA->x * vB->y - vA->y * vB->x;

    vO->x = result.x;
    vO->y = result.y;
    vO->z = result.z;
}

/*
 * Debug print a vector to the screen.
 */
void vec3_diPrintf(Vec3f* v) {
    diPrintf("%s   ", f2s(v->x));
    diPrintf("%s   ", f2s(v->y));
    diPrintf("%s   ", f2s(v->z));
    diPrintf("\n");
}

/*
 * Log a vector.
 */
void vec3_recomp_printf(Vec3f* v) {
    recomp_printf("%-6s\t", f2s(v->x));
    recomp_printf("%-6s\t", f2s(v->y));
    recomp_printf("%-6s",   f2s(v->z));
    recomp_printf("\n");
}

/** 
  * Does 4x4 matrix multiplication! 
  * The out matrix can be one of the input matrices, since a temporary matrix is used. 
  */
void matrix_multiply(MtxF* mA, MtxF* mB, MtxF* mO) {
    MtxF out;

    //Multiply, onto a temporary matrix
    for (u8 j = 0; j < 4; j++) {
        for (u8 i = 0; i < 4; i++) {
            out.m[i][j] = 0;
            for (u8 c = 0; c < 4; c++) {
                out.m[i][j] += (mA->m[c][j] * mB->m[i][c]);
            }
        }
    }

    //Store result onto output matrix
    for (u8 j = 0; j < 4; j++) {
        for (u8 i = 0; i < 4; i++) {
            mO->m[i][j] = out.m[i][j];
        }
    }
}

/*
 * Debug print a matrix to the screen.
 */
void matrix_diPrintf(MtxF *mtx) {
    //TODO: improve screen alignment/spacing between elements?
    diPrintf("\n");
    diPrintf("%4s    %4s     %4s   %4s\n", "aX", "aY", "aZ", "t");
    for (u8 j = 0; j < 4; j++) {
        for (u8 i = 0; i < 4; i++) {
            diPrintf("%6s    ", f2s(mtx->m[i][j]));
        }
        diPrintf("\n");
    }
}

/*
 * Log a matrix.
 */
void matrix_recomp_printf(MtxF *mtx) {
    recomp_printf("\n");
    recomp_printf("%-6s\t%-6s\t%-6s\t%-6s\n", "aX", "aY", "aZ", "t");
    for (u8 j = 0; j < 4; j++) {
        for (u8 i = 0; i < 4; i++) {
            recomp_printf("%-6s\t", f2s(mtx->m[i][j]));
        }
        recomp_printf("\n");
    }
}

// #define DEBUG_EULER_EXTRACT

/**
  * Extracts Euler angles (yaw/pitch/roll) from a transformation matrix.
  * Sourced from: https://www.geometrictools.com/Documentation/EulerAngles.pdf
  *
  * NOTE: this method assumes a ZXY rotation order (rotating around Y axis first, then X, then Z)
  *
  * Dinosaur Planet seems to use a ZXY rotation order for its SRTs 
  * (TODO: can anything affect the rotation order, like SRT flags?)
  */
void rotation_from_matrix(MtxF* mtx, s16* yaw, s16* pitch, s16* roll, f32 normalisingScale) {
    s32 outYaw;
    s32 outPitch;
    s32 outRoll;

    MtxF n;

    //Normalise the axial components using the object's scale (as a quick shortcut, since DP's object scale is always uniform in X/Y/Z)
    if ((normalisingScale != 1.0f) && (normalisingScale != 0.0f)) {
        for (u8 j = 0; j < 3; j++) { //Note: only need to use 3x3 part of matrix for extracting rotations
            for (u8 i = 0; i < 3; i++) {
                n.m[i][j] = mtx->m[i][j]/normalisingScale;
            }
        }
    }
    #ifdef DEBUG_EULER_EXTRACT
    matrix_diPrintf(&n);
    #endif

    //Extract angles

    //NOTE: requires normalised rotation matrix, otherwise sine won't be 1 at 90 degrees
    outPitch = radians_to_angle16(-asinf(n.m[2][1]));
    CIRCLE_WRAP(outPitch);

    if (outPitch == M_90_DEGREES) {
        outYaw  = 0;
        outRoll = atan2f_to_s(-n.m[1][0], n.m[0][0]);
    } else if (outPitch == -M_90_DEGREES) {
        outYaw  = 0;
        outRoll = atan2f_to_s(-n.m[1][0], n.m[0][0]);
    } else {
        outYaw  = atan2f_to_s(n.m[2][0], n.m[2][2]);
        outRoll = atan2f_to_s(n.m[0][1], n.m[1][1]);
    }

    CIRCLE_WRAP(outYaw);
    CIRCLE_WRAP(outRoll);

    #ifdef DEBUG_EULER_EXTRACT
    diPrintf("extractYaw   (degrees): %s\n", f2s(angle16_to_degrees(outYaw)));
    diPrintf("extractPitch (degrees): %s\n", f2s(angle16_to_degrees(outPitch)));
    diPrintf("extractRoll  (degrees): %s\n", f2s(angle16_to_degrees(outRoll)));
    #endif

    *yaw   = outYaw;
    *pitch = outPitch;
    *roll  = outRoll;
}
