#include "common.h"
#include "custom_object_ids.h"
#include "dlls/engine/6_amsfx.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/gfx/model.h"
#include "sys/map.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objlib.h"
#include "sys/print.h"

// #define DEBUG_SWITCH_DIAL_ANGLE
// #define DEBUG_STOP_BLOCKING

typedef struct {
    u8 switchPressed;
    f32 resetTimer;
    Object* dial;
    s16 angleInWheel;
} WCDialProjectileSwitch_Data;

typedef struct {
    ObjSetup base;
    s16 gamebit;            //GamebitID to set when switch is hit
    s16 resetDelay;         //Number of seconds until the switch resets to its unpressed state (if `Switch_FLAG_Resets_After_Delay` is set)
    u8 pitch;               //Rotation
    u8 scale;               //Scale multiplier (0x64 = 100%)
    u8 modelIndexAndFlags;  //Flags on lowest two bits, modelIndex on upper bits
    u8 yaw;                 //Rotation
    u8 tintR;
    u8 tintG;
    u8 tintB;
    u8 enableTint;          //Enable tinting the switch to a different colour
} WCDialProjectileSwitch_Setup;

typedef enum {
    Switch_FLAG_Can_Be_Toggled_Via_Attacks = 1, //Repeated Projectile Spell hits will toggle the switch's state
    Switch_FLAG_Resets_After_Delay = 2          //Switch resets resets to its unpressed state after a number of seconds (specified by `objData->resetDelay`)
} WCDialProjectileSwitch_Flags;

static void WCDialProjectileSwitch_changeState(Object* self, int switchPressed, int playSound);

void WCDialProjectileSwitch_ctor(void* dll) { }

void WCDialProjectileSwitch_dtor(void* dll) { }

/* Calculates a switch's angular position within the dial wheel (relative to the dial at 0 roll) */
static s16 WCDialProjectileSwitch_getObjAngularPosition(Object* dial, Object* obj) {
    f32 sin;
    f32 cos;
    f32 dx;
    f32 dy;
    f32 worldOriginInObjectSpaceX;

    if (dial == NULL || dial->stateFlags & OBJSTATE_DESTROYED || 
        obj == NULL || obj->stateFlags & OBJSTATE_DESTROYED
    ) {
        return 0;
    }

    sin = Sinf(dial->srt.yaw);
    cos = Cosf(dial->srt.yaw);

    worldOriginInObjectSpaceX = (dial->srt.transl.x * cos) - (dial->srt.transl.z * sin); 
    dx = (sin * obj->srt.transl.z) - (obj->srt.transl.x * cos) + worldOriginInObjectSpaceX;
    dy = obj->globalPosition.y - dial->globalPosition.y;

    return Arctanf(dx, dy);
}

/* Finds the dial, and calculates/stores the switch's base angle in the dial wheel */
static void WCDialProjectileSwitch_findDial(Object* self, WCDialProjectileSwitch_Data* objData) {
    Object* dial;
    f32 distance;

    //Return early if the dial has already been found
    if (objData->dial && (objData->dial->stateFlags & OBJSTATE_DESTROYED) == FALSE) {
        return;
    }

    distance = BLOCKS_GRID_UNIT;
    dial = objFindClosestObject(self, OBJ_WCTempleDial, &distance);
    if (dial == NULL || dial->stateFlags & OBJSTATE_DESTROYED) {
        objData->dial = NULL;
        return;
    }

    objData->dial = dial;

    //Calculate the switch's angular position within the dial wheel
    objData->angleInWheel = WCDialProjectileSwitch_getObjAngularPosition(dial, self);
}

// func: 0 | export: 0
static void WCDialProjectileSwitch_obj_Setup(Object* self, WCDialProjectileSwitch_Setup* objSetup, s32 reset) {
    WCDialProjectileSwitch_Data* objData = self->data;
    
    self->srt.yaw = objSetup->yaw << 8;
    self->srt.pitch = objSetup->pitch << 8;
    
    //Set scale
    if (objSetup->scale == 0) {
        self->srt.scale = self->def->scale;
    } else {
        self->srt.scale = objSetup->scale * self->def->scale * (1.0f / 64.0f);
    }
    self->objhitInfo->unk52 = (objSetup->scale * self->def->hitbox_flagsB6) / 64;
    
    //Set model instance
    self->modelInstIdx = objSetup->modelIndexAndFlags >> 2;
    if (self->modelInstIdx >= self->def->numModels) {
        self->modelInstIdx = 0;
    }
    
    //Check if the switch is already pressed
    objData->switchPressed = mainGetBits(objSetup->gamebit);
    if (objData->switchPressed) {
        WCDialProjectileSwitch_changeState(self, TRUE, FALSE);
    } else {
        WCDialProjectileSwitch_changeState(self, FALSE, FALSE);
    }
    
    //Find the dial and calculate the switch's angle within its wheel
    WCDialProjectileSwitch_findDial(self, objData);

    if (objSetup->enableTint == FALSE) {
        self->stateFlags |= OBJSTATE_PRINT_DISABLED;
    }
    self->stateFlags |= OBJSTATE_UPDATE_DISABLED;
}

// func: 1 | export: 1
static void WCDialProjectileSwitch_obj_Control(Object* self) {
    WCDialProjectileSwitch_Setup* objSetup;
    WCDialProjectileSwitch_Data* objData;

    objData = self->data;
    objSetup = (WCDialProjectileSwitch_Setup*)self->setup;

    //Ensure the dial has been found before doing anything else
    WCDialProjectileSwitch_findDial(self, objData);
    if (objData->dial == NULL) {
        return;
    }

    if (objData->switchPressed && (mainGetBits(objSetup->gamebit) == FALSE)) {
        WCDialProjectileSwitch_changeState(self, FALSE, TRUE);
    }
    
    //If the switch has an active delay timer, wait for it to expire
    if (objData->resetTimer > 0.0f) {
        objData->resetTimer -= (f32)gUpdateRate;
        if (objData->resetTimer <= 0.0f) {
            objData->resetTimer = 0.0f;
            mainSetBits(objSetup->gamebit, 0);
        } else {
            return;
        }
    }

#ifdef DEBUG_SWITCH_DIAL_ANGLE
    {
        s32 angleDiff = objData->angleInWheel - objData->dial->srt.roll;
        CIRCLE_WRAP(angleDiff);

        diPrintf("Angle diff: %x (%d degrees)\n", angleDiff, (s32)((f32)((f32)angleDiff)/M_360_DEGREES_F*360.0f));
    }
#endif

    //React to Projectile Switch attacks
    if (func_80025F40(self, NULL, NULL, NULL) == Damage_Type_Projectile) {

#ifndef DEBUG_STOP_BLOCKING
        //Check if the wheel's opening is roughly over the switch
        {
            s32 angleDiff = objData->angleInWheel - objData->dial->srt.roll;
            CIRCLE_WRAP(angleDiff);
            if (angleDiff < 0) {
                angleDiff = -angleDiff;
            }

            //If the wheel is covering the switch, deflect the attack
            if (angleDiff > M_20_DEGREES) {
                dll_amSfx->Play(self, SOUND_25B_Magic_Attack_Deflected, MAX_VOLUME, NULL, NULL, 0, NULL);
                return;
            }
        }
#endif

        if (objData->switchPressed) {
            if ((objSetup->modelIndexAndFlags & 3) == Switch_FLAG_Can_Be_Toggled_Via_Attacks) {
                WCDialProjectileSwitch_changeState(self, FALSE, TRUE);
                mainSetBits(objSetup->gamebit, 0);
            }
        } else {
            WCDialProjectileSwitch_changeState(self, TRUE, TRUE);
            mainSetBits(objSetup->gamebit, 1);
            if ((objSetup->modelIndexAndFlags & 3) == Switch_FLAG_Resets_After_Delay) {
                objData->resetTimer = objSetup->resetDelay * 0.1f * 60.0f;
            }
        }
    }
}

// func: 2 | export: 2
static void WCDialProjectileSwitch_obj_Update(Object* self) { }

// func: 3 | export: 3
static void WCDialProjectileSwitch_obj_Print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    WCDialProjectileSwitch_Setup* objSetup = (WCDialProjectileSwitch_Setup*)self->setup;
    
    if (visibility) {
        if (objSetup->enableTint) {
            objprintSetMultiplierColor(objSetup->tintR, objSetup->tintG, objSetup->tintB);
        }
        objprintDrawModel(self, gdl, mtxs, vtxs, pols, 1.0f);
    }
}

// func: 4 | export: 4
static void WCDialProjectileSwitch_obj_Free(Object* self, s32 onlySelf) { }

// func: 5 | export: 5
static u32 WCDialProjectileSwitch_obj_GetModelFlags(Object* self) {
    WCDialProjectileSwitch_Setup* objSetup;
    s32 modelIndex;

    objSetup = (WCDialProjectileSwitch_Setup*) self->setup;
               
    modelIndex = objSetup->modelIndexAndFlags >> 2;
    if (modelIndex >= self->def->numModels) {
        modelIndex = 0;
    }
    
    return MODFLAGS_MODEL_INDEX(modelIndex) | MODFLAGS_LOAD_SINGLE_MODEL;
}

// func: 6 | export: 6
static u32 WCDialProjectileSwitch_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(WCDialProjectileSwitch_Data);
}

// func: 7
static void WCDialProjectileSwitch_changeState(Object* self, int switchPressed, int playSound) {
    WCDialProjectileSwitch_Data* objData;
    TextureAnimator* texAnim;
    s32 soundID;

    objData = self->data;
    
    //Optionally play a state change sound
    if (playSound) {
        soundID = switchPressed ? SOUND_7AE_Switch_Hit_Blast : SOUND_7AF_Switch_Reset_Swoosh;
        dll_amSfx->Play(self, soundID, MAX_VOLUME, NULL, NULL, 0, NULL);
    }
    
    //Set texture frame
    texAnim = objExprGetTexAnimator(self, 0, 0);
    if (texAnim != NULL) {
        texAnim->frame = switchPressed << 8;
    }
    
    //Set state
    objData->switchPressed = switchPressed;
}

DLL_IObject_Vtbl WCDialProjectileSwitch_vtbl = {
    .Setup = (void*)WCDialProjectileSwitch_obj_Setup,
    .Control = WCDialProjectileSwitch_obj_Control,
    .Update = WCDialProjectileSwitch_obj_Update,
    .Print = WCDialProjectileSwitch_obj_Print,
    .Free = WCDialProjectileSwitch_obj_Free,
    .GetModelFlags = WCDialProjectileSwitch_obj_GetModelFlags,
    .GetDataSize = WCDialProjectileSwitch_obj_GetDataSize
};
