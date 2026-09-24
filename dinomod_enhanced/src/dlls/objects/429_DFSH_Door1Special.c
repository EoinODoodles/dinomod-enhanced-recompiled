#include "dll.h"
#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "sys/main.h"

#include "recomp/dlls/objects/429_DFSH_door1Special_recomp.h"

//TEMPORARY DEFINES
#define DFSH_Door1Special_obj_Setup DFSH_Door1Special_setup
#define DFSH_Door1Special_animCallback DFSH_Door1Special_anim_callback
//END OF TEMPORARY DEFINES

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 gamebitOpened;       //[Main Door Only] Opens the door
/*1A*/ s16 gamebitDoorState;    //[Main Door Only] Restores the door's state during setup
/*1C*/ s16 seqPreemptTime;      //[Main Door Only] ObjSeq time to jump to when restoring state
/*1E*/ s8 seqIndex;             //[Main Door Only] ObjSeq to play
/*1F*/ u8 yaw;
/*20*/ u8 enabledActors;        //[Main Door Only] ObjSeq actor mask
/*21*/ u8 scale;
/*22*/ s16 gamebitLit;          //A point on the door's Krazoa symbol lights up when this gamebit is set (through pressing ancient switches around Discovery Falls)
} DFSH_DoorSpecial_Setup;

typedef struct {
    u16 phase;      //Phase angle for the Krazoa symbol's glow effect
    u8 state;       //[Main Door Only] SeqDoor state (`DFSH_DoorSpecial_States`)
    u8 glowState;   //Glow effect state (`DFSH_DoorSpecial_GlowStates`)
    u8 runControl;  //[Main Door Only] Boolean: starts off TRUE, set to FALSE at the end of obj_Control (so control only runs once)
} DFSH_DoorSpecial_Data;

typedef enum {
    DFSH_Door1Special_STATE_0_Closed,
    DFSH_Door1Special_STATE_1_Open,
    DFSH_Door1Special_STATE_2_Opening,
    DFSH_Door1Special_STATE_3_Closing
} DFSH_DoorSpecial_States;

typedef enum {
    DFSH_DoorSpecial_GLOW_0_Unlit,
    DFSH_DoorSpecial_GLOW_1_Fade_In,
    DFSH_DoorSpecial_GLOW_2_Pulse
} DFSH_DoorSpecial_GlowStates;

typedef enum {
    SeqDoor_SEQCMD_1_Finished_Closing = 1,
    SeqDoor_SEQCMD_2_Finished_Opening = 2
} SeqDoor_ObjSeqMessages;

extern int DFSH_Door1Special_animCallback(Object* self, Object* overrideObj, AnimObj_Data* animData, s8 prevCallbackResult);

RECOMP_PATCH void DFSH_Door1Special_obj_Setup(Object* self, DFSH_DoorSpecial_Setup* objSetup, s32 reset) {
    DFSH_DoorSpecial_Data* objData;
    TextureAnimator* texAnim;

    objData = self->data;
    
    //Restore texture glow state
    {
        if (mainGetBits(objSetup->gamebitLit)) {
            objData->glowState = DFSH_DoorSpecial_GLOW_2_Pulse;
        } else {
            objData->glowState = DFSH_DoorSpecial_GLOW_0_Unlit;
        }
    
        texAnim = objExprGetTexAnimator(self, 0, 0);
        if (texAnim != NULL) {
            if (objData->glowState == DFSH_DoorSpecial_GLOW_2_Pulse) {
                texAnim->frame = 1;
            } else {
                texAnim->frame = 0;
            }
        }
    }
    
    objData->runControl = TRUE;
    self->srt.yaw = objSetup->yaw << 8;
    self->animCallback = DFSH_Door1Special_animCallback;
    
    //Set scale
    {
        if (objSetup->scale == 0) {
            objSetup->scale = 64;
        }
        self->srt.scale = objSetup->scale * (1.0f / 64.0f);
        if (self->srt.scale == 0.0f) {
            self->srt.scale = 1.0f;
        }
        self->srt.scale *= self->def->scale;
    }

    //Restore state by gamebit
    if (objSetup->gamebitDoorState != NO_GAMEBIT) {
        objData->state = mainGetBits(objSetup->gamebitDoorState);
    } else {
        objData->state = DFSH_Door1Special_STATE_0_Closed;
    }

    //@recomp: ensure both door gamebits are synced
    if (objData->state == DFSH_Door1Special_STATE_0_Closed) {
        if (mainGetBits(objSetup->gamebitOpened)) {
            mainSetBits(objSetup->gamebitOpened, FALSE);
        }
    } else {
        if (mainGetBits(objSetup->gamebitOpened) == FALSE) {
            mainSetBits(objSetup->gamebitOpened, TRUE);
        }
    }
    
    objData->phase = 0;
}

RECOMP_PATCH int DFSH_Door1Special_animCallback(Object* self, Object* overrideObj, AnimObj_Data* animData, s8 prevCallbackResult) {
    DFSH_DoorSpecial_Data* objData;
    DFSH_DoorSpecial_Setup* objSetup;
    TextureAnimator* texAnim;
    s32 i;
    s32 frame;

    objData = self->data;
    objSetup = (DFSH_DoorSpecial_Setup*)self->setup;
    
    //Texture glow State Machine
    switch (objData->glowState) {
    case DFSH_DoorSpecial_GLOW_0_Unlit:
        if (mainGetBits(objSetup->gamebitLit)) {
            objData->glowState = DFSH_DoorSpecial_GLOW_1_Fade_In;
        }
        break;
    case DFSH_DoorSpecial_GLOW_1_Fade_In:
        //Texture blends into glowing state
        texAnim = objExprGetTexAnimator(self, 0, 0);
        if (texAnim != NULL) {
            frame = texAnim->frame + (gUpdateRate * 8);
            if (frame > 0x100) {
                frame = 0x100;
                objData->glowState = DFSH_DoorSpecial_GLOW_2_Pulse;
            }
            texAnim->frame = frame;
        }
        break;
    case DFSH_DoorSpecial_GLOW_2_Pulse:
    default:
        //Glow pulses slowly, via oscillating texture frame blending 
        texAnim = objExprGetTexAnimator(self, 0, 0);
        if (texAnim != NULL) {
            objData->phase += gUpdateRate * 800;
            texAnim->frame = 0x100 - ((1.0f - mathCosfInterp(objData->phase)) * 50.0f);
        }
        break;
    }
    
    //Door opening State Machine (very similar to DLL 307 "SeqDoor")
    if (objData->state == DFSH_Door1Special_STATE_0_Closed) {
        if (mainGetBits(objSetup->gamebitOpened)) {
            objData->state = DFSH_Door1Special_STATE_2_Opening;
        }
    } else if ((objData->state == DFSH_Door1Special_STATE_1_Open) && (mainGetBits(objSetup->gamebitOpened) == FALSE)) {
        objData->state = DFSH_Door1Special_STATE_3_Closing;
    }
    
    if (objData->state == DFSH_Door1Special_STATE_2_Opening) {
        for (i = 0; i < animData->messageCount; i++) {
            if (animData->messages[i] == SeqDoor_SEQCMD_2_Finished_Opening) {
                objData->state = DFSH_Door1Special_STATE_1_Open;
                if (objSetup->gamebitDoorState != NO_GAMEBIT) {
                    mainSetBits(objSetup->gamebitDoorState, DFSH_Door1Special_STATE_1_Open);

                    //@recomp: ensure both door gamebits are synced
                    mainSetBits(objSetup->gamebitOpened, TRUE);
                }
            }
        }
    } else if (objData->state == DFSH_Door1Special_STATE_3_Closing) {
        for (i = 0; i < animData->messageCount; i++) {
            if (animData->messages[i] == SeqDoor_SEQCMD_1_Finished_Closing) {
                objData->state = DFSH_Door1Special_STATE_0_Closed;
                if (objSetup->gamebitDoorState != NO_GAMEBIT) { //@recomp: check if gamebit is specified
                    mainSetBits(objSetup->gamebitDoorState, DFSH_Door1Special_STATE_0_Closed);

                    //@recomp: ensure both door gamebits are synced
                    mainSetBits(objSetup->gamebitOpened, FALSE);
                }
            }
        }
    }
    
    return !(objData->state == DFSH_Door1Special_STATE_2_Opening) && !(objData->state == DFSH_Door1Special_STATE_3_Closing);
}
