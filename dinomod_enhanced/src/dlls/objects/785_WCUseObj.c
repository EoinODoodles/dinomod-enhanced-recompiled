#include "modding.h"
#include "object_util.h"
#include "recomputils.h"

#include "dll.h"
#include "game/gamebits.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object_id.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"

#include "recomp/dlls/_asm/785_recomp.h"

//#define DEBUG_IN_FRONT_OF_SWITCH

//TEMPORARY DEFINES
#define WCUseObj_obj_Setup dll_785_obj_Setup
#define WCUseObj_obj_Control dll_785_obj_Control
#define WCUseObj_obj_GetDataSize dll_785_obj_GetDataSize
#define WCUseObj_animCallback dll_785_func_634

#define BIT_WC_Placed_Gold_RedEye_Tooth 0x25A
#define BIT_WC_Placed_Silver_RedEye_Tooth 0x25B
//END OF TEMPORARY DEFINES

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 yaw;
/*19*/ u8 pitch;
/*1A*/ u8 roll;
/*1B*/ u8 flags;
/*1C*/ s16 gamebitInteracted;
/*1E*/ s16 gamebitUsedItem;
/*20*/ s8 baseObjSeqIdx;
/*21*/ s8 modelno;
/*22*/ s16 gamebitUnlocked;
/*24*/ s16 preemptTime;
} WCUseObj_Setup;

typedef struct {
    u8 state;
    u8 actID;
    /* RECOMP */
    f32 sinYaw;         //Used for telling whether the player is in front/behind of WCSunTempleSwitch
    f32 cosYaw;         //Used for telling whether the player is in front/behind of WCSunTempleSwitch
    f32 selfProjected;  //Used for telling whether the player is in front/behind of WCSunTempleSwitch
} WCUseObj_Data;

typedef enum {
    WCUseObj_STATE_Unused = 0, //The button/item deposit point hasn't been used yet
    WCUseObj_STATE_Used = 1    //The button/item deposit point has been used
} WCUseObj_States;

typedef enum {
    WCUseObj_FLAG_1_Hide_On_Revisit_When_Used = 1,
    WCUseObj_CUSTOMFLAG_2_Invert_GamebitUnlocked = 2, //@recomp: repurpose unused flag: invert gamebitUnlocked behaviour (lock when gamebit set, instead of unlock when gamebit set)
    WCUseObj_FLAG_4_ObjSeq_Sets_Gamebit = 4,
    WCUseObj_FLAG_8_Lock_After_Use = 8,
    WCUseObj_FLAG_10_No_Targetting_When_Locked = 0x10,
    WCUseObj_FLAG_20_PreemptActors1and2 = 0x20,
    WCUseObj_FLAG_40_PreemptActors1and2 = 0x40,
    WCUseObj_FLAG_80_PreemptActors1and3 = 0x80
} WCUseObj_Flags;

typedef enum {
    WCUseObj_MODELIDX_Sun = 0,
    WCUseObj_MODELIDX_Moon = 1
} WCUseObj_Models;

int WCUseObj_animCallback(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue);

RECOMP_PATCH void WCUseObj_obj_Setup(Object* self, WCUseObj_Setup* objSetup, s32 reset) {
    WCUseObj_Data* objData;
    TextureAnimator* texAnim;

    self->srt.yaw = objSetup->yaw << 8;
    self->srt.pitch = objSetup->pitch << 8;
    self->srt.roll = objSetup->roll << 8;
    
    self->animCallback = WCUseObj_animCallback;
    
    self->modelInstIdx = objSetup->modelno;
    if (self->modelInstIdx >= self->def->numModels) {
        STUBBED_PRINTF("USEOBJ.c: modelno out of range romdefno=%d\n", objSetup->modelno);
        self->modelInstIdx = 0;
    }
    
    objData = self->data;
    objData->state = mainGetBits(objSetup->gamebitInteracted);
    objData->actID = gDLL_29_Gplay->vtbl->get_act(self->mapID);
    
    if ((objSetup->flags & WCUseObj_FLAG_1_Hide_On_Revisit_When_Used) && (objData->state != WCUseObj_STATE_Unused)) {
        self->opacity = 0;
    }
    
    if (objData->state != WCUseObj_STATE_Unused) {
        texAnim = objExprGetTexAnimator(self, 0, 0);
        if (texAnim != NULL) {
            texAnim->frame = 0x100;
        }
    }

    //@recomp: store calcs for WCSunTempleSwitch, for determining whether player is in front/behind
    if (self->id == OBJ_WCSunTempleSwit) {
        objData->sinYaw = Sinf(self->srt.yaw);
        objData->cosYaw = Cosf(self->srt.yaw);
        objData->selfProjected = (objData->sinYaw * self->globalPosition.x) + (objData->cosYaw * self->globalPosition.z);
    }
}

/* Checks whether the player is in front/behind the Object (used to ensure switches can only be pressed when standing ahead of them) */
static _Bool WCUseObj_isPlayerInFront(Object* self, WCUseObj_Data* objData) {
    Object* player = objGetPlayer();
    f32 playerValue;

    if (objData == NULL) {
        return FALSE;
    }

    if (player) {
        playerValue = (objData->sinYaw * player->globalPosition.x) + (objData->cosYaw * player->globalPosition.z);
        
#ifdef DEBUG_IN_FRONT_OF_SWITCH
        if (vec3DistanceXZSquared(&self->globalPosition, &player->globalPosition) < SQ(50)) {
            diPrintf("selfProjected: %f\n", &objData->selfProjected);
            diPrintf("playerValue: %f\n", &playerValue);
            diPrintf("isPlayerInFront?: %d\n", (playerValue > objData->selfProjected));
        }
#endif

        if (playerValue > objData->selfProjected) {
            return TRUE;
        }
    }

    return FALSE;
}

/* Allow inverting gamebitEnabled's value check (UseObj locked when gamebitEnabled set, rather than unlocked when gamebit set) */
RECOMP_PATCH void WCUseObj_obj_Control(Object* self) {
    WCUseObj_Data* objData;
    WCUseObj_Setup* objSetup;
    TextureAnimator* texAnim;
    s32 actorMask;
    /* RECOMP */
    u32 unlocked;

    objData = self->data;
    objSetup = (WCUseObj_Setup*)self->setup;
    
    objData->state = mainGetBits(objSetup->gamebitInteracted);
    
    if (objData->state == WCUseObj_STATE_Unused) {
        texAnim = objExprGetTexAnimator(self, 0, 0);
        if (texAnim != NULL) {
            texAnim->frame = 0;
        }
        
        self->srt.transl.x = objSetup->base.x;
        self->srt.transl.y = objSetup->base.y;
        self->srt.transl.z = objSetup->base.z;

        //@recomp: hide Act 2 WCUseObjs' LockIcons if WC's act is somehow 0 instead of 1
        if ((objData->actID == 0) && 
            (objSetup->base.actExclusions1 & MAP_ACT(1)) //Only applies to WCUseObjs that are excluded from Act 1
        ) {
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
            return;
        }

        //@recomp: only show LockIcon when in front of WCSunTempleSwitches
        if (self->id != OBJ_WCSunTempleSwit || WCUseObj_isPlayerInFront(self, objData)) {
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
            if (objSetup->gamebitUnlocked != NO_GAMEBIT) {
                //@recomp: handle custom flag, inverting gamebitUnlocked's behaviour (changing from unlocked when set to locked when set)
                if (objSetup->flags & WCUseObj_CUSTOMFLAG_2_Invert_GamebitUnlocked) {
                    unlocked = !mainGetBits(objSetup->gamebitUnlocked);
                } else {
                    unlocked = mainGetBits(objSetup->gamebitUnlocked);
                }

                if (unlocked) {
                    self->unkAF &= ~ARROW_FLAG_10_Greyed_Out;
                } else {
                    self->unkAF |= ARROW_FLAG_10_Greyed_Out;
                    if (objSetup->flags & WCUseObj_FLAG_10_No_Targetting_When_Locked) {
                        self->unkAF |= ARROW_FLAG_8_No_Targetting;
                    }
                }
            } else {
                self->unkAF &= ~ARROW_FLAG_10_Greyed_Out;
            }
        } else {
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
        }
        
        //Check if the button was pressed / an item was used
        if ((self->unkAF & ARROW_FLAG_1_Interacted) && 
            (objSetup->gamebitUsedItem == NO_GAMEBIT || gDLL_1_cmdmenu->vtbl->was_this_item_used(objSetup->gamebitUsedItem))
        ) {
            if (objSetup->baseObjSeqIdx != -1) {
                if (self->id == OBJ_WCInvUseObj) {
                    //@recomp: treat an actID of 0 as being equivalent to Act 1
                    if ((objData->actID <= 1) && (mainGetBits(BIT_WC_Placed_Gold_RedEye_Tooth) || mainGetBits(BIT_WC_Placed_Silver_RedEye_Tooth))) {
                        gDLL_3_Animation->vtbl->start_obj_sequence(objSetup->baseObjSeqIdx + 2, self, -1);
                    } else if ((objData->actID == 2) && (mainGetBits(BIT_WC_Used_Sun_Stone) || mainGetBits(BIT_WC_Used_Moon_Stone))) {
                        gDLL_3_Animation->vtbl->start_obj_sequence(objSetup->baseObjSeqIdx + 2, self, -1);
                    } else {
                        gDLL_3_Animation->vtbl->start_obj_sequence(objSetup->baseObjSeqIdx, self, -1);
                    }
                } else {
                    gDLL_3_Animation->vtbl->start_obj_sequence(objSetup->baseObjSeqIdx, self, -1);
                }
            }
            
            
            if ((objSetup->flags & WCUseObj_FLAG_4_ObjSeq_Sets_Gamebit) == FALSE) {
                mainSetBits(objSetup->gamebitInteracted, TRUE);

                texAnim = objExprGetTexAnimator(self, 0, 0);
                if (texAnim != NULL) {
                    texAnim->frame = 0x100;
                }
            }
            
            if (objSetup->flags & WCUseObj_FLAG_8_Lock_After_Use) {
                mainSetBits(objSetup->gamebitUnlocked, FALSE);
            } else {
                objData->state = WCUseObj_STATE_Used;
                self->unkDC = 1;
            }
            
            joyDisableButtons(0, A_BUTTON);
        }
    } else {
        //Restore "used" state with an ObjSeq preempt
        if ((self->unkDC == 0) && (objSetup->baseObjSeqIdx != -1)) {
            if (objSetup->preemptTime != 0) {
                gDLL_3_Animation->vtbl->preempt_sequence_time(self, objSetup->preemptTime);
                actorMask = 1;
                if (objSetup->flags & WCUseObj_FLAG_20_PreemptActors1and2) {
                    actorMask = 1 | 2;
                }
                if (objSetup->flags & WCUseObj_FLAG_40_PreemptActors1and2) {
                    actorMask |= 1 | 2;
                }
                if (objSetup->flags & WCUseObj_FLAG_80_PreemptActors1and3) {
                    actorMask |= 4;
                }
                gDLL_3_Animation->vtbl->start_obj_sequence(objSetup->baseObjSeqIdx, self, actorMask);
            }
        }
        
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
    }
    
    self->unkDC = 1;
}

/* Extend objData */
RECOMP_PATCH u32 WCUseObj_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(WCUseObj_Data);
}
