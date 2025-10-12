#include "modding.h"

#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objanim.h"
#include "sys/objtype.h"
#include "dll.h"

#include "recomp/dlls/objects/712_DIMSnowHorn1_recomp.h"
#include "sys/print.h"

extern s16 _data_80[];
extern f32 _data_84[];

typedef struct {
    s32 unk0;
    s8 unk4[0x272 - 0x4];
    s8 unk272;
    f32 unk274;
    f32 unk278;
    f32 unk27C;
    s8 unk280[0x28C - 0x280];
    f32 unk28C;
    f32 unk290;
    f32 unk294;
    s8 unk298[0x2B0 - 0x298];
    f32 unk2B0;
    s8 unk2B4[0x310 - 0x2B4];
    s32 unk310;
    s8 unk314[0x328 - 0x314];
    s16 unk328;
    s16 unk32A;
    s8 unk32C[0x33A - 0x32C];
    s8 unk33A;
    s8 unk33B[0x900 - 0x33B];
    s16 unk900;
    s16 unk902;
    s16 unk904;
    s16 unk906;
    s16 unk908;
    s16 unk90A;
} DIMSnowHorn_Data;

// Prevent a softlock that occurs when the SnowHorn runs out of energy (originally by MusicalProgrammer)
RECOMP_PATCH s32 dll_712_func_2EEC(Object* self, DIMSnowHorn_Data* objData, f32 arg2) {
    u8 one;
    DIMSnowHorn_Data *objData2 = self->data;
    f32 temp_fv0;
    f32 var_fa0;
    f32 var_fv1;
    f32 animProgress;
    s16 curModAnimId;
    f32 new_var2;
    s32 animCondition1;
    s32 returnValue;
    s32 animCondition2;
    s32 animIndex;
    f32 *temp_v0;

    objData->unk0 |= 0x200000;

    if (objData->unk272){
        self->srt.yaw += objData->unk32A * 0xB6;
        objData->unk328 = 0;
        objData->unk32A = 0;
    }
    if (objData->unk290 < 0.05f){
        objData->unk290 = 0.0f;
        objData->unk328 = 0;
        objData->unk32A = 0;
    }

    if (objData->unk328 < 90){
        self->srt.yaw += ((objData->unk32A * arg2) / 36.0f) * 182.0f;
    } else {
        return 9;
    }

    var_fa0 = objData->unk290;
    if (objData->unk290 < 0.0f){
        var_fa0 = 0.0f;
    }
    if (var_fa0 > 1.0f){
        var_fa0 = 1.0f;
    }

    //Handle SnowHorn running out of energy 
    //(@bug: causes a softlock since there's no fail state cutscene to reset you back to before the blizzard)
    //@recomp: allow SnowHorn to continue moving when exhausted
    if (objData2->unk900 == 0){
        // var_fa0 = 0;
    }

    var_fv1 = var_fa0 * 0.85f;
    if (var_fv1 < 0){
        var_fv1 = 0;
    }

    objData->unk28C += ((var_fv1 - objData->unk28C) / objData->unk2B0) * arg2;
    if (self->srt.pitch > 0){
        var_fv1 -= fsin16_precise(self->srt.pitch) * 0.3f;
    } else {
        var_fv1 -= fsin16_precise(self->srt.pitch) * 0.15f;
    }
    if (var_fv1 < _data_84[2]){
        var_fv1 = _data_84[2];
    }

    objData->unk278 += ((var_fv1 - objData->unk278) / objData->unk2B0) * arg2;

    animCondition1 = 0;
    one = 1;

    animProgress = self->animProgress;

    for (animIndex = 0; self->curModAnimId != _data_80[animIndex] && animIndex < 2; animIndex++);

    if (animIndex >= 2){
        animIndex = 0;
    }

    if (self->curModAnimId == 0x208){
        animIndex = 1;
    }

    temp_v0 = &_data_84[animIndex * 2];
    if (objData->unk28C < temp_v0[0]){
        animCondition1 = 1;
        if (animIndex == 1){
            return 9;
        }
        animIndex -= one;
    } else if (temp_v0[1] <= objData->unk28C){
        animCondition1 = 1;
        if (animIndex == 0){
            animProgress = 0.0f;
        }
        animIndex++;
    }

    animCondition2 = 1;
    if (objData->unk33A && self->curModAnimId == 0x208){
        animCondition1 = 1;
        animCondition2 = 0;
    }

    if (animCondition1){
        if ((animIndex == 1) && animCondition2){
            func_80023D30(self, 0x208, animProgress, 0);
        } else {
            func_80023D30(self, _data_80[animIndex], animProgress, 0);
        }
    }
    func_8002493C(self, objData->unk278, &objData->unk298);

    if (objData->unk310 & 0x8000){
        return 0xD;
    } else {
        return 0;
    }
}
