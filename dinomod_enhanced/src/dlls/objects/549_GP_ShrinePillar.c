#include "modding.h"

#include "dlls/objects/214_animobj.h"
#include "sys/objanim.h"
#include "segment_334F0.h"

#include "recomp/dlls/objects/549_GP_ShrinePillar_recomp.h"

typedef struct {
    ObjSetup base;
    s16 unk18;
    s16 unk1A;
    s16 unk1C;
    s8 unk1E;
    u8 unk1F;
    u8 unk20;
    s16 unk22;
} GP_ShrinePillar_Setup;

typedef struct {
    u8 unk0;
    u8 unk1;
    f32 unk4;
} GP_ShrinePillar_Data;

static s32 counter;

RECOMP_HOOK_DLL(dll_549_ctor) void dll_549_ctor_hook() {
    counter = 0;
}

RECOMP_PATCH int dll_549_func_264(Object* a0, Object* a1, AnimObj_Data* a2, void* a3) {
    GP_ShrinePillar_Setup* setup;
    GP_ShrinePillar_Data* objdata;
    s32* temp_v0_2;
    s32* temp_v0;
    s32 var_v1_3;
    s32 var_v0_2;
    s32 var_v1_4;

    objdata = (GP_ShrinePillar_Data*)a0->data;
    setup = (GP_ShrinePillar_Setup*)a0->setup;
    diPrintf("override %d\n", objdata->unk0);
    diPrintf("counter: %d\n", counter);
    switch (objdata->unk0) {
    case 0:
        if (main_get_bits(setup->unk18) != 0) {
            objdata->unk0 = 1;
        }
        break;
    case 1:
        for (var_v0_2 = 0; var_v0_2 < a2->unk98; var_v0_2++) {
            if (a2->unk8E[var_v0_2] == 1) {
                objdata->unk0 = 6;
                if (setup->unk1A != -1) {
                    main_set_bits(setup->unk1A, 1);
                }
            }
        }
        break;
    case 6:
        if (main_get_bits(setup->unk22) != 0) {
            objdata->unk0 = 7;
        }
        break;
    case 7:
        for (var_v0_2 = 0; var_v0_2 < a2->unk98; var_v0_2++) {
            if (a2->unk8E[var_v0_2] == 2) {
                objdata->unk0 = 2;
            }
        }
        break;
    case 2:
        if (func_80025F40(a0, NULL, NULL, NULL) == 0x19) {
            objdata->unk0 = 4;
        }
        break;
    case 3:
        objdata->unk4 -= delayFloat;
        if (objdata->unk4 <= 0.0f) {
            objdata->unk0 = 5;
        }
        break;
    case 4:
        temp_v0 = func_800348A0(a0, 0, 0);
        if (temp_v0 != NULL) {
            var_v1_3 = *temp_v0 + (delayByte * 8);
            if (var_v1_3 > 0x100) {
                var_v1_3 = 0x100;
                objdata->unk0 = 3;
                objdata->unk4 = 800.0f; // TODO: change to 1200.0f
            }
            *temp_v0 = var_v1_3;
            counter++; // @recomp
            if (counter == 3) {
                main_set_bits(0x7D0, 1);
            }
        }
        break;
    case 5:
        temp_v0_2 = func_800348A0(a0, 0, 0);
        if (temp_v0_2 != NULL) {
            var_v1_4 = *temp_v0_2 - (delayByte * 8);
            if (var_v1_4 < 0) {
                var_v1_4 = 0;
                objdata->unk0 = 2;
            }
            *temp_v0_2 = var_v1_4;
            counter--; // @recomp
        }
        break;
    }
    if ((objdata->unk0 == 1) || (objdata->unk0 == 7) || (objdata->unk0 == 8)) {
        return 0;
    } else {
        return 1;
    }
}
