#include "modding.h"
#include "recompconfig.h"

#include "dlls/objects/210_player.h"
#include "sys/controller.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "dll.h"
#include "functions.h"

enum RecompLogAButtonMode {
    RECOMP_LOG_ROWING_TAP,
    RECOMP_LOG_ROWING_HOLD
};

static int dinomod_log_can_hold_a(void) {
    return recomp_get_config_u32("log_a_button") == RECOMP_LOG_ROWING_HOLD;
}

#include "recomp/dlls/_asm/793_recomp.h"

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
    u8 startRotation;
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

RECOMP_PATCH void dll_793_setup(Object *self, BWlog_Setup *setup, s32 arg2) {
    BWlog_Data *objdata = (BWlog_Data*)self->data;
    s32 i;

    gDLL_27->vtbl->init(&objdata->unk0, 
        DLL27FLAG_NONE, 
        DLL27FLAG_8000000 | DLL27FLAG_40000 | DLL27FLAG_20 | DLL27FLAG_2 | DLL27FLAG_1, 
        DLL27MODE_1);
    gDLL_27->vtbl->setup_terrain_collider(&objdata->unk0, 2, _data_0, _data_18, _data_20);
    // @recomp: Add block hits collider
    gDLL_27->vtbl->setup_hits_collider(&objdata->unk0, 2, _data_0, _data_18, 8);
    objdata->unk0.boundsYExtension = 100;
    obj_add_object_type(self, OBJTYPE_11);
    objdata->unk31C[1] = 0x2000;
    objdata->unk2B4 = 15.0f;
    
    self->ptr0x64->flags |= 0x40A50;
    self->ptr0x64->unk3C = 0xFF;
    self->ptr0x64->unk3D = 0xFF;
    self->ptr0x64->unk3E = 0xFF;
    self->ptr0x64->unk3F = 0x7F;

    for (i = 0; i < 2; i++) {
        objdata->unk260[i].x = self->srt.transl.x;
        objdata->unk260[i].y = self->srt.transl.y;
        objdata->unk260[i].z = self->srt.transl.z;
    }

    // @recomp: Support start yaw via setup
    self->srt.yaw = setup->startRotation << 8;
}

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

    objdata = (BWlog_Data*)self->data;
    sp98 = 10000.0f;
    objdata->unk338 = obj_get_nearest_type_to(OBJTYPE_23, self, &sp98);
    if (objdata->unk338 != NULL) {
        temp_s0 = (ObjType23Setup*)objdata->unk338->setup;
        sp98 = vec3_distance(&self->positionMirror, &objdata->unk338->positionMirror);
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
        // @recomp: Make turning framerate independent
        self->srt.yaw -= (s32) ((f32) objdata->unk322 * (60.0f - ((f32) objdata->unk324 * 0.05f)) * 0.1f * gUpdateRateF) & 0xFFFF & 0xFFFF;
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
        sp184[0] *= -0.5f;
        sp184[2] *= 0.5f;
        objdata->unk310 = sqrtf(SQ(sp184[2]) + SQ(sp184[0]));
        sp184[2] += objdata->unk2FC;
        objdata->unk290[i] += ((sp184[2] - objdata->unk290[i]) * gUpdateRateF * 0.1f);
        if (objdata->unk32A == 0) {
            objdata->unk298[i] += ((sp184[0] - objdata->unk298[i]) * gUpdateRateF * 0.1f);
        }
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
    gDLL_27->vtbl->func_1e8(self, &objdata->unk0, gUpdateRateF);
    gDLL_27->vtbl->func_5a8(self, &objdata->unk0);
    gDLL_27->vtbl->func_624(self, &objdata->unk0, gUpdateRateF);
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
    if (floor <= -100000.0f) {
        floor = objdata->unk0.floorYList[arg2];
    }

    temp = objdata->unk2B4 + floor;
    if ((objdata->unk260[arg2].y + 30.0f) < temp) {
        diPrintf("Water too high\n");
    }
    temp += (fsin16_precise(objdata->unk31C[arg2]) * 1.5f);
    objdata->unk31C[arg2] += (gUpdateRateF * 512.0f);
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
            gDLL_6_AMSFX->vtbl->play_sound(self, 0xA75, (u8) var_fv0, NULL, NULL, 0, NULL);
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
            gDLL_6_AMSFX->vtbl->play_sound(self, 0xA74, (u8) var_fv0, NULL, NULL, 0, NULL);
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

    objdata->unk320 = get_masked_button_presses(0);
    objdata->unk322 = get_joystick_x(0);
    objdata->unk324 = get_joystick_y(0);
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
    } else if ((objdata->unk320 & A_BUTTON) || (dinomod_log_can_hold_a() && get_masked_buttons(0) & A_BUTTON)) {
        objdata->unk2B8 = 30.0f;
    }
}

RECOMP_PATCH void dll_793_func_2020(Object* arg0, BWlog_Data* arg1) {
    u8 var_a1;
    u8 sp3E;
    s32 var_v1;

    if (arg1->unk314 == 0) {
        gDLL_6_AMSFX->vtbl->play_sound(arg0, 0xA77, 0x7F, &arg1->unk314, NULL, 0, NULL);
    } else {
        arg1->unk30C = arg1->unk310 * 127.0f;
        arg1->unk30C += fsin16_precise(arg1->unk328) * 30.0f;
        if (arg1->unk30C < 30.0f) {
            arg1->unk30C = 30.0f;
        } else if (arg1->unk30C > 127.0f) {
            arg1->unk30C = 127.0f;
        }
        gDLL_6_AMSFX->vtbl->func_860(arg1->unk314, arg1->unk30C);
        arg1->unk308 = ((arg1->unk300[0] + arg1->unk300[1]) * 0.5f) / 25.0f;
        if (arg1->unk308 < 0.0f) {
            arg1->unk308 = 0.0f;
        }
        arg1->unk308 = 1.0f - arg1->unk308;
        arg1->unk308 = (arg1->unk308 * 0.2f) + 0.2f;
        arg1->unk308 += (fsin16_precise(arg1->unk326) * 0.1f);
        gDLL_6_AMSFX->vtbl->func_954(arg1->unk314, arg1->unk308);
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
        gDLL_6_AMSFX->vtbl->play_sound(arg0, 0x76D, var_v1, NULL, NULL, 0, NULL);
    }
    arg1->unk32D = sp3E;
}
