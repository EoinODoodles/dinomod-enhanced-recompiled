#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "sys/objtype.h"
#include "game/gamebits.h"

#include "recomp/dlls/objects/737_DRLavaControl_recomp.h"

typedef struct {
/*0*/ Object* lfxEmitter;
/*4*/ s16 lfxEmitterVal;
/*6*/ s8 freezeTimer;
/*7*/ s8 freezeDuration;
/*8*/ s8 flags;
/*9*/ s8 effectIndex; //used to switch between different effect presets during Ice Spell cooldown
//@recomp: extending struct
/*C*/ u32 soundHandleHiss;    
/*10*/ u32 soundHandleCrackle;
} Extended_DRLavaControl_Data;

typedef struct {
/*00*/    ObjSetup base;
/*18*/    s8 unused18;
/*18*/    s8 dataIndex;
/*1A*/    s16 freezeDuration;   //how long it takes to freeze with Ice Blast spell
/*1C*/    s16 unused1C;
/*1E*/    s16 gameBitFrozen;    //gamebit to set when fully cooled with Ice Blast
} DRLavaControl_Setup;

extern s16 lfxEmitterUnk1E[];

extern void DRLavaControl_freeze_update_effects(Object* self, Extended_DRLavaControl_Data* objData, s32 effectIndex);
extern Object* DRLavaControl_create_light(Object* self, s32 lfxSetupUnk1E);

// Fix DRLavaControl LFXEmitters from crashing after you leave section, after cooling lava (originally by MusicalProgrammer)
RECOMP_PATCH void DRLavaControl_freeze_update_effects(Object* self, Extended_DRLavaControl_Data* objData, s32 effectIndex) {
    f32 distance;
    f32 temperature_tValue; //0.0 when cooled, 1.0 when hot
    u16 dataUnused[] = { 288, 287, 286, 282 }; //unused indices?
    Object* waveAnimator;
    
    if (objData->freezeTimer == 0) {
        temperature_tValue = 0.0f;
    } else {
        temperature_tValue = (f32) objData->freezeTimer / (f32) objData->freezeDuration;
    }
    
    distance = 500.0f;
    waveAnimator = obj_get_nearest_type_to(0x1D, self, &distance);
    if (waveAnimator) {
        // diPrintf(" WAVE OBJ %x ", waveAnimator);
        ((DLL_Unknown*)waveAnimator->dll)->vtbl->func[7].withOneS32OneF32((s32)waveAnimator, temperature_tValue); //blending wave amplitude to 0 as lava cools?
        ((DLL_Unknown*)waveAnimator->dll)->vtbl->func[8].withTwoArgs((s32)waveAnimator, 1);
        ((DLL_Unknown*)waveAnimator->dll)->vtbl->func[9].withFourArgs((s32)waveAnimator, 0xB3, 0x18, 0x18); //reddish RGB colour?
    }
    
    if (effectIndex != objData->effectIndex) {
        if (objData->lfxEmitter) {
            obj_destroy_object(objData->lfxEmitter);
            objData->lfxEmitter = NULL; //@recomp: prevent crash
        }
        if (effectIndex) {
            // diPrintf(" Creating LIGHT %i  x %f z %f \n", arg2, self->srt.transl.x, self->srt.transl.z);
            objData->lfxEmitter = DRLavaControl_create_light(self, (objData->lfxEmitterVal - effectIndex) + 3);
        }
        objData->effectIndex = effectIndex;
    }
}

RECOMP_PATCH void DRLavaControl_freeze(Object* self) {
    Extended_DRLavaControl_Data* objData;
    DRLavaControl_Setup* objSetup;

    objData = self->data;
    if (objData->flags & 1) {
        return;
    }

    objSetup = (DRLavaControl_Setup*)self->setup;

    if (objData->freezeTimer > 0) {
        if (!objData->soundHandleHiss){ //@recomp: use soundHandle
            gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_80C_Steam_Hissing, MAX_VOLUME, &objData->soundHandleHiss, NULL, 0, NULL);
        }
        gDLL_17->vtbl->func1(self, 0x5A, NULL, 2, -1, NULL);
        gDLL_17->vtbl->func1(self, 0x5B, NULL, 2, -1, NULL);
        objData->freezeTimer -= gUpdateRate; //@recomp: framerate independent freezing (Banjeoin)

        if (objData->freezeTimer <= 0) {
            objData->freezeTimer = 0;
            if (!objData->soundHandleCrackle){ //@recomp: use soundHandle
                gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_80B_Crackling_Freezing, MAX_VOLUME, &objData->soundHandleCrackle, NULL, 0, NULL);
            }
            objData->flags |= 1;
            main_set_bits(objSetup->gameBitFrozen, TRUE);
            // diPrintf(" bit set %i ", objSetup->gameBitFrozen);
            main_increment_bits(BIT_DR_Lava_Pools_Cooled_Count);
            gDLL_5_AMSEQ2->vtbl->func0(self, 0x102, 0, 0, 0);
        }
    }

    DRLavaControl_freeze_update_effects(self, objData, objData->freezeTimer / 5);
}

/** gUpdateRate tends to be 2 or 3 on N64, so multiplying initial values to keep freeze durations equal across framerates */
RECOMP_PATCH void DRLavaControl_setup(Object* self, DRLavaControl_Setup* objSetup, s32 arg2) {
    Extended_DRLavaControl_Data* objData;
    s32 isFrozen;

    objData = self->data;
    objData->freezeTimer = objSetup->freezeDuration * 3;    //@recomp: increase to account for average FPS on N64
    objData->freezeDuration = objData->freezeTimer;
    
    if (main_get_bits(objSetup->gameBitFrozen)) {
        isFrozen = TRUE;
    } else {
        isFrozen = FALSE;
    }
    
    objData->flags |= isFrozen;
    objData->lfxEmitterVal = lfxEmitterUnk1E[objSetup->dataIndex];
    if (objData->flags & 1) {
        DRLavaControl_freeze_update_effects(self, objData, 0);
        return;
    }
    DRLavaControl_freeze_update_effects(self, objData, 3);
    gDLL_5_AMSEQ2->vtbl->func0(self, 0x102, 0, 0, 0);
}

RECOMP_PATCH u32 DRLavaControl_get_data_size(Object *self, u32 a1) {
    return sizeof(Extended_DRLavaControl_Data);
}

RECOMP_PATCH void DRLavaControl_free(Object* self, s32 arg1) {
    Extended_DRLavaControl_Data* objData = self->data;

    if (objData->lfxEmitter && (arg1 == 0)) {
        obj_destroy_object(objData->lfxEmitter);
    }
    gDLL_13_Expgfx->vtbl->func4.withOneArg((s32)self);

    //@recomp: sound handles
    if (objData->soundHandleHiss) {
        gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandleHiss);
        objData->soundHandleHiss = 0;
    }
    if (objData->soundHandleCrackle) {
        gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandleCrackle);
        objData->soundHandleCrackle = 0;
    }
}
