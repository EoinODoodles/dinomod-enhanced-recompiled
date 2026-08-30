#include "configs.h"
#include "modding.h"
#include "objects/781_WCPressureSwitch.h"

#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/gfx/animseq.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/objexpr.h"
#include "sys/print.h"

#include "recomp/dlls/objects/781_WCpressureswitch_recomp.h"

//TEMPORARY DEFINES
#define WCPressureSwitch_obj_Control WCpressureswitch_control
#define WCPressureSwitch_obj_GetModelFlags WCpressureswitch_get_model_flags
#define WCPressureSwitch_addObject WCpressureswitch_add_object
#define WCPressureSwitch_isObjectOnSwitch WCpressureswitch_is_object_on_switch
#define WCPressureSwitch_animCallback WCpressureswitch_anim_callback
//END OF TEMPORARY DEFINES

// #define DEBUG_SWITCH

typedef struct {
/*00*/ u32 soundHandle;
/*04*/ s8 pressedTimer;
/*05*/ s8 state;
/*08*/ Object* objectsOnSwitch[10];
/*30*/ Vec2f objCoords[10];
} PressureSwitch_Data;

typedef enum {
    STATE_0_UP,
    STATE_1_MOVING_UP,
    STATE_2_DOWN,
    STATE_3_MOVING_DOWN
} WCPressureSwitch_States;

static char WCPressureSwitchStateNames[4][30] = {
    "0_UP",
    "1_MOVING_UP",
    "2_DOWN",
    "3_MOVING_DOWN"
};

extern void WCPressureSwitch_addObject(Object* self, Object* objectOnSwitch);
extern s32 WCPressureSwitch_isObjectOnSwitch(Object* self);
extern int WCPressureSwitch_animCallback(Object* self, Object* animObj, AnimObj_Data* animObjData, s8 prevCallbackValue);

RECOMP_PATCH void WCPressureSwitch_obj_Control(Object* self) {
    WCPressureSwitch_Setup* objSetup;
    f32 deltaY;
    Object* listedObject;
    TextureAnimator* animTexture;
    s32 index;
    PressureSwitch_Data* objData;

    objSetup = (WCPressureSwitch_Setup*)self->setup;
    objData = self->data;

#ifdef DEBUG_SWITCH
    diPrintf("PressureSwitch %x: [control] State %s\n", self->setup->uID, WCPressureSwitchStateNames[objData->state]);
#endif

    //@recomp: Sync state with WCBeacon's lit gamebit (when it's lit, at least)
    if (objSetup->gamebitFinished > NO_GAMEBIT + 1) {
        if (mainGetBits(objSetup->gamebitFinished)) {
            objData->state = STATE_2_DOWN;
            self->srt.transl.y = objSetup->base.y - objSetup->yOffsetAnimation;

            animTexture = objExprGetTexAnimator(self, 0, 0);
            if (animTexture != NULL) {
                animTexture->frame = 0x100;
            }
            return;
        }
    }

    //Decrement timer until not considered pressed (fps-dependent)
    objData->pressedTimer -= gUpdateRate; //@recomp: fix framerate dependency
    if (objData->pressedTimer < 0) {
        objData->pressedTimer = 0;
    }

    //@recomp: reset optional "player on switch" gamebit
    if (objSetup->gamebitPlayerOnSwitch > NO_GAMEBIT + 1) {
        mainSetBits(objSetup->gamebitPlayerOnSwitch, FALSE);
    }

    //Handle adding objects to switch
    if (self->polyhits->unk10F > 0) {
        for (index = 0; index < self->polyhits->unk10F; index++) {
            listedObject = (Object*)self->polyhits->unk100[index];

            //@recomp: optionally ignore projectiles
            if (configs_GetWCPressureSwitchIgnoreProjectiles() && (listedObject->controlNo == OBJCONTROL_Projectile)) {
                continue;
            }

            deltaY = listedObject->srt.transl.y - self->srt.transl.y;
            if (deltaY > objSetup->yThreshold) {
                WCPressureSwitch_addObject(self, listedObject);
            }

            //@recomp: optionally set a gamebit if the player is on the switch
            if ((objSetup->gamebitPlayerOnSwitch > NO_GAMEBIT + 1) && (listedObject->controlNo == OBJCONTROL_Player)) {
                mainSetBits(objSetup->gamebitPlayerOnSwitch, TRUE);
            }
        }
    }

    //Check if object on switch
    if (WCPressureSwitch_isObjectOnSwitch(self)) {
        objData->pressedTimer = 10; //@recomp: fix framerate dependency
    }

    //Main state machine
    deltaY = objSetup->base.y - objSetup->yOffsetAnimation;
    switch (objData->state) {
        case STATE_0_UP:
            if ((objData->pressedTimer != 0) && (deltaY <= self->srt.transl.y)) {
                dll_amSfx->Play(self, SOUND_99A_Mechanical_Ratcheting, MAX_VOLUME, NULL, 0, 0, 0);
                objData->state = STATE_3_MOVING_DOWN;
            }
            break;
        case STATE_3_MOVING_DOWN:
            self->srt.transl.y -= 0.05f * gUpdateRateF;
            if (self->srt.transl.y < deltaY) {
                mainSetBits(objSetup->gameBitPressed, 1);
                objData->state = STATE_2_DOWN;
                self->srt.transl.y = deltaY;
            }
            break;
        case STATE_2_DOWN:
            /* Subtly different behaviour to other pressure switches,
             * waits for flag to unset before depressing the switch (for WC's timed challenges) */
            if (!mainGetBits(objSetup->gameBitPressed)) {
                dll_amSfx->Play(self, SOUND_99A_Mechanical_Ratcheting, MAX_VOLUME, NULL, 0, 0, 0);
                objData->state = STATE_1_MOVING_UP;
            }
            break;
        case STATE_1_MOVING_UP:
            self->srt.transl.y += 0.05f * gUpdateRateF;
            if (self->srt.transl.y > objSetup->base.y) {
                self->srt.transl.y = objSetup->base.y;
                objData->state = STATE_0_UP;
            }
            break;
    }

    //Change texture frame (sun/moon glowing)
    animTexture = objExprGetTexAnimator(self, 0, 0);
    if (animTexture != NULL) {
        if (objData->state == STATE_2_DOWN) {
            animTexture->frame = 1;
        } else {
            animTexture->frame = 0;
        }
        animTexture->frame <<= 8;
    }
}

// Prevents the pressure switches' object arrays from overflowing and crashing (originally by MusicalProgrammer)
RECOMP_PATCH void WCPressureSwitch_addObject(Object* self, Object* objectOnSwitch) {
    PressureSwitch_Data *objdata = self->data;
    u8 objectIndex;
    
    //@recomp: fix loop condition, and check if object already in list
    for (objectIndex = 0; objectIndex < 10; objectIndex++){
        if (objdata->objectsOnSwitch[objectIndex] == NULL || 
            objdata->objectsOnSwitch[objectIndex] == objectOnSwitch){
            break;
        }
    }
    
    objdata->objectsOnSwitch[objectIndex] = objectOnSwitch;    
    objdata->objCoords[objectIndex].x = objectOnSwitch->srt.transl.x;
    objdata->objCoords[objectIndex].y = objectOnSwitch->srt.transl.z;
}

RECOMP_PATCH s32 WCPressureSwitch_isObjectOnSwitch(Object* self) {
    PressureSwitch_Data* objdata;
    Vec2f* coord;
    u8 index;
    u8 returnVal;

    objdata = self->data;

    returnVal = FALSE;

    for (index = 0; index < 10; index++) {
        if (!objdata->objectsOnSwitch[index])
            continue;

        //@recomp: ignore objects other than the player while a timed challenge is already active
        // (The Pressure Switch QOL config lets you swap between the sun/moon challenges on the fly -
        // so this check makes sure only the player can cause the swap, not Tricky! Tricky can still
        // press the switch down if the Sun/Moon challenges aren't active though, as before.)
        if ((objdata->objectsOnSwitch[index]->controlNo != OBJCONTROL_Player) && menu_func_8000FB1C() == FALSE) {
            continue;
        }

        coord = &objdata->objCoords[index];
        if (objdata->objectsOnSwitch[index]->srt.transl.x == coord->x && 
            objdata->objectsOnSwitch[index]->srt.transl.z == coord->y) {
            returnVal = TRUE;
        } else {
            objdata->objectsOnSwitch[index] = NULL;
        }
    }

    return returnVal;
}

/* Ensure both models are loaded, so the Moon switch's polyHits work */
RECOMP_PATCH u32 WCPressureSwitch_obj_GetModelFlags(Object* self) {
    return MODFLAGS_1;
}
