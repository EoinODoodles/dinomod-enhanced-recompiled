#include "modding.h"

#include "common.h"
#include "game/gamebits.h"

#include "recomp/dlls/objects/737_DRLavaControl_recomp.h"
#include "sys/objtype.h"

typedef struct {
/*0*/ Object* lfxEmitter;
/*4*/ s16 lfxEmitterVal;
/*6*/ s8 freezeTimer;
/*7*/ s8 freezeDuration;
/*8*/ s8 flags;
/*9*/ s8 effectIndex; //used to switch between different effect presets during Ice Spell cooldown
} DRLavaControl_Data;

typedef struct {
/*00*/    ObjSetup base;
/*18*/    s8 unused18;
/*18*/    s8 dataIndex;
/*1A*/    s16 freezeDuration;   //how long it takes to freeze with Ice Blast spell
/*1C*/    s16 unused1C;
/*1E*/    s16 gameBitFrozen;    //gamebit to set when fully cooled with Ice Blast
} DRLavaControl_Setup;

extern Object* DRLavaControl_create_light(Object* self, s32 lfxSetupUnk1E);

// Fix DRLavaControl LFXEmitters from crashing after you leave section, after cooling lava (originally by MusicalProgrammer)
RECOMP_PATCH void DRLavaControl_freeze_update_effects(Object* self, DRLavaControl_Data* objData, s32 effectIndex) {
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
