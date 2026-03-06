#include "modding.h"

#include "common.h"
#include "recomputils.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/objanim.h"
#include "dlls/objects/common/sidekick.h"

extern void func_80026160(Object* obj);

#include "recomp/dlls/objects/461_CCsandwormBoss_recomp.h"

typedef struct {
    u8 unk0;
    u8 unk1;
    u8 unk2;
    u8 unk3;
    Object* unk4;
    Object* unk8;
    Object *unkC;
    Object *unk10;
    f32 unk14;
    f32 unk18;
    u8 _unk1C[0x20 - 0x1C];
} DLL461_Data;

extern u32 _data_C[];

extern void dll_461_func_1030(Object* a0, void* a1);
extern void dll_461_func_1090(Object* a0, void* a1, void* a2, s32 a3);
extern void dll_461_func_1250(Object* a0, void* a1);
extern void dll_461_func_12B0(Object* a0, void* a1);
extern void dll_461_func_1384(Object* a0, Vec3f* a1, f32 a2);
extern void dll_461_func_14B0(Object* a0, void* a1);
extern void dll_461_func_1540(Object* a0, void* a1);

RECOMP_PATCH void dll_461_func_5E0(Object* arg0, DLL461_Data* arg1) {
    ObjSetup* sp4C;
    f32 sp48;
  
    sp4C = arg0->setup;
    arg1->unk18 += gUpdateRateF;
    sp48 = 3.4028235e38f;
    obj_get_nearest_type_to(0x12, arg0, &sp48);

    // @recomp: Bail if player pointer isn't setup
    //          (original patch by MusicalProgrammer)
    if (arg1->unk4 == NULL) {
        return;
    }

    diPrintf("worm %d, barrel %d\n", (s32) vec3_distance_xz(&arg0->positionMirror, &arg1->unk4->positionMirror), (s32) sp48);
    switch (arg1->unk0) {
    case 4:
        dll_461_func_12B0(arg0, arg1->unk4);
        if (vec3_distance_xz_squared(&arg0->positionMirror, &arg1->unk4->positionMirror) < 32400.0f) {
            dll_461_func_1090(arg0, arg1->unk4, arg1, 5);
        } else {
            if (((DLL_ISidekick*)arg1->unk8->dll)->vtbl->func24(arg1->unk8) != 0) {
                diPrintf("kyte dist %d interest range 50.0F\n", (s32) vec3_distance_xz(&arg0->positionMirror, &arg1->unk8->positionMirror));
                if (vec3_distance_xz_squared(&arg0->positionMirror, &arg1->unk8->positionMirror) < 3600.0f) {
                    dll_461_func_1250(arg0, arg1);
                    arg1->unk18 = 0.0f;
                }
            }
        }
        break;
    case 5:
        dll_461_func_12B0(arg0, arg1->unk4);
        if (arg0->animProgress > 0.95f) {
            dll_461_func_1030(arg0, arg1);
        }
        break;
    case 6:
        dll_461_func_12B0(arg0, arg1->unk4);
        if (arg0->animProgress > 0.95f) {
            dll_461_func_1250(arg0, arg1);
        }
        break;
    case 7:
        dll_461_func_12B0(arg0, arg1->unk8);
        if (arg0->animProgress > 0.95f) {
            dll_461_func_1250(arg0, arg1);
        }
        break;
    case 8:
        dll_461_func_12B0(arg0, arg1->unk4);
        if (arg0->animProgress > 0.95f) {
            arg1->unk0 = 0xD;
            arg1->unk14 = 0.005f;
            func_80023D30(arg0, 5, 0, 0);
        }
        dll_461_func_14B0(arg0, arg1);
        break;
    case 9:
        dll_461_func_12B0(arg0, arg1->unk8);
        if (arg1->unk18 > 300.0f) {
            main_set_bits(0x46E, 0x65U);
        } else {
            main_set_bits(0x46E, 0xC3U);
        }
        if (vec3_distance_xz_squared(&arg0->positionMirror, &arg1->unk4->positionMirror) < 32400.0f) {
            dll_461_func_1090(arg0, arg1->unk4, arg1, 6);
        } else if (vec3_distance_xz_squared(&arg0->positionMirror, &arg1->unk8->positionMirror) < 32400.0f) {
            dll_461_func_1090(arg0, arg1->unk8, arg1, 7);
        } else {
            if ((((DLL_ISidekick*)arg1->unk8->dll)->vtbl->func24(arg1->unk8) != 0) || (vec3_distance_xz_squared(&arg0->positionMirror, &arg1->unk8->positionMirror) < 90000.0f)) {
                func_8002493C(arg0, 1.5f, &arg1->unk14);
                dll_461_func_1384(arg0, &arg1->unk8->srt.transl, 1.5f);
            } else if (vec3_distance_xz_squared(&arg0->positionMirror, (Vec3f* ) &sp4C->x) < 10000.0f) {
                dll_461_func_1030(arg0, arg1);
            } else {
                arg1->unk0 = 0xA;
                arg1->unk14 = 0.01f;
                func_80023D30(arg0, 2, 0, 0);
            }
        }
        break;
    case 10:
        if (arg0->animProgress > 0.95f) {
            arg1->unk0 = 0xB;
        }
        break;
    case 11:
        if (((s32) sp4C->x == (s32) arg0->srt.transl.f[0]) && ((s32) sp4C->z == (s32) arg0->srt.transl.f[2])) {
            sp48 = vec3_distance_xz_squared(&arg0->positionMirror, &arg1->unk4->positionMirror);
            if (sp48 < 2500.0f) {
                arg1->unk3 = 0;
                gDLL_3_Animation->vtbl->func17(8, arg1->unkC, -1);
            } else if (sp48 < 32400.0f) {
                arg1->unk0 = 0xC;
                arg1->unk14 = 0.005f;
                func_80023D30(arg0, 8, 0, 0);
                gDLL_6_AMSFX->vtbl->play_sound(arg0, (u16)_data_C[rand_next(0, 3)], 0x7FU, NULL, NULL, 0, NULL);
                arg1->unk2 = 0;
                arg1->unk1 = 3;
            } else {
                sp48 = 50.0f;
                arg1->unk10 = obj_get_nearest_type_to(0x12, arg0, &sp48);
                if ((arg1->unk10 != NULL) && (gDLL_54->vtbl->func4.withOneArgS32((s32)arg1->unk10->data) == 0)) {
                    arg1->unk0 = 0xD;
                    arg1->unk3 = 0;
                    arg1->unk18 = 0.0f;
                    gDLL_3_Animation->vtbl->func17(5, arg1->unkC, -1);
                } else {
                    arg1->unk0 = 0xC;
                    arg1->unk14 = 0.01f;
                    func_80023D30(arg0, 0xB, 0, 0);
                    arg1->unk2 = 0;
                    arg1->unk1 = 3;
                }
            }
        } else {
            dll_461_func_1384(arg0, (Vec3f* ) &sp4C->x, 3.0f);
            if (vec3_distance_xz_squared(&arg0->positionMirror, (Vec3f* ) &sp4C->x) < 10000.0f) {
                arg1->unk2 = 1;
            }
        }
        break;
    case 12:
        dll_461_func_12B0(arg0, arg1->unk4);
        if (arg0->animProgress > 0.95f) {
            dll_461_func_1030(arg0, arg1);
        }
        break;
    case 13:
        if (arg1->unk10 != NULL) {
            sp4C = arg1->unk10->setup;
            arg1->unk10->srt.transl.f[0] = sp4C->x;
            arg1->unk10->srt.transl.f[1] = sp4C->y;
            arg1->unk10->srt.transl.f[2] = sp4C->z;
            arg1->unk10 = NULL;
        }
        if (arg1->unk18 > 3000.0f) {
            arg1->unk0 = 4;
        } else if (vec3_distance_xz_squared(&arg0->positionMirror, &arg1->unk4->positionMirror) < 32400.0f) {
            dll_461_func_1090(arg0, arg1->unk4, arg1, 8);
        }
        dll_461_func_14B0(arg0, arg1);
        break;
    case 14:
        arg1->unk0 = 0xF;
        arg0->srt.flags |= 0x4000;
        func_800267A4(arg0);
        func_80026160(arg0);
        main_set_bits(0x3FB, 1U);
        break;
    case 15:
        return;
    }

    func_80024108(arg0, arg1->unk14, gUpdateRateF, 0);
    dll_461_func_1540(arg0, arg1);
}
