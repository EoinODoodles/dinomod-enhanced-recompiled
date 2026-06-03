#include "modding.h"

#include "game/objects/object.h"
#include "sys/objexpr.h"
#include "sys/main.h"

extern struct {
    u8 unk0_0 : 1;
} D_800B2E00;

extern s32 D_80091720[];

RECOMP_PATCH s32 func_800334A4(Object* obj, Object* lookat, Vec3f* refPoint, HeadAnimation* anims, s16* arg4, f32 yOffset, s16 arg6, s16 arg7) {
    f32 dx;
    f32 dy;
    f32 dz; // fa1
    f32 xzDist;
    s16 sp84[2];
    s16 goal[2];
    s32 temp_lo;
    s16 var_a0;
    s16 var_a1;
    s32 pad;
    s32 var_v1;
    s16* bone;
    u8 sp6B;
    s32 temp_ft0;
    s16* var_t3;
    s32 i; // s3
    s32 j;

    sp6B = 0;
    dx = refPoint->x - lookat->srt.transl.x;
    dz = refPoint->z - lookat->srt.transl.z;
    dy = (refPoint->y + yOffset) - lookat->srt.transl.y;
    xzDist = sqrtf(SQ(dx) + SQ(dz));
    sp84[0] = (s16)(u16)arctan2_f(dx, dz) - (obj->srt.yaw & 0xFFFF);
    CIRCLE_WRAP(sp84[0]);
    sp84[1] = arg7 - (-arctan2_f(xzDist, dy) & 0xFFFF);
    CIRCLE_WRAP(sp84[1]);
    if (D_800B2E00.unk0_0) {
        sp84[0] -= 0x8000;
        sp84[1] = -sp84[1];
    }
    for (i = 0; i < 10; i++) {
        bone = func_80034804(obj, D_80091720[i]);
        if (bone == NULL) {
            // @recomp: Reset flag that flips the lookat direction. For some reason, this early return does not reset
            //          the flag like the end of the function does. This causes the flag to leak over into other calls
            //          causing various NPCs to have a messed up head lookat. Notably, the shop keeper sets this flag.
            D_800B2E00.unk0_0 = 0;
            return sp6B;
        }
        for (j = 0; j < 2; j++) {
            if (j % 2) {
                var_a0 = arg4[i + 15U] * 182.04f;
            } else {
                var_a0 = arg4[i] * 182.04f;
            }
            goal[j] = sp84[j];
            if (var_a0 < sp84[j]) {
                goal[j] = var_a0;
                sp84[j] -= var_a0;
            } else if (sp84[j] < -var_a0) {
                goal[j] = -var_a0;
                sp84[j] += var_a0;
            } else {
                sp84[j] = 0;
            }
        }
        if (anims != NULL) {
            anims->headGoalAngle = goal[0];
            func_80034250(anims, bone);
            anims[1].headGoalAngle = goal[1];
            func_80034518(anims + 1, bone, 10.0f, 500.0f);
            anims += 2;
        } else {
            var_t3 = arg4 + 15;
            var_a1 = (bone[1] + goal[0]) >> 1;
            var_a1 -= bone[1];
            temp_lo = ((s16) (-arg4[i] * 182.04f) / 10) * gUpdateRate;
            if (var_a1 < temp_lo) {
                var_a1 = temp_lo;
            } else {
                temp_lo = ((s16) (arg4[i] * 182.04f) / 10) * gUpdateRate;
                if (temp_lo < var_a1) {
                    var_v1 = temp_lo;
                } else {
                    var_v1 = var_a1;
                }
                var_a1 = var_v1;
            }
            var_a0 = (bone[0] + goal[1]) >> 1;
            var_a0 -= bone[0];
            temp_ft0 = (s16) (var_t3[i] * 182.04f);
            pad = (- temp_ft0 / 10) * gUpdateRate;
            if (var_a0 < pad) {
                var_a0 = pad;
            } else {
                if (((temp_ft0 / 20) * gUpdateRate) < var_a0) {
                    var_v1 = ((temp_ft0 / 20) * gUpdateRate);
                } else {
                    var_v1 = var_a0;
                }
                var_a0 = var_v1;
            }
            bone[0] += var_a0;
            bone[1] += var_a1;
        }
        if (i == 0) {
            var_v1 = (goal[0] - 4) < bone[1];
            if (var_v1 != 0) {
                var_v1 = bone[1] < (goal[0] + 4);
            }
            sp6B = var_v1;
        }
    }
    D_800B2E00.unk0_0 = 0;
    return sp84[0];
}
