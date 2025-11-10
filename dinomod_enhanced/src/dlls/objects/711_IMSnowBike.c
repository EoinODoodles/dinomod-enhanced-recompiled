#include "modding.h"

#include "dlls/objects/214_animobj.h"
#include "recomputils.h"
#include "sys/gfx/modgfx.h"
#include "sys/objtype.h"
#include "sys/objanim.h"
#include "prevent_bss_reordering.h"

#include "recomp/dlls/objects/711_IMSnowBike_recomp.h"

typedef struct {
/*00*/ Vec3f unk0;
/*0C*/ Vec3f unkC;
/*18*/ f32 unk18; // also gravity kinda?
/*1C*/ f32 unk1C;
/*20*/ f32 unk20;
/*24*/ f32 _unk24;
/*28*/ f32 unk28; // gravity?
/*2C*/ f32 unk2C;
/*30*/ f32 unk30; // friction?
} IMSnowBike_Data_2AC;

typedef struct {
/*00*/ u8 _unk0[0xE - 0x0];
/*0E*/ s8 turnInput; // for players, this is joystick X
/*0F*/ s8 thrustInput; // for players, this is joystick Y
/*10*/ s32 unk10;
} IMSnowBike_Data_2E0;

typedef struct {
/*000*/ SRT unk0;
/*018*/ RaceStruct unk18; // racePos
/*03C*/ u8 _unk3C[0x48 - 0x3C];
/*048*/ u8 unk48;
/*049*/ u8 unk49;
/*04A*/ u8 _unk4A[0x4C - 0x4A];
/*04C*/ DLL27_Data unk4C;
/*2AC*/ IMSnowBike_Data_2AC unk2AC;
/*2E0*/ IMSnowBike_Data_2E0 unk2E0;
/*2F4*/ DLL_IModgfx *unk2F4;
/*2F8*/ DLL_IModgfx *unk2F8;
/*2FC*/ u8 _unk2FC[0x32C - 0x2FC];
/*32C*/ Vec3f unk32C[5];
/*368*/ u8 _unk368[0x380 - 0x368];
/*380*/ f32 unk380;
/*384*/ f32 unk384;
/*388*/ f32 unk388;
/*38C*/ f32 unk38C;
/*390*/ f32 unk390;
/*394*/ f32 unk394;
/*398*/ f32 unk398;
/*39C*/ f32 unk39C;
/*3A0*/ f32 unk3A0;
/*3A4*/ f32 unk3A4;
/*3A8*/ f32 unk3A8;
/*3AC*/ f32 unk3AC;
/*3B0*/ f32 unk3B0;
/*3B4*/ u32 unk3B4; // sound handle
/*3B8*/ u32 unk3B8;
/*3BC*/ u32 unk3BC;
/*3C0*/ u32 unk3C0;
/*3C4*/ s32 unk3C4;
/*3C8*/ s16 unk3C8;
/*3CA*/ s16 unk3CA;
/*3CC*/ s16 unk3CC;
/*3CE*/ s16 unk3CE;
/*3D0*/ s16 unk3D0;
/*3D2*/ s16 unk3D2; // downward tilt while in air?
/*3D4*/ u8 _unk3D4[0x3D6 - 0x3D4];
/*3D6*/ s16 unk3D6;
/*3D8*/ s16 unk3D8;
/*3DA*/ s8 unk3DA;
/*3DB*/ u8 unk3DB;
/*3DC*/ u8 unk3DC;
/*3DD*/ u8 flags;
/*3DE*/ s8 unk3DE; // state?
/*3DF*/ s8 unk3DF;
/*3E0*/ s8 unk3E0;
/*3E1*/ s8 unk3E1;
    // @recomp:
    u8 recompCounter;
    Vec3f recompPrevPos;
} IMSnowBike_Data;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 unk18;
/*19*/ u8 unk19;
/*1A*/ s16 unk1A;
/*1C*/ u8 unk1C;
/*1D*/ u8 unk1D;
/*1E*/ s16 unk1E;
} IMSnowBike_Setup;

enum IMSnowBikeFlags {
    SNOWBIKEFLAG_NONE = 0x0,
    SNOWBIKEFLAG_1 = 0x1,
    SNOWBIKEFLAG_2 = 0x2,
    SNOWBIKEFLAG_GROUNDED = 0x4, // at least one test point touching the ground
    SNOWBIKEFLAG_8 = 0x8,
    SNOWBIKEFLAG_10 = 0x10, // currently in sequence?
    SNOWBIKEFLAG_IS_CPU = 0x20 // is SharpClaw
};

extern DLL_IModgfx *_data_A8;

extern SRT _bss_8;

extern void dll_711_func_1F54(Object *self, IMSnowBike_Data *objdata, IMSnowBike_Data_2AC *arg2, f32 updateRate, s32 arg4);
extern void dll_711_func_2BA0(Object *self, IMSnowBike_Data *objdata, IMSnowBike_Data_2AC *arg2, f32 updateRate, s32 arg4);
extern void dll_711_func_3430(Object *self, IMSnowBike_Data *objdata, MtxF *a2, s32 a3, s32 a4, s32 a5);
extern void dll_711_func_3780(Object *self, IMSnowBike_Data *objdata, DLL27_Data *arg2);

RECOMP_HOOK_DLL(dll_711_setup) void dll_711_setup_hook(Object *self, IMSnowBike_Setup *setup, s32 arg2) {
    IMSnowBike_Data *objdata = (IMSnowBike_Data*)self->data;
    objdata->recompPrevPos = self->srt.transl;
    objdata->recompCounter = 0;
}

// @recomp: Remove problematic speed adjustment. The changes to dll_711_func_1F54 conflict with this.
RECOMP_PATCH void dll_711_update(Object *self) {
    IMSnowBike_Data *objdata;
    //IMSnowBike_Data_2AC *temp_v0;
    //Vec3f spBC;
    //MtxF sp7C;
    MtxF sp3C;

    objdata = self->data;
    //temp_v0 = &objdata->unk2AC;
    
    if (!(objdata->flags & SNOWBIKEFLAG_IS_CPU)) {
        // _bss_8.yaw = -objdata->unk3CC;
        // _bss_8.pitch = -objdata->unk3CE;
        // _bss_8.roll = -objdata->unk3D0;
        // matrix_from_srt_reversed(&sp7C, &_bss_8);
        // self->speed.x = (self->srt.transl.x - self->positionMirror2.x) * gUpdateRateInverseF;
        // self->speed.y = (self->srt.transl.y - self->positionMirror2.y) * gUpdateRateInverseF;
        // self->speed.z = (self->srt.transl.z - self->positionMirror2.z) * gUpdateRateInverseF;
        // spBC.f[0] = self->speed.x * 0.93749994f;
        // spBC.f[1] = self->speed.y * 0.93749994f;
        // spBC.f[2] = self->speed.z * 0.93749994f;
        // vec3_transform(&sp7C, spBC.f[0], spBC.f[1], spBC.f[2], &temp_v0->unkC.x, &temp_v0->unkC.y, &temp_v0->unkC.z);
        dll_711_func_3430(self, objdata, &sp3C, 0, 0, 0);
        vec3_transform(&sp3C, 0.0f, 0.0f, -10.0f, &objdata->unk3A8, &objdata->unk3AC, &objdata->unk3B0);
    }
}

RECOMP_PATCH u32 dll_711_get_data_size(Object *self, u32 a1) {
    return sizeof(IMSnowBike_Data);
}

RECOMP_PATCH void dll_711_func_1870(Object *self, IMSnowBike_Data *objdata, Gfx **gdl, Mtx **mtxs, Vertex **vtxs, Triangle **pols) {
    static u32 soundHandle; // bss+0x20
    Vertex *spA4;
    Gfx *spA0;
    Triangle *sp9C;
    f32 var_fv0;
    s32 pad[5];
    SRT sp6C;
    s32 sp68;
    s32 pad_sp54[3];
    IMSnowBike_Data_2AC *sp3C;
    u32 sp48[] = {0x00000006, 0x00000069, 0x00000069, 0x000000ff};
    s32 volume;

    sp3C = &objdata->unk2AC;
    spA0 = *gdl;
    spA4 = *vtxs;
    sp9C = *pols;
    var_fv0 = 0.0f;
    if (sp3C->unkC.z < 0.0f) {
        var_fv0 = sp3C->unkC.z;
    }
    sp6C.transl.z = var_fv0;
    if (sp3C->unkC.z < 0.0f) {
        var_fv0 = sp3C->unkC.x;
    }
    sp6C.transl.x = var_fv0;
    dl_set_prim_color(&spA0, 0xFF, 0xFF, 0xFF, 0xFF);
    if (sp3C->unkC.z < -0.5f) {
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_12E, &sp6C, 4, -1, NULL);
    }
    if (sp3C->unkC.z < -1.5f) {
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_12F, &sp6C, 4, -1, NULL);
    }
    if (sp3C->unkC.z < -2.1f) {
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_130, &sp6C, 4, -1, NULL);
    }
    sp68 = 0;
    if (objdata->unk2E0.thrustInput > 0) {
        // @recomp: Make spawn rate independent of framerate. Otherwise the expgfx buffer fills up at higher framerates.
        sp68 = 10 / (3.0f / gUpdateRateF);
    }
    while (sp68 != 0) {
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_131, NULL, 4, -1, NULL);
        sp68 -= 1;
    }
    sp6C.yaw = 0;
    sp6C.pitch = 0;
    sp6C.roll = 0;
    sp6C.scale = 1.0f;
    objdata->unk3D8 -= gUpdateRate;
    if (objdata->unk398 < -1.2f) {
        sp48[1] += rand_next(0, 155);
        sp48[2] += rand_next(0, 155);
        volume = (0.0f - objdata->unk398) * 21.0f;
        if ((objdata->unk3E1 & 0xF) && (objdata->unk3D8 <= 0)) {
            sp6C.transl.x = objdata->unk32C[0].x - self->positionMirror.x;
            sp6C.transl.y = objdata->unk32C[0].y - self->positionMirror.y;
            sp6C.transl.z = objdata->unk32C[0].z - self->positionMirror.z;
            _data_A8->vtbl->func0(self, 0, &sp6C, 1, -1, sp48);
            gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_292, volume, &soundHandle, NULL, 0, NULL);
            gDLL_6_AMSFX->vtbl->func_954(soundHandle, (volume / 127.0f) + 0.5f);
        } else if ((objdata->unk3E1 & 2) && (objdata->unk3D8 <= 0)) {
            sp6C.transl.x = objdata->unk32C[1].x;
            sp6C.transl.y = objdata->unk32C[1].y;
            sp6C.transl.z = objdata->unk32C[1].z;
            _data_A8->vtbl->func0(self, 0, &sp6C, 1, -1, sp48);
            gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_292, volume, &soundHandle, NULL, 0, NULL);
            gDLL_6_AMSFX->vtbl->func_954(soundHandle, (volume / 127.0f) + 0.5f);
        } else if ((objdata->unk3E1 & 4) && (objdata->unk3D8 <= 0)) {
            sp6C.transl.x = objdata->unk32C[2].x;
            sp6C.transl.y = objdata->unk32C[2].y;
            sp6C.transl.z = objdata->unk32C[2].z;
            _data_A8->vtbl->func0(self, 0, &sp6C, 1, -1, sp48);
            gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_292, volume, &soundHandle, NULL, 0, NULL);
            gDLL_6_AMSFX->vtbl->func_954(soundHandle, (volume / 127.0f) + 0.5f);
        }
    }
    if (objdata->unk3D8 <= 0) {
        objdata->unk3D8 = 0x1E;
    }
    *gdl = spA0;
    *vtxs = spA4;
    *pols = sp9C;
}

RECOMP_PATCH void dll_711_func_1F54(Object *self, IMSnowBike_Data *objdata, IMSnowBike_Data_2AC *arg2, f32 updateRate, s32 arg4) {
    MtxF sp140;
    MtxF sp100;
    MtxF spC0;
    f32 temp2;
    DLL27_Data *dll27Data;
    f32 temp3;
    f32 temp4;
    f32 temp1;
    f32 var_fa0;
    f32 spA4;
    f32 var_fv0;
    Vec3f sp94;
    f32 var_fv1;
    s32 var_v0;
    s32 pad2;
    Vec3f sp7C;
    Vec3f sp70;
    s32 temp_a2;
    s32 pad;
    s32 sp64;
    s32 var_a0;
    s32 var_v1;
    f32 temp;
    f32 sp54;
    f32 sp50;
    f32 sp4C;
    f32 sp48;

    dll27Data = &objdata->unk4C;
    if (objdata->unk3DA != 0) {
        arg2->unkC.x *= objdata->unk380;
        arg2->unkC.y *= objdata->unk380;
        arg2->unkC.z *= objdata->unk380;
        objdata->unk3DA--;
        if (objdata->unk3DA < 0) {
            objdata->unk3DA = 0;
        }
    }
    if (arg2->unkC.x > 2.0f) {
        arg2->unkC.x = 2.0f;
    }
    if (arg2->unkC.x < -2.0f) {
        arg2->unkC.x = -2.0f;
    }
    if (arg2->unkC.y > 4.0f) {
        arg2->unkC.y = 4.0f;
    }
    if (arg2->unkC.y < -4.0f) {
        arg2->unkC.y = -4.0f;
    }
    if (arg2->unkC.z > 2.0f) {
        arg2->unkC.z = 2.0f;
    }
    if (arg2->unkC.z < -4.6f) {
        arg2->unkC.z = -4.6f;
    }
    _bss_8.yaw = objdata->unk3CC;
    _bss_8.pitch = objdata->unk3CE;
    _bss_8.roll = objdata->unk3D0;
    matrix_from_srt(&sp140, &_bss_8);
    _bss_8.yaw = -objdata->unk3CC;
    _bss_8.pitch = -objdata->unk3CE;
    _bss_8.roll = -objdata->unk3D0;
    matrix_from_srt_reversed(&sp100, &_bss_8);
    if (!(objdata->flags & SNOWBIKEFLAG_GROUNDED)) {
        var_fv0 = (f32) -objdata->unk2E0.thrustInput / 60.0f;
        if (var_fv0 > 1.0f) {
            var_fv0 = 1.0f;
        } else if (var_fv0 < -1.0f) {
            var_fv0 = -1.0f;
        }
        var_fv0 *= 6144.0f;
        objdata->unk3D2 += ((s32) var_fv0 - objdata->unk3D2) >> 5;
    } else {
        if (objdata->unk3D2 != 0) {
            objdata->unk3D2 = objdata->unk3D2 - (-objdata->unk3D2 >> 5);
        }
    }
    // make down/gravity vec? relative to bike orientation
    vec3_transform(&sp100, 0.0f, arg2->unk28 * arg2->unk1C, 0.0f, &sp94.x, &sp94.y, &sp94.z);
    if (objdata->unk2E0.thrustInput < 0) {
        // joystick held back
        var_fv1 = -(f32) objdata->unk2E0.thrustInput * 4.0f;
    } else {
        // joystick held forward
        var_fv1 = -(f32) objdata->unk2E0.thrustInput * 6.0f;
    }
    // factor in thruster
    var_fv0 = arg2->unk20 * var_fv1;
    if (var_fv0 < 0.0f) {
        arg2->unkC.z += var_fv0 * 0.01666666f;
    } else if (arg2->unkC.z <= 0.0f) {
        arg2->unkC.z += var_fv0 * 0.01666666f;
        if (arg2->unkC.z > 0.0f) {
            arg2->unkC.z = 0.0f;
        }
    }
    arg2->unk0.x = sp94.f[0] * arg2->unk18;
    arg2->unk0.y = sp94.f[1] * arg2->unk18;
    arg2->unk0.z = sp94.f[2] * arg2->unk18;
    arg2->unkC.x = arg2->unk0.x + arg2->unkC.x;
    arg2->unkC.y = arg2->unk0.y + arg2->unkC.y;
    arg2->unkC.z = arg2->unk0.z + arg2->unkC.z;
    if (dll27Data->unk25D != 0) {
        var_fv0 = arg2->unk2C * sp94.y;
        if (arg2->unkC.z < 0.0f) {
            if (var_fv0 < 0.0f) {
                var_fv0 = -var_fv0;
            }
        } else if (var_fv0 > 0.0f) {
            var_fv0 = -var_fv0;
        }
        var_fv0 *= arg2->unk18;
        var_fa0 = arg2->unkC.z + var_fv0;
        if (arg2->unkC.z < 0.0f) {
            if (var_fa0 > 0.0f) {
                arg2->unkC.z = 0.0f;
            } else {
                arg2->unkC.z = var_fa0;
            }
        } else if (var_fa0 < 0.0f) {
            arg2->unkC.z = 0.0f;
        } else {
            arg2->unkC.z = var_fa0;
        }
        if (arg2->unkC.z < 0.0f) {
            var_fa0 = -arg2->unkC.z;
        } else {
            var_fa0 = arg2->unkC.z;
        }
        var_fv0 = arg2->unk2C * sp94.y * (4.0f + SQ(var_fa0));
        if (arg2->unkC.x < 0.0f) {
            if (var_fv0 < 0.0f) {
                var_fv0 = -var_fv0;
            }
        } else if (var_fv0 > 0.0f) {
            var_fv0 = -var_fv0;
        }
        var_fv0 *= arg2->unk18;
        var_fv1 = arg2->unkC.x + var_fv0;
        if (arg2->unkC.x < 0.0f) {
            if (var_fv1 > 0.0f) {
                arg2->unkC.x = 0.0f;
            } else {
                arg2->unkC.x = var_fv1;
            }
        } else if (var_fv1 < 0.0f) {
            arg2->unkC.x = 0.0f;
        } else {
            arg2->unkC.x = var_fv1;
        }
        objdata->unk3E0 = 0;
        objdata->unk3D2 = 0;
    } else {
        objdata->unk3E0 += 1;
        if (objdata->unk3E0 > 0x64) {
            objdata->unk3E0 = 0x64;
        }
    }
    temp = SQ(arg2->unkC.z);
    var_fv0 = arg2->unk30 * temp;
    if (arg2->unkC.z > 0.0f) {
        var_fv0 = -var_fv0;
    }
    var_fv0 *= arg2->unk18;
    var_fa0 = arg2->unkC.z + (var_fv0);
    if (arg2->unkC.z < 0.0f) {
        if (var_fa0 > 0.0f) {
            arg2->unkC.z = 0.0f;
        } else {
            arg2->unkC.z = var_fa0;
        }
    } else if (var_fa0 < 0.0f) {
        arg2->unkC.z = 0.0f;
    } else {
        arg2->unkC.z = var_fa0;
    }
    vec3_transform(&sp140, arg2->unkC.x, arg2->unkC.y, arg2->unkC.z, self->speed.f, &self->speed.y, &self->speed.z);
    self->speed.x *= ((1.0f / 15.0f) + 1.0f);
    self->speed.y *= ((1.0f / 15.0f) + 1.0f);
    self->speed.z *= ((1.0f / 15.0f) + 1.0f);
    // @recomp: Ensure the upcoming speed adjustment code only runs at 20hz.
    //          The following changes ensure that physics here behave exactly the same at any framerate.
    //          The speed adjustments in particular are very sensitive. By making this work the same as 20hz
    //          at any framerate, the physics of sliding down slopes will work exactly as it was tuned in
    //          the vanilla ROM.
    if (objdata->recompCounter == 0) {
        objdata->recompPrevPos = self->srt.transl;
    }
    obj_integrate_speed(self, self->speed.x, self->speed.y, self->speed.z);
    // @recomp: Change this section to run at 60hz
    //if (arg4 != 0) {
        spA4 = 1.0f / /*updateRate*/3;
        dll_711_func_3780(self, objdata, dll27Data);
        gDLL_27->vtbl->func_1E8(self, dll27Data, /*gUpdateRateF*/1);
        gDLL_27->vtbl->func_5A8(self, dll27Data);
        gDLL_27->vtbl->func_624(self, dll27Data, /*updateRate*/1);
    // @recomp: Run the following at 20hz no matter what the framerate is
    objdata->recompCounter++;
    if (objdata->recompCounter == 3) {
        objdata->recompCounter = 0;
        self->speed.x = (self->srt.transl.x - objdata->recompPrevPos.x) * spA4;
        self->speed.y = (self->srt.transl.y - objdata->recompPrevPos.y) * spA4;
        self->speed.z = (self->srt.transl.z - objdata->recompPrevPos.z) * spA4;
        sp70.f[0] = self->speed.x * (1.0f / ((1.0f / 15.0f) + 1.0f));
        sp70.f[1] = self->speed.y * (1.0f / ((1.0f / 15.0f) + 1.0f));
        sp70.f[2] = self->speed.z * (1.0f / ((1.0f / 15.0f) + 1.0f));
        vec3_transform(&sp100, sp70.f[0], sp70.f[1], sp70.f[2], &arg2->unkC.x, &arg2->unkC.y, &arg2->unkC.z);
    }
    // @recomp: Change this section to run at 60hz
        sp7C.f[0] = 0.0f;
        sp7C.f[1] = 1.0f;
        sp7C.f[2] = 0.0f;
        if (dll27Data->unk25C & 0xF) {
            objdata->flags |= SNOWBIKEFLAG_GROUNDED;
        } else {
            objdata->flags &= ~SNOWBIKEFLAG_GROUNDED;
        }
        var_a0 = 0;
        for (var_v1 = 0; var_v1 < 4; var_v1++) {
            if (dll27Data->unk25C & (1 << var_v1)) {
                sp7C.x = dll27Data->unk68.unk0[var_v1].x + sp7C.x;
                sp7C.y = dll27Data->unk68.unk0[var_v1].y + sp7C.y;
                sp7C.z = dll27Data->unk68.unk0[var_v1].z + sp7C.z;
                var_a0 += 1;
            }
        }

        sp7C.x *= 0.25f;
        sp7C.y *= 0.25f;
        sp7C.z *= 0.25f;
        if (var_a0 != 0) {
            var_fv0 = 1.0f / var_a0;
            sp7C.x *= var_fv0;
            sp7C.y *= var_fv0;
            sp7C.z *= var_fv0;
        } else {
            sp7C.x = 0.0f;
            sp7C.y = 1.0f;
            sp7C.z = 0.0f;
        }
        _bss_8.yaw = -objdata->unk3CC;
        _bss_8.pitch = 0;
        _bss_8.roll = 0;
        matrix_from_srt_reversed(&spC0, &_bss_8);
        vec3_transform(&spC0, sp7C.x, sp7C.y, sp7C.z, &sp7C.x, &sp7C.y, &sp7C.z);
        sp64 = 0x4000 - arctan2_f(sp7C.y, sp7C.z);
        var_v0 = -(0x4000 - arctan2_f(sp7C.y, sp7C.x));
        sp64 -= (objdata->unk3CE & 0xFFFF);
        CIRCLE_WRAP(sp64);
        objdata->unk3CE += (((sp64 >> 2) / 3) * (s32) /*updateRate*/1);
        self->srt.pitch = objdata->unk3CE + objdata->unk3D2;
        var_v0 -= (objdata->unk3D0 & 0xFFFF);
        CIRCLE_WRAP(var_v0);
        objdata->unk3D0 += (((var_v0 >> 2) / 3) * (s32) /*updateRate*/1);
    //}
    objdata->unk3CC -= (s16) (objdata->unk2E0.turnInput * (70.0f - ((f32) objdata->unk2E0.thrustInput * 0.05f)) * 0.0666f);
    if (objdata->flags & SNOWBIKEFLAG_8) {
        sp4C = fsin16_precise(objdata->unk3C4);
        sp48 = fcos16_precise(objdata->unk3C4);
        sp54 = fsin16_precise(objdata->unk3CC);
        sp50 = fcos16_precise(objdata->unk3CC);
        if (((sp4C * sp54) + (sp48 * sp50)) > 0.0f) {
            temp1 = -sp48 * sp54;
            temp2 = sp4C * sp50;
            temp3 = sp48 * sp54;
            temp4 = -sp4C * sp50;
            if ((temp3 + temp4) < (temp1 + temp2)) {
                var_v0 = arctan2_f(-sp48, sp4C);
            } else {
                var_v0 = arctan2_f(sp48, -sp4C);
            }
            var_v0 -= objdata->unk3CC & 0xFFFF;
            CIRCLE_WRAP(var_v0);
            objdata->unk3CC += var_v0 >> 3;
        }
    }
}

RECOMP_PATCH void dll_711_func_2BA0(Object *self, IMSnowBike_Data *objdata, IMSnowBike_Data_2AC *arg2, f32 updateRate, s32 arg4) {
    DLL27_Data *dll27Data;
    MtxF sp11C;
    MtxF spDC;
    MtxF sp9C;
    f32 temp_fa0;
    f32 temp_fv0;
    f32 temp_fv0_2;
    f32 sp8C;
    f32 var_fv1;
    Vec3f sp7C;
    Vec3f sp70;
    Vec3f sp64;
    s32 temp_a1;
    s32 var_a0;
    s32 var_s0;
    s32 var_v1;

    dll27Data = &objdata->unk4C;

    if (objdata->unk3DA != 0) {
        arg2->unkC.x *= objdata->unk380;
        arg2->unkC.y *= objdata->unk380;
        arg2->unkC.z *= objdata->unk380;
        objdata->unk3DA--;
        if (objdata->unk3DA < 0) {
            objdata->unk3DA = 0;
        }
    }
    if (arg2->unkC.x > 2.0f) {
        arg2->unkC.x = 2.0f;
    }
    if (arg2->unkC.x < -2.0f) {
        arg2->unkC.x = -2.0f;
    }
    if (arg2->unkC.y > 4.0f) {
        arg2->unkC.y = 4.0f;
    }
    if (arg2->unkC.y < -4.0f) {
        arg2->unkC.y = -4.0f;
    }
    if (arg2->unkC.z > 2.0f) {
        arg2->unkC.z = 2.0f;
    }
    if (arg2->unkC.z < -3.8f) {
        arg2->unkC.z = -3.8f;
    }
    _bss_8.yaw = objdata->unk3CC;
    _bss_8.pitch = objdata->unk3CE;
    _bss_8.roll = objdata->unk3D0;
    matrix_from_srt(&sp11C, &_bss_8);
    _bss_8.yaw = -objdata->unk3CC;
    _bss_8.pitch = -objdata->unk3CE;
    _bss_8.roll = -objdata->unk3D0;
    matrix_from_srt_reversed(&spDC, &_bss_8);
    vec3_transform(&spDC, 0.0f, arg2->unk28 * arg2->unk1C, 0.0f, &sp7C.x, &sp7C.y, &sp7C.z);
    temp_fv0 = 2.0f * -(f32) objdata->unk2E0.thrustInput;
    temp_fv0 *= arg2->unk18;
    arg2->unkC.z += temp_fv0;
    arg2->unk0.x = sp7C.f[0] * arg2->unk18;
    arg2->unk0.y = sp7C.f[1] * arg2->unk18;
    arg2->unk0.z = sp7C.f[2] * arg2->unk18;
    arg2->unkC.x = arg2->unk0.x + arg2->unkC.x;
    arg2->unkC.y = arg2->unk0.y + arg2->unkC.y;
    arg2->unkC.z = arg2->unk0.z + arg2->unkC.z;
    if (dll27Data->unk25D != 0) {
        arg2->unkC.x = 0.0f;
        var_fv1 = arg2->unk2C * sp7C.y;
        if (arg2->unkC.z < 0.0f) {
            if (var_fv1 < 0.0f) {
                var_fv1 = -var_fv1;
            }
        } else if (var_fv1 > 0.0f) {
            var_fv1 = -var_fv1;
        }
        var_fv1 *= arg2->unk18;
        temp_fv0 = arg2->unkC.z + var_fv1;
        if (temp_fv0 > 0.0f) {
            arg2->unkC.z = 0.0f;
        } else {
            arg2->unkC.z = temp_fv0;
        }
    }
    temp_fv0_2 = SQ(arg2->unkC.z);
    var_fv1 = arg2->unk30 * temp_fv0_2;
    var_fv1 *= arg2->unk18;
    temp_fv0 = arg2->unkC.z + (var_fv1);
    if (temp_fv0 > 0.0f) {
        arg2->unkC.z = 0.0f;
    } else {
        arg2->unkC.z = temp_fv0;
    }
    vec3_transform(&sp11C, arg2->unkC.x, arg2->unkC.y, arg2->unkC.z, self->speed.f, &self->speed.y, &self->speed.z);
    self->speed.x *= ((1.0f / 15.0f) + 1.0f);
    self->speed.y *= ((1.0f / 15.0f) + 1.0f);
    self->speed.z *= ((1.0f / 15.0f) + 1.0f);
    // @recomp: This is the same patch that is in dll_711_func_1F54 but for SharpClaw
    //          instead of the player.
    if (objdata->recompCounter == 0) {
        objdata->recompPrevPos = self->srt.transl;
    }
    obj_integrate_speed(self, self->speed.x, self->speed.y, self->speed.z);
    // @recomp: Ditto
    //if (arg4 != 0) {
        sp8C = (1.0f / /*updateRate*/3);
        gDLL_27->vtbl->func_1E8(self, dll27Data, /*gUpdateRateF*/1);
        gDLL_27->vtbl->func_5A8(self, dll27Data);
        gDLL_27->vtbl->func_624(self, dll27Data, /*updateRate*/1);
    // @recomp: Ditto
    objdata->recompCounter++;
    if (objdata->recompCounter == 3) {
        objdata->recompCounter = 0;
        self->speed.x = (self->srt.transl.x - objdata->recompPrevPos.x) * sp8C;
        self->speed.y = (self->srt.transl.y - objdata->recompPrevPos.y) * sp8C;
        self->speed.z = (self->srt.transl.z - objdata->recompPrevPos.z) * sp8C;
        sp64.f[0] = self->speed.x * (1.0f / ((1.0f / 15.0f) + 1.0f));
        sp64.f[1] = self->speed.y * (1.0f / ((1.0f / 15.0f) + 1.0f));
        sp64.f[2] = self->speed.z * (1.0f / ((1.0f / 15.0f) + 1.0f));
        vec3_transform(&spDC, sp64.f[0], sp64.y, sp64.f[2], &arg2->unkC.x, &arg2->unkC.y, &arg2->unkC.z);
    }
    // @recomp: Ditto
        sp70.f[0] = 0.0f;
        sp70.f[1] = 1.0f;
        sp70.f[2] = 0.0f;
        if (dll27Data->unk25C & 0xF) {
            objdata->flags |= SNOWBIKEFLAG_GROUNDED;
        } else {
            objdata->flags &= ~SNOWBIKEFLAG_GROUNDED;
        }
        var_a0 = 0;
        for (var_v1 = 0; var_v1 < 4; var_v1++) {
            if (dll27Data->unk25C & (1 << var_v1)) {
                sp70.x = dll27Data->unk68.unk0[var_v1].x + sp70.x;
                sp70.y = dll27Data->unk68.unk0[var_v1].y + sp70.y;
                sp70.z = dll27Data->unk68.unk0[var_v1].z + sp70.z;
                var_a0++;
            }
        }
        if (var_a0 != 0) {
            temp_fv0 = 1.0f / var_a0;
            sp70.x *= temp_fv0;
            sp70.y *= temp_fv0;
            sp70.z *= temp_fv0;
        } else {
            sp70.x = 0.0f;
            sp70.y = 1.0f;
            sp70.z = 0.0f;
        }
        _bss_8.yaw = -objdata->unk3CC;
        _bss_8.pitch = 0;
        _bss_8.roll = 0;
        matrix_from_srt_reversed(&sp9C, &_bss_8);
        vec3_transform(&sp9C, sp70.x, sp70.y, sp70.z, &sp70.x, &sp70.y, &sp70.z);
        var_s0 = 0x4000 - arctan2_f(sp70.y, sp70.z);
        temp_a1 = -(0x4000 - arctan2_f(sp70.y, sp70.x));
        var_s0 -= (objdata->unk3CE & 0xFFFF);
        CIRCLE_WRAP(var_s0);
        objdata->unk3CE += (((var_s0 >> 2) / 3) * (s32) /*updateRate*/1);
        self->srt.pitch = objdata->unk3CE + objdata->unk3D2;
        temp_a1 -= (objdata->unk3D0 & 0xFFFF);
        CIRCLE_WRAP(temp_a1);
        objdata->unk3D0 += (((temp_a1 >> 2) / 3) * (s32) /*updateRate*/1);
    //}
    objdata->unk3CC -= (s16) (objdata->unk2E0.turnInput * (70.0f - (objdata->unk2E0.thrustInput * 0.05f)) * 0.0666f);
}
