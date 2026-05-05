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
    Object* unk4; // krystal
    Object* unk8; // kyte
    Object *unkC;
    Object *unk10;
    f32 unk14;
    f32 unk18;
    u8 _unk1C[0x20 - 0x1C];
} CCsandwormBoss_Data;

extern u32 _data_C[];

extern void dll_461_func_1030(Object* a0, void* a1);
extern void dll_461_func_1090(Object* a0, void* a1, void* a2, s32 a3);
extern void dll_461_func_1250(Object* a0, void* a1);
extern void dll_461_func_12B0(Object* a0, void* a1);
extern void dll_461_func_1384(Object* a0, Vec3f* a1, f32 a2);
extern void dll_461_func_14B0(Object* a0, void* a1);
extern void dll_461_func_1540(Object* a0, void* a1);

RECOMP_PATCH void dll_461_func_5E0(Object *self, CCsandwormBoss_Data *objdata) {
    ObjSetup* setup;
    f32 dist;
  
    setup = self->setup;
    objdata->unk18 += gUpdateRateF;
    dist = 3.4028235e38f;
    obj_get_nearest_type_to(0x12, self, &dist);

    // @recomp: Bail if player pointer isn't setup
    //          (original patch by MusicalProgrammer)
    if (objdata->unk4 == NULL) {
        return;
    }

    diPrintf("worm %d, barrel %d\n", (s32) vec3_distance_xz(&self->globalPosition, &objdata->unk4->globalPosition), (s32) dist);
    switch (objdata->unk0) {
    case 4:
        dll_461_func_12B0(self, objdata->unk4);
        if (vec3_distance_xz_squared(&self->globalPosition, &objdata->unk4->globalPosition) < 32400.0f) {
            dll_461_func_1090(self, objdata->unk4, objdata, 5);
        } else {
            if (((DLL_ISidekick*)objdata->unk8->dll)->vtbl->func24(objdata->unk8) != 0) {
                diPrintf("kyte dist %d interest range 50.0F\n", (s32) vec3_distance_xz(&self->globalPosition, &objdata->unk8->globalPosition));
                if (vec3_distance_xz_squared(&self->globalPosition, &objdata->unk8->globalPosition) < 3600.0f) {
                    dll_461_func_1250(self, objdata);
                    objdata->unk18 = 0.0f;
                }
            }
        }
        break;
    case 5:
        dll_461_func_12B0(self, objdata->unk4);
        if (self->animProgress > 0.95f) {
            dll_461_func_1030(self, objdata);
        }
        break;
    case 6:
        dll_461_func_12B0(self, objdata->unk4);
        if (self->animProgress > 0.95f) {
            dll_461_func_1250(self, objdata);
        }
        break;
    case 7:
        dll_461_func_12B0(self, objdata->unk8);
        if (self->animProgress > 0.95f) {
            dll_461_func_1250(self, objdata);
        }
        break;
    case 8:
        dll_461_func_12B0(self, objdata->unk4);
        if (self->animProgress > 0.95f) {
            objdata->unk0 = 0xD;
            objdata->unk14 = 0.005f;
            func_80023D30(self, 5, 0, 0);
        }
        dll_461_func_14B0(self, objdata);
        break;
    case 9:
        dll_461_func_12B0(self, objdata->unk8);
        if (objdata->unk18 > 300.0f) {
            main_set_bits(0x46E, 0x65U);
        } else {
            main_set_bits(0x46E, 0xC3U);
        }
        if (vec3_distance_xz_squared(&self->globalPosition, &objdata->unk4->globalPosition) < 32400.0f) {
            dll_461_func_1090(self, objdata->unk4, objdata, 6);
        } else if (vec3_distance_xz_squared(&self->globalPosition, &objdata->unk8->globalPosition) < 32400.0f) {
            dll_461_func_1090(self, objdata->unk8, objdata, 7);
        } else {
            if ((((DLL_ISidekick*)objdata->unk8->dll)->vtbl->func24(objdata->unk8) != 0) || (vec3_distance_xz_squared(&self->globalPosition, &objdata->unk8->globalPosition) < 90000.0f)) {
                func_8002493C(self, 1.5f, &objdata->unk14);
                dll_461_func_1384(self, &objdata->unk8->srt.transl, 1.5f);
            } else if (vec3_distance_xz_squared(&self->globalPosition, (Vec3f* ) &setup->x) < 10000.0f) {
                dll_461_func_1030(self, objdata);
            } else {
                objdata->unk0 = 0xA;
                objdata->unk14 = 0.01f;
                func_80023D30(self, 2, 0, 0);
            }
        }
        break;
    case 10:
        if (self->animProgress > 0.95f) {
            objdata->unk0 = 0xB;
        }
        break;
    case 11:
        if (((s32) setup->x == (s32) self->srt.transl.f[0]) && ((s32) setup->z == (s32) self->srt.transl.f[2])) {
            dist = vec3_distance_xz_squared(&self->globalPosition, &objdata->unk4->globalPosition);
            if (dist < 2500.0f) {
                objdata->unk3 = 0;
                gDLL_3_Animation->vtbl->func17(8, objdata->unkC, -1);
            } else if (dist < 32400.0f) {
                objdata->unk0 = 0xC;
                objdata->unk14 = 0.005f;
                func_80023D30(self, 8, 0, 0);
                gDLL_6_AMSFX->vtbl->play(self, (u16)_data_C[rand_next(0, 3)], 0x7FU, NULL, NULL, 0, NULL);
                objdata->unk2 = 0;
                objdata->unk1 = 3;
            } else {
                dist = 50.0f;
                objdata->unk10 = obj_get_nearest_type_to(0x12, self, &dist);
                if ((objdata->unk10 != NULL) && (gDLL_54->vtbl->func4.withOneArgS32((s32)objdata->unk10->data) == 0)) {
                    objdata->unk0 = 0xD;
                    objdata->unk3 = 0;
                    objdata->unk18 = 0.0f;
                    gDLL_3_Animation->vtbl->func17(5, objdata->unkC, -1);
                } else {
                    objdata->unk0 = 0xC;
                    objdata->unk14 = 0.01f;
                    func_80023D30(self, 0xB, 0, 0);
                    objdata->unk2 = 0;
                    objdata->unk1 = 3;
                }
            }
        } else {
            dll_461_func_1384(self, (Vec3f* ) &setup->x, 3.0f);
            if (vec3_distance_xz_squared(&self->globalPosition, (Vec3f* ) &setup->x) < 10000.0f) {
                objdata->unk2 = 1;
            }
        }
        break;
    case 12:
        dll_461_func_12B0(self, objdata->unk4);
        if (self->animProgress > 0.95f) {
            dll_461_func_1030(self, objdata);
        }
        break;
    case 13:
        if (objdata->unk10 != NULL) {
            setup = objdata->unk10->setup;
            objdata->unk10->srt.transl.f[0] = setup->x;
            objdata->unk10->srt.transl.f[1] = setup->y;
            objdata->unk10->srt.transl.f[2] = setup->z;
            objdata->unk10 = NULL;
        }
        if (objdata->unk18 > 3000.0f) {
            objdata->unk0 = 4;
        } else if (vec3_distance_xz_squared(&self->globalPosition, &objdata->unk4->globalPosition) < 32400.0f) {
            dll_461_func_1090(self, objdata->unk4, objdata, 8);
        }
        dll_461_func_14B0(self, objdata);
        break;
    case 14:
        objdata->unk0 = 0xF;
        self->srt.flags |= OBJFLAG_INVISIBLE;
        func_800267A4(self);
        func_80026160(self);
        main_set_bits(0x3FB, 1);
        break;
    case 15:
        return;
    }

    func_80024108(self, objdata->unk14, gUpdateRateF, 0);
    dll_461_func_1540(self, objdata);
}
