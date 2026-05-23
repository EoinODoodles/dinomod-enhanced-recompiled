#include "modding.h"
#include "recompconfig.h"

#include "dlls/objects/210_player.h"
#include "dlls/objects/418_DFriverflow.h"
#include "dlls/engine/27.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "dll.h"

enum RecompLogAButtonMode {
    RECOMP_LOG_ROWING_TAP,
    RECOMP_LOG_ROWING_HOLD
};

static int dinomod_log_can_hold_a(void) {
    return recomp_get_config_u32("log_a_button") == RECOMP_LOG_ROWING_HOLD;
}

#include "recomp/dlls/objects/793_BWlog_recomp.h"

typedef struct {
    ObjSetup base;
    u8 _unk18[2];
    s16 unk1A;
} ObjType23Setup;

typedef struct {
    DLL27_Data unk0;
    Vec3f unk260[2];
    Vec3f unk278[2];
    f32 unk290[2];
    f32 unk298[2];
    Vec4f unk2A0;
    f32 unk2B0;
    f32 unk2B4;
    f32 unk2B8;
    f32 unk2BC;
    f32 unk2C0;
    f32 unk2C4;
    f32 unk2C8;
    f32 unk2CC;
    f32 unk2D0[2];
    f32 unk2D8[2];
    f32 unk2E0[2];
    u8 _unk2E8[0x2F8 - 0x2E8];
    f32 unk2F8;
    f32 unk2FC;
    f32 unk300[2];
    f32 unk308;
    f32 unk30C;
    f32 unk310;
    u32 unk314;
    s32 unk318;
    u16 unk31C[2];
    u16 unk320;
    s16 unk322;
    s16 unk324;
    s16 unk326;
    s16 unk328;
    u8 unk32A;
    u8 unk32B;
    u8 unk32C;
    u8 unk32D;
    u8 unk32E;
    u8 _unk32E[0x338 - 0x32F];
    Object *unk338; // dockpoint
} BWlog_Data;

// @recomp: Custom setup
typedef struct {
    ObjSetup setup;
    s8 startRotation;
} BWlog_Setup;

extern Vec3f _data_0[2];
extern f32 _data_18[2];
extern u8 _data_20[2];

extern void dll_793_func_EB0(Object* self, BWlog_Data* objdata, s32 arg2);
extern void dll_793_func_1600(Object* self, BWlog_Data* objdata);
extern void dll_793_func_178C(Object* self, BWlog_Data* objdata, s32 arg2);
extern void dll_793_func_1858(Object* self, BWlog_Data* objdata);
extern void dll_793_func_1A4C(Object* self, BWlog_Data* objdata);
extern void dll_793_func_1A5C(Object* self, BWlog_Data* objdata);
extern void dll_793_func_1C18(Object* self, BWlog_Data* objdata);
extern void dll_793_func_2020(Object* arg0, BWlog_Data* arg1);
extern void dll_793_func_2444(Object* self, BWlog_Data* objdata);

// @recomp: Copy of DFlog's anim callback
static int recomp_BWlog_animcallback(Object *self, Object *a1, AnimObj_Data *a2, s8 a3) {
    func_800267A4(self);
    return 0;
}

RECOMP_PATCH void dll_793_setup(Object *self, BWlog_Setup *setup, s32 arg2) {
    BWlog_Data *objdata = (BWlog_Data*)self->data;
    s32 i;

    // @recomp: Register anim callback. DFlog was swapped with BWlog in dinomod but BWlog is missing
    //          an anim callback, but there's some sequences that assume one is set up to adjust collision.
    self->animCallback = recomp_BWlog_animcallback;

    gDLL_27->vtbl->init(&objdata->unk0, 
        DLL27FLAG_NONE, 
        DLL27FLAG_8000000 | DLL27FLAG_40000 | DLL27FLAG_20 | DLL27FLAG_2 | DLL27FLAG_1, 
        DLL27MODE_1);
    gDLL_27->vtbl->setup_terrain_collider(&objdata->unk0, 2, _data_0, _data_18, _data_20);
    // @recomp: Add block hits collider
    gDLL_27->vtbl->setup_hits_collider(&objdata->unk0, 2, _data_0, _data_18, 8);
    objdata->unk0.boundsYExtension = 100;
    obj_add_object_type(self, OBJTYPE_Vehicle);
    objdata->unk31C[1] = 0x2000;
    objdata->unk2B4 = 15.0f;
    
    self->shadow->flags |= (OBJ_SHADOW_FLAG_WATER_SURFACE | OBJ_SHADOW_FLAG_TOP_DOWN | OBJ_SHADOW_FLAG_USE_OBJ_YAW | OBJ_SHADOW_FLAG_CUSTOM_COLOR | OBJ_SHADOW_FLAG_CUSTOM_DIR);
    self->shadow->r = 0xFF;
    self->shadow->g = 0xFF;
    self->shadow->b = 0xFF;
    self->shadow->a = 0x7F;

    for (i = 0; i < 2; i++) {
        objdata->unk260[i].x = self->srt.transl.x;
        objdata->unk260[i].y = self->srt.transl.y;
        objdata->unk260[i].z = self->srt.transl.z;
    }

    // @recomp: Do the same hit info setup as DFlog. Helps with cases where an Override takes control
    //          over the log instantly after being spawned.
    self->objhitInfo->unk58 |= 1;
    self->objhitInfo->unk58 |= 4;
    // @recomp: Initialize DLL 27 state. In cases where an Override takes control away from the log right
    //          after the log spawns, sometimes this state won't be initialized with the log's current position.
    //          This is a problem with the totem puzzle log, where the log will warp away a bit after regaining
    //          control if this state is not set up right.
    gDLL_27->vtbl->reset(self, &objdata->unk0);

    // @recomp: Support start yaw via setup
    self->srt.yaw = setup->startRotation << 8;
}

// This is yoinked and modified from dll_27_func_1D60. Looks at the log collisions and rotates the log
// such that it reacts to moving along a wall. This effect can also be enabled by setting the DLL 27
// flag 0x8000, but the built-in version of this applies a very weak spin.
// static void wall_react(Object* arg0, DLL27_Data* arg1) {
//     f32 temp;
//     f32 f2;
//     f32 f0;
//     s32 var_s1_2;
//     s32 var_s0_2;
//     f32 f14;
//     u8 temp_t7;
//     s8 var_a1;
//     f32 f12;
//     s32 i;
//     SRT spDC;
//     f32 temp2;
//     f32 spC8[4];
//     f32 spB8[4];
//     f32 spA8[4];
//     MtxF sp68;
//     Vec3f globalPosition;

//     for (s32 i = 0; i < 2; i++) {
//         // Note: Don't do anything if we're not touching a wall, otherwise we get a weird wiggle at higher fps
//         Vec3f *array = NULL;
//         if (i == 0) {
//             if (!(arg1->unk25C & 3)) {
//                 continue;
//             }
//             array = arg1->unk38;
//         } else {
//             if (!(arg1->hitsTouchBits & 3)) {
//                 continue;
//             }
//             array = arg1->unk110;
//         }

//         temp_t7 = (arg1->numTestPoints >> 4);
//         globalPosition.x = 0.0f;
//         globalPosition.y = 0.0f;
//         globalPosition.z = 0.0f;
//         for (i = 0; i < (temp_t7*3); i+=3) {
//             globalPosition.x += array[0].f[i];
//             globalPosition.y += array[0].f[i+1];
//             globalPosition.z += array[0].f[i+2];
//         }
//         VECTOR_SCALE(globalPosition, 1.0f / temp_t7);

//         spDC.yaw = -arg0->srt.yaw;
//         spDC.pitch = -arg0->srt.pitch;
//         spDC.roll = -arg0->srt.roll;
//         spDC.scale = 1;
//         spDC.transl.x = -globalPosition.x;
//         spDC.transl.y = -globalPosition.y;
//         spDC.transl.z = -globalPosition.z;
//         matrix_from_srt_reversed(&sp68, &spDC);
//         for (var_s0_2 = 0, i = 0; i < temp_t7; i++) {
//             vec3_transform(&sp68, 
//                             array[var_s0_2].x, array[var_s0_2].y, array[var_s0_2].z, 
//                             &spC8[i], &spB8[i], &spA8[i]);
//             var_s0_2++;
//         }

//         var_s1_2 = 0;
//         var_s0_2 = 1;
//         var_a1 = 1;

//         f0 = spC8[0] + spC8[var_s1_2];
//         f2 = spA8[0] + spA8[var_s1_2];
//         temp = spC8[var_s0_2] + spC8[var_a1];
//         f12 = f0 - temp;
//         temp2 = spA8[var_s0_2] + spA8[var_a1];
//         f14 = f2 - temp2;
//         arg0->srt.yaw += (s16) ((arctan2_f(f12, f14) & 0xFFFF) + 0x8000);
//     }
// }

RECOMP_PATCH void dll_793_control(Object* self) {
    BWlog_Data* objdata;
    f32 var_fv1;
    f32 sp184[3];
    f32 sp178[3];
    SRT sp160;
    MtxF sp120;
    MtxF spE0;
    MtxF spA0;
    f32 sp9C;
    f32 sp98;
    ObjType23Setup* temp_s0;
    s32 i;

    // @recomp: Run objhit func just like DFlog does. Resets the state modified by the anim callback.
    func_8002674C(self);

    objdata = (BWlog_Data*)self->data;
    sp98 = 10000.0f;
    objdata->unk338 = obj_get_nearest_type_to(OBJTYPE_Dockpoint, self, &sp98);
    if (objdata->unk338 != NULL) {
        temp_s0 = (ObjType23Setup*)objdata->unk338->setup;
        sp98 = vec3_distance(&self->globalPosition, &objdata->unk338->globalPosition);
        if (objdata->unk32E == 2) {
            var_fv1 = 0.95f;
        } else {
            var_fv1 = 0.5f;
        }
        if (sp98 < (f32) temp_s0->unk1A) {
            for (i = 0; i < 2; i++) {
                objdata->unk278[i].x *= var_fv1;
                objdata->unk278[i].z *= var_fv1;
            }
        } else {
            objdata->unk338 = NULL;
        }
    }
    dll_793_func_1C18(self, objdata);
    if (objdata->unk32E == 2) {
        dll_793_func_1600(self, objdata);
        switch (objdata->unk32A) {
        case 1:
        case 2:
            dll_793_func_1858(self, objdata);
            break;
        case 3:
        case 4:
            dll_793_func_1A4C(self, objdata);
            break;
        default:
            dll_793_func_1A5C(self, objdata);
            break;
        }
        // @recomp: Don't apply turn to yaw directly to behave more like DFLog (turn logic moved to below code)
        //self->srt.yaw -= (s32) ((f32) objdata->unk322 * (60.0f - ((f32) objdata->unk324 * 0.05f)) * 0.1f * gUpdateRateF) & 0xFFFF & 0xFFFF;
    }
    sp160.yaw = self->srt.yaw;
    sp160.pitch = self->srt.pitch;
    sp160.roll = self->srt.roll;
    sp160.scale = 1.0f;
    sp160.transl.x = self->srt.transl.x;
    sp160.transl.y = self->srt.transl.y;
    sp160.transl.z = self->srt.transl.z;
    matrix_from_srt(&sp120, &sp160);
    sp160.roll = 0;
    sp160.transl.x = 0;
    sp160.transl.y = 0;
    sp160.transl.z = 0;
    matrix_from_srt(&spE0, &sp160);
    sp160.yaw = -sp160.yaw;
    sp160.pitch = -sp160.pitch;
    matrix_from_srt_reversed(&spA0, &sp160);
    for (i = 0; i < 2; i++) {
        vec3_transform(&sp120, 
                       _data_0[i].x, _data_0[i].y, _data_0[i].z, 
                       &objdata->unk260[i].x, &objdata->unk260[i].y, &objdata->unk260[i].z);
        dll_793_func_EB0(self, objdata, i);
        vec3_transform(&spA0, 
                       objdata->unk2D0[i], 0.0f, objdata->unk2E0[i], 
                       &sp184[0], &sp184[1], &sp184[2]);
        // @recomp: Don't cut riverflow strength in half (many of the game's riverflows were designed for DFlog
        //          which is affected by riverflows much more than BWLog). Note that this does make the Blackwater
        //          Canyon riverflows feel stronger and may need to be tweaked for that level.
        // sp184[0] *= -0.5f;
        // sp184[2] *= 0.5f;
        sp184[0] *= -1.0f;
        objdata->unk310 = sqrtf(SQ(sp184[2]) + SQ(sp184[0]));
        sp184[2] += objdata->unk2FC;

        // @recomp: Apply turn by rotating the front/end log points (like DFLog rather than altering yaw directly)
        if (objdata->unk32E == 2) {
            f32 turn = (f32) objdata->unk322 * (60.0f - ((f32) objdata->unk324 * 0.05f)) / 60.0f * 0.01f;
            // Reduce turning effectiveness with increased pitch to help avoid fast spiralling when going down waterfalls
            f32 pitchDamp = (f32)self->srt.pitch / (f32)M_90_DEGREES;
            if (pitchDamp < 0) {
                pitchDamp = -pitchDamp;
            }
            turn *= (1.0f - MIN(pitchDamp, 1.0f));
            if (i == 1) {
                sp184[0] -= turn;
            } else {
                sp184[0] += turn;
            }
        }

        // @recomp: Add roll movement here instead of forcing unk298. Note: Doing this here makes rolling affected
        //          by riverflows, which helps prevent escaping intended areas. This shouldn't have too much of
        //          an effect on the Blackwater Canyon rapids.
        if (objdata->unk32A == 1) {
            sp184[0] += -2.5f;
        } else if (objdata->unk32A == 2) {
            sp184[0] += 2.5f;
        }

        objdata->unk290[i] += ((sp184[2] - objdata->unk290[i]) * gUpdateRateF * 0.1f);
        // @recomp: Allow lateral movement while rolling (we changed how roll movement is applied)
        //if (objdata->unk32A == 0) {
            objdata->unk298[i] += ((sp184[0] - objdata->unk298[i]) * gUpdateRateF * 0.1f);
        //}
        vec3_transform(&spE0, 
                       objdata->unk298[i], 0.0f, -objdata->unk290[i], 
                       &objdata->unk278[i].x, &sp9C, &objdata->unk278[i].z);
        // @recomp: Clamp pitch acceleration to avoid launches at extreme angles.
        //          There might be a better way to patch this, but this works pretty much perfectly.
        CLAMP(objdata->unk278[i].y, -10.0f, 10.0f);
        sp184[0] = objdata->unk278[i].x * gUpdateRateF;
        sp184[1] = objdata->unk278[i].y * gUpdateRateF;
        sp184[2] = objdata->unk278[i].z * gUpdateRateF;
        objdata->unk260[i].x = objdata->unk260[i].x + sp184[0];
        objdata->unk260[i].y = objdata->unk260[i].y + sp184[1];
        objdata->unk260[i].z = objdata->unk260[i].z + sp184[2];
    }
    sp178[0] = objdata->unk260[0].x + objdata->unk260[1].x;
    sp178[1] = objdata->unk260[0].y + objdata->unk260[1].y;
    sp178[2] = objdata->unk260[0].z + objdata->unk260[1].z;
    self->srt.transl.x = sp178[0] * 0.5f;
    self->srt.transl.y = sp178[1] * 0.5f;
    self->srt.transl.z = sp178[2] * 0.5f;
    sp178[0] = objdata->unk260[1].x - objdata->unk260[0].x;
    sp178[1] = objdata->unk260[1].y - objdata->unk260[0].y;
    sp178[2] = objdata->unk260[1].z - objdata->unk260[0].z;
    self->srt.pitch = -arctan2_f(sp178[1], sqrtf(SQ(sp178[2]) + SQ(sp178[0])));
    // @recomp: Clamp pitch to avoid reaching exactly 90 degrees (clamps to ~87 degrees).
    //          This allows the player to move forward slightly even when vertical.
    CLAMP(self->srt.pitch, -0x3E00, 0x3E00);
    // @recomp: Calculate yaw from log front/back points (like DFLog)
    self->srt.yaw = arctan2_f(sp178[0], sp178[2]);
    gDLL_27->vtbl->func_1E8(self, &objdata->unk0, gUpdateRateF);
    gDLL_27->vtbl->func_5A8(self, &objdata->unk0);
    gDLL_27->vtbl->func_624(self, &objdata->unk0, gUpdateRateF);
    // @recomp: When not rolling, rotate log when bumping into walls
    // TODO: this is fun but it messes up going down waterfalls. will need some changes before we can enable it
    // if (objdata->unk32A == 0) {
    //     wall_react(self, &objdata->unk0);
    // }
    dll_793_func_2020(self, objdata);
    dll_793_func_2444(self, objdata);
}

RECOMP_PATCH void dll_793_func_EB0(Object* self, BWlog_Data* objdata, s32 arg2) {
    f32 temp_fa1;
    f32 temp_ft5;
    f32 temp_fv0;
    f32 sp60;
    s32 sp50;
    f32 var_fv0;
    f32 var_fv1;
    s32 var_v1;
    f32 temp;

    // @recomp: Use floor position when water is not detected
    f32 floor = objdata->unk0.waterYList[arg2];
    s32 foundWater = TRUE;
    if (floor <= -100000.0f) {
        floor = objdata->unk0.floorYList[arg2];
        foundWater = FALSE;
    }

    temp = objdata->unk2B4 + floor;
    if ((objdata->unk260[arg2].y + 30.0f) < temp) {
        diPrintf("Water too high\n");
    }
    // @recomp: Don't apply the pitch wiggle if not actually in water. Avoids the log wiggling
    //          when it's up on the CClogpush cliff.
    if (foundWater) {
        temp += (fsin16_precise(objdata->unk31C[arg2]) * 1.5f);
        objdata->unk31C[arg2] += (gUpdateRateF * 512.0f);
    }
    sp60 = temp - objdata->unk260[arg2].y;
    if ((sp60 > 0.0f) && (objdata->unk300[arg2] < 0.0f)) {
        var_fv0 = objdata->unk278[arg2].y * 127.0f;
        if (var_fv0 < 0.0f) {
            var_fv0 = -var_fv0;
        }
        if (var_fv0 > 127.0f) {
            var_fv0 = 127.0f;
        }
        if (var_fv0 > 20.0f) {
            gDLL_6_AMSFX->vtbl->play(self, 0xA75, (u8) var_fv0, NULL, NULL, 0, NULL);
        }
    } else if ((sp60 < 0.0f) && (objdata->unk300[arg2] > 0.0f)) {
        var_fv0 = objdata->unk278[arg2].y * 127.0f;
        if (var_fv0 < 0.0f) {
            var_fv0 = -var_fv0;
        }
        if (var_fv0 > 127.0f) {
            var_fv0 = 127.0f;
        }
        if (var_fv0 > 20.0f) {
            gDLL_6_AMSFX->vtbl->play(self, 0xA74, (u8) var_fv0, NULL, NULL, 0, NULL);
        }
    }
    objdata->unk300[arg2] = sp60;
    if (sp60 > 25.0f) {
        sp60 = 25.0f;
    }
    if (sp60 < 0.0f) {
        sp60 = 0.0f;
    }
    objdata->unk278[arg2].y = (f32) (objdata->unk278[arg2].y + ((sp60 / 15.0f) * 0.15f * gUpdateRateF));
    objdata->unk278[arg2].y = (f32) (objdata->unk278[arg2].y - (0.1f * gUpdateRateF));
    var_v1 = (s32) gUpdateRate;
    diPrintf("[%d]=%f\n", arg2, &sp60);
    if (sp60 > 0.0f) {
        if (objdata->unk278[arg2].y < 0.0f) {
            var_fv1 = sp60 / 25.0f;
            temp_ft5 = 1.0f + var_fv1;
            if (var_fv1 > 1.0f) {
                var_fv1 = 1.0f;
            }
            var_fv1 = (1.0f - var_fv1);
            temp_fa1 = (0.007000029f * var_fv1) + 0.988f;
            while (var_v1--) {
                temp_fv0 = objdata->unk278[arg2].y;
                if (temp_fv0 > 0/*.0f*/) {
                    var_fv1 = temp_fv0;
                } else {
                    var_fv1 = -temp_fv0;
                }
                objdata->unk278[arg2].y = (f32) (temp_fv0 - (temp_fv0 * var_fv1 * 0.1f * temp_ft5));
                objdata->unk290[arg2] = (f32) (objdata->unk290[arg2] * temp_fa1);
                objdata->unk298[arg2] = (f32) (objdata->unk298[arg2] * temp_fa1);
                objdata->unk2FC *= 0.99f;
            }
            return;
        }
        
        var_fv1 = sp60 / 25.0f;
        temp_ft5 = 1.0f + var_fv1;
        if (var_fv1 > 1.0f) {
            var_fv1 = 1.0f;
        }
        var_fv1 = (1.0f - var_fv1);
        temp_fa1 = (0.007000029f * var_fv1) + 0.988f;
        while (var_v1--) {
            temp_fv0 = objdata->unk278[arg2].y;
            if (temp_fv0 > 0/*.0f*/) {
                var_fv1 = temp_fv0;
            } else {
                var_fv1 = -temp_fv0;
            }
            objdata->unk278[arg2].y = (f32) (temp_fv0 - (temp_fv0 * var_fv1 * 0.1f * temp_ft5));
            objdata->unk290[arg2] = (f32) (objdata->unk290[arg2] * temp_fa1);
            objdata->unk298[arg2] = (f32) (objdata->unk298[arg2] * temp_fa1);
            objdata->unk2FC *= 0.99f;
        }
        return;
    }
    
    while (var_v1--) {
        objdata->unk278[arg2].y = (f32) (objdata->unk278[arg2].y * 0.994f);
        objdata->unk290[arg2] = (f32) (objdata->unk290[arg2] * 0.995f);
        objdata->unk298[arg2] = (f32) (objdata->unk298[arg2] * 0.99f);
        objdata->unk2FC *= 0.99f;
    }
}

RECOMP_PATCH void dll_793_func_1600(Object* self, BWlog_Data* objdata) {
    s32 doubleTappedA;

    objdata->unk320 = joy_get_pressed(0);
    objdata->unk322 = joy_get_stick_x(0);
    objdata->unk324 = joy_get_stick_y(0);
    objdata->unk2F8 -= gUpdateRateF;
    if (objdata->unk2F8 <= 0.0f) {
        objdata->unk32C = 0;
        objdata->unk2F8 = 0.0f;
    }
    doubleTappedA = FALSE;
    if (objdata->unk320 & A_BUTTON) {
        if (objdata->unk32C != 0) {
            doubleTappedA = TRUE;
            objdata->unk32C = 0;
            objdata->unk2F8 = 0.0f;
        } else {
            objdata->unk32C = 1;
            objdata->unk2F8 = 15.0f;
        }
    }
    if (doubleTappedA) {
        if (objdata->unk322 >= 0x15) {
            dll_793_func_178C(self, objdata, 0);
            objdata->unk32A = 2;
            return;
        }
        if (objdata->unk322 < -0x14) {
            dll_793_func_178C(self, objdata, 1);
            objdata->unk32A = 1;
        }
      // @recomp: If enabled, allow the A button to be held instead of requiring repeated tapping
    } else if ((objdata->unk320 & A_BUTTON) || (dinomod_log_can_hold_a() && joy_get_buttons(0) & A_BUTTON)) {
        objdata->unk2B8 = 30.0f;
    }
}

RECOMP_PATCH void dll_793_func_178C(Object* self, BWlog_Data* objdata, s32 arg2) {
    if (arg2 != 0) {
        objdata->unk2A0.x = 1500.0f;
        objdata->unk2A0.y = 500.0f;
        objdata->unk2A0.z = 2000.0f;
        objdata->unk2A0.w = 4000.0f;
        objdata->unk32B = 2;
        // @recomp: Don't force lateral movement for rolling here, we'll factor in this speed in the normal movement code
        // objdata->unk298[0] = -2.5f;
        // objdata->unk298[1] = -2.5f;
    } else {
        objdata->unk2A0.x = -1500.0f;
        objdata->unk2A0.y = -500.0f;
        objdata->unk2A0.z = -2000.0f;
        objdata->unk2A0.w = -4000.0f;
        objdata->unk32B = 3;
        // @recomp: Ditto
        // objdata->unk298[0] = 2.5f;
        // objdata->unk298[1] = 2.5f;
    }
    objdata->unk318 = 0;
    objdata->unk2BC = 0.0f;
    objdata->unk2B0 = objdata->unk2A0.x;
    objdata->unk2C8 = 0.0f;
    objdata->unk2CC = 0.0f;
}

RECOMP_PATCH void dll_793_func_2020(Object* arg0, BWlog_Data* arg1) {
    u8 var_a1;
    u8 sp3E;
    s32 var_v1;

    if (arg1->unk314 == 0) {
        gDLL_6_AMSFX->vtbl->play(arg0, 0xA77, 0x7F, &arg1->unk314, NULL, 0, NULL);
    } else {
        arg1->unk30C = arg1->unk310 * 127.0f;
        arg1->unk30C += fsin16_precise(arg1->unk328) * 30.0f;
        if (arg1->unk30C < 30.0f) {
            arg1->unk30C = 30.0f;
        } else if (arg1->unk30C > 127.0f) {
            arg1->unk30C = 127.0f;
        }
        gDLL_6_AMSFX->vtbl->set_vol(arg1->unk314, arg1->unk30C);
        arg1->unk308 = ((arg1->unk300[0] + arg1->unk300[1]) * 0.5f) / 25.0f;
        if (arg1->unk308 < 0.0f) {
            arg1->unk308 = 0.0f;
        }
        arg1->unk308 = 1.0f - arg1->unk308;
        arg1->unk308 = (arg1->unk308 * 0.2f) + 0.2f;
        arg1->unk308 += (fsin16_precise(arg1->unk326) * 0.1f);
        gDLL_6_AMSFX->vtbl->set_pitch(arg1->unk314, arg1->unk308);
        arg1->unk326 = arg1->unk326 + (gUpdateRate << 8);
        arg1->unk328 = arg1->unk328 + (gUpdateRate << 9);
    }
    var_v1 = 0;
    // @recomp: Also consider block hit touches
    sp3E = (arg1->unk0.hitsTouchBits | arg1->unk0.unk25C) & 3;
    var_a1 = sp3E & (sp3E ^ arg1->unk32D);
    if (var_a1 & 1) {
        var_v1 = (s32) ((sqrtf(SQ(arg1->unk290[0]) + SQ(arg1->unk298[0])) * 127.0f) / 0.95f);
    }
    if (var_a1 & 2) {
        if (var_v1 > ((sqrtf(SQ(arg1->unk290[1]) + SQ(arg1->unk298[1])) * 127.0f) / 0.95f)) {
            var_v1 = (s32) (f32) var_v1; // what
        } else {
            var_v1 = (s32) ((sqrtf(SQ(arg1->unk290[1]) + SQ(arg1->unk298[1])) * 127.0f) / 0.95f);
        }
    }
    if (var_v1 >= 0xB) {
        if (var_v1 >= 0x80) {
            var_v1 = 0x7F;
        }
        gDLL_6_AMSFX->vtbl->play(arg0, 0x76D, var_v1, NULL, NULL, 0, NULL);
    }
    arg1->unk32D = sp3E;
}

RECOMP_PATCH void dll_793_func_1C18(Object* self, BWlog_Data* objdata) {
    s32 i;
    s32 k;
    s32 sp120[2];
    s32 objListLength;
    Object* obj;
    Object** objList;
    f32 temp_fs0;
    f32 temp_fv0;
    f32 temp_fv0_2;
    f32 temp_fv1;
    f32 sp100;
    f32 spFC;
    f32 spF8;
    SRT spE0;
    MtxF spA0;

    for (i = 0; i < 2; i++) {
        objdata->unk2D0[i] = 0.0f;
        objdata->unk2E0[i] = 0.0f;
        sp120[i] = 0;
    }

    // grabs DFriverflow instances (and possibly more)
    objList = obj_get_all_of_type(22, &objListLength);

    for (i = 0; i < objListLength; i++) {
        obj = objList[i];
        // @recomp: Respect riverflow log filter flag (makes BWlog behave correctly in areas where DFLog was originally used)
        if (((DFriverflow_Setup*)obj->setup)->flags & 1) {
            for (k = 0; k < 2; k++) {
                temp_fv0 = obj->srt.transl.y - objdata->unk260[k].y;
                if ((temp_fv0 <= 200.0f) && (temp_fv0 >= -200.0f)) {
                    temp_fs0 = obj->srt.transl.x - objdata->unk260[k].x;
                    temp_fv0_2 = obj->srt.transl.z - objdata->unk260[k].z;
                    temp_fs0 = sqrtf(SQ(temp_fs0) + SQ(temp_fv0_2));
                    temp_fv1 = (f32) (u8)((DFriverflow_Setup*)obj->setup)->range * 1.5f;
                    if (temp_fs0 < temp_fv1) {
                        temp_fs0 = ((temp_fv1 - temp_fs0) / temp_fv1);
                        temp_fs0 *= (obj->srt.scale * 10.0f);
                        objdata->unk2D0[k] += fsin16_precise(obj->srt.yaw) * temp_fs0;
                        objdata->unk2E0[k] += fcos16_precise(obj->srt.yaw) * temp_fs0;
                        sp120[k] += 1;
                    }
                }
            }
        }
    }

    for (i = 0; i < 2; i++) {
        if (sp120[i] != 0) {
            objdata->unk2D0[i] /= sp120[i];
            objdata->unk2E0[i] /= sp120[i];
        }
    }
    
    for (i = 0; i < 2; i++) {
        spE0.yaw = arctan2_f(objdata->unk2D0[i], objdata->unk2E0[i]);
        spE0.pitch = 0;
        spE0.roll = 0;
        spE0.scale = 1.0f;
        spE0.transl.x = 0;
        spE0.transl.y = 0;
        spE0.transl.z = 0;
        matrix_from_srt_reversed(&spA0, &spE0);
        vec3_transform(&spA0, 
            objdata->unk0.waterNormalXList[i], objdata->unk0.waterNormalYList[i], objdata->unk0.waterNormalZList[i], 
            &sp100, &spFC, &spF8);
        spE0.yaw = 0;
        spE0.pitch = 0x4000 - arctan2_f(spFC, spF8);
        spE0.roll = -(0x4000 - arctan2_f(spFC, sp100));
        matrix_from_srt_reversed(&spA0, &spE0);
        vec3_transform(&spA0, 
            objdata->unk2D0[i], 0.0f, objdata->unk2E0[i], 
            &objdata->unk2D0[i], &objdata->unk2D8[i], &objdata->unk2E0[i]);
    }
}

/** Get off log in the direction of the dockpoint (helps avoid hopping off into deep water). */
RECOMP_PATCH s32 dll_793_func_C3C(Object *self) {
    SRT sp88;
    MtxF sp48;
    f32 sp44;
    f32 sp40;
    f32 sp3C;
    f32 temp;
    f32 temp2;
    BWlog_Data* objdata = self->data;

    if (objdata->unk338 != NULL) {
        sp88.yaw = self->srt.yaw + 0x4000;
        sp88.pitch = self->srt.pitch;
        sp88.roll = self->srt.roll;
        sp88.transl.x = 0.0f;
        sp88.transl.y = 0.0f;
        sp88.transl.z = 0.0f;
        sp88.scale = 1.0f;
        matrix_from_srt(&sp48, &sp88);
        vec3_transform(&sp48, 0.0f, 0.0f, 1.0f, &sp44, &sp40, &sp3C);
        temp2 = -((self->srt.transl.x * sp44) + (sp40 * self->srt.transl.y) + (sp3C * self->srt.transl.z));
        temp = (objdata->unk338->srt.transl.x * sp44) + (sp40 * objdata->unk338->srt.transl.y) + (sp3C * objdata->unk338->srt.transl.z) + temp2;
        if (temp < 0) {
            return 1;
        }
    }
    return 2;
}
