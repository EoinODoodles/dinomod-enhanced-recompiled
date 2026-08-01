#include "game/gamebits.h"
#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "game/objects/object_id.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"

#include "recomp/dlls/objects/307_recomp.h"

// #define DEBUG_SEQDOOR

//TEMPORARY DEFINES
#define SeqDoor_obj_Setup dll_307_setup
#define SeqDoor_obj_Control dll_307_control
#define SeqDoor_obj_GetDataSize dll_307_get_data_size
#define SeqDoor_animCallback dll_307_func_33C
#define SeqDoor_setCameraPositionGamebits dll_307_func_6E4
//END OF TEMPORARY DEFINES

typedef struct {
    ObjSetup base;
    s16 gamebitOpenA;           //animCallback advances door to "Opening" state when this gamebit (and gamebitOpenB if specified) is set, or to "Closing" state when unset (obj must be in a sequence!)
    s16 gamebitRestoreState;    //Restores state when the door loads (either "Open" or "Closed" state) - objSeq will use a preemptTime to skip the door to its open position when restoring "Open" state.
    s16 preemptTime;            //The sequence time to jump to when the object loads in its "Open" state (state restored via `gamebitRestoreState`).
    s8 objSeqIdx;               //The door opening cutscene's objSeqIdx
    u8 yaw;
    u8 preemptEnabledActors;    //Configures which actors to include when the door's sequence is played using a preemptTime
    u8 scale;                   //Scale factor for the door (0x40 is 1x scale)
    s16 gamebitOpenB;           //When specified, animCallback advances door to "Opening" state when this gamebit and gamebitOpenA are set (obj must be in a sequence!)
    s16 gamebitCameraBack;      //When specified, animCallback flips part (`flipBitsCameraBack`) of this gamebit's value if the camera is behind the door when the door finishes opening/closing (obj must be in a sequence!)
    s16 gamebitCameraFront;     //When specified, animCallback flips part (`flipBitsCameraFront`) of this gamebit's value if the camera is in front of the door when the door finishes opening/closing (obj must be in a sequence!)
    u8 flipBitsCameraBack;      //The section of `gamebitCameraBack`'s value to flip when `gamebitCameraBack`'s value is updated
    u8 flipBitsCameraFront;     //The section of `gamebitCameraFront`'s value to flip when `gamebitCameraFront`'s value is updated
} SeqDoor_Setup;

typedef struct {
    f32 sinYaw;                  //Used to streamline calculating whether the camera is in front of/behind the door
    f32 cosYaw;                  //Used to streamline calculating whether the camera is in front of/behind the door
    f32 worldOriginObjectSpaceZ; //Used to streamline calculating whether the camera is in front of/behind the door
    u8 state;                    //The door's state machine value (handled in animCallback only)
    u8 startSequence;            //Whether the control function should start playing the door's objSeq (always activates the first time control runs)
    u8 flags;                    //Tracks whether gamebitOpenA and gamebitOpenB (if specified) are set - used to apply textureAnimators to the door, etc.
    /* RECOMP */
    u8 prevStateGamebitValue;
#ifdef DEBUG_SEQDOOR
    u8 lastController;
#endif
} SeqDoor_Data;

typedef enum {
    SeqDoor_STATE_0_Closed,
    SeqDoor_STATE_1_Open,
    SeqDoor_STATE_2_Opening,
    SeqDoor_STATE_3_Closing
} SeqDoor_States;

typedef enum {
    SeqDoor_FLAG_1_Activated_A = 1,
    SeqDoor_FLAG_2_Activated_B = 2
} SeqDoor_Flags;

typedef enum {
    SeqDoor_SEQCMD_1_Finished_Closing = 1,
    SeqDoor_SEQCMD_2_Finished_Opening = 2
} SeqDoor_ObjSeqMessages;

//@Recomp: extra flags
typedef enum {
    SeqDoor_CUSTOMFLAG_4_Played = 4,
    SeqDoor_CUSTOMFLAG_8_Delay_Play_Until_Gamebit_Set = 8,
    SeqDoor_CUSTOMFLAG_10_Was_Already_Open = 0x10,
    SeqDoor_CUSTOMFLAG_20_Unload_If_Already_Open = 0x20,
    SeqDoor_CUSTOMFLAG_40_Unload_At_End_of_Sequence = 0x40,
    SeqDoor_CUSTOMFLAG_80_Sync_With_State_Gamebit = 0x80
} SeqDoor_CustomFlags;

extern int SeqDoor_animCallback(Object *self, Object *animObj, AnimObj_Data *animData, s8 prevCallbackValue);
extern void SeqDoor_setCameraPositionGamebits(SeqDoor_Data *objData, SeqDoor_Setup *objSetup);

RECOMP_PATCH void SeqDoor_obj_Setup(Object* self, SeqDoor_Setup* objSetup, s32 reset) {
    SeqDoor_Data* objData = self->data;

#ifdef DEBUG_SEQDOOR
    recomp_printf("\n%s entered setup\n", self->def->name);
#endif

    objData->startSequence = TRUE;
    self->srt.yaw = objSetup->yaw << 8;
    self->animCallback = SeqDoor_animCallback;
    self->stateFlags |= OBJSTATE_UPDATE_DISABLED;

    //Set scale
    {
        self->srt.scale = objSetup->scale * (1.0f / 64.0f);
        if (self->srt.scale == 0.0f) {
            self->srt.scale = 1.0f;
        }
        self->srt.scale *= self->def->scale;
    }

    //Restore state via gamebit
    if (objSetup->gamebitRestoreState != NO_GAMEBIT) {
        objData->state = mainGetBits(objSetup->gamebitRestoreState);
    } else {
        objData->state = SeqDoor_STATE_0_Closed;
    }

    //Project the world origin onto the door's objectSpace Z-axis
    //(Helps with later calcs determining whether the camera is in front of or behind the door)
    objData->sinYaw = mathSinfInterp(self->srt.yaw);
    objData->cosYaw = mathCosfInterp(self->srt.yaw);
    objData->worldOriginObjectSpaceZ = -((objData->sinYaw * self->srt.transl.x) + (objData->cosYaw * self->srt.transl.z));

    //Set flags based on gamebits (for WCDoorSlab lighting up its Moon/Sun icons, etc.)
    objData->flags = 0;
    if (mainGetBits(objSetup->gamebitOpenA)) {
        objData->flags |= SeqDoor_FLAG_1_Activated_A;
    }
    if (mainGetBits(objSetup->gamebitOpenB)) {
        objData->flags |= SeqDoor_FLAG_2_Activated_B;
    }

    //@recomp: flag when a door loads in its Open state
    if (objData->state == SeqDoor_STATE_1_Open) {
        objData->flags |= SeqDoor_CUSTOMFLAG_10_Was_Already_Open;
    }

    //@recomp: enable custom flags on specific objects like WCCageDoor 
    // (TODO: set custom flags via the objSetup instead, maybe? 
    // The `SeqDoor_Setup` struct doesn't really have enough unused bytes to store initial flags currently, 
    // but maybe it could be extended and all its MAPS instances updated! Or maybe this is fine?)
    switch (self->id) {
    case OBJ_WCCageDoor:
        objData->flags |= (SeqDoor_CUSTOMFLAG_8_Delay_Play_Until_Gamebit_Set | 
                           SeqDoor_CUSTOMFLAG_20_Unload_If_Already_Open | 
                           SeqDoor_CUSTOMFLAG_40_Unload_At_End_of_Sequence
                        );
        break;
    case OBJ_WCSlabDoor:
        objData->flags |= SeqDoor_CUSTOMFLAG_80_Sync_With_State_Gamebit;
        break;
    case OBJ_WCBossDoor:
        //@recomp: ensure King RedEye's ramp door is open when Sabre's exiting it with the SpellStone
        objData->flags |= SeqDoor_CUSTOMFLAG_80_Sync_With_State_Gamebit;
        break;
    }

#ifdef DEBUG_SEQDOOR
    if (objData->flags & ~(SeqDoor_FLAG_1_Activated_A | SeqDoor_FLAG_2_Activated_B)) {
        recomp_printf("%s given custom flags: %x\n", self->def->name, objData->flags);
    }
#endif

    //@recomp: optionally delay playing the sequence
    if (objData->flags & SeqDoor_CUSTOMFLAG_8_Delay_Play_Until_Gamebit_Set) {
        objData->startSequence = FALSE;
    }

    //@recomp: initialise prevStateGamebitValue to gamebit value
    objData->prevStateGamebitValue = objData->state;
}

static void SeqDoor_resetTransform(Object* self, SeqDoor_Data* objData, SeqDoor_Setup* objSetup) {
    self->srt.transl.x = objSetup->base.x;
    self->srt.transl.y = objSetup->base.y;
    self->srt.transl.z = objSetup->base.z;
    self->srt.yaw = objSetup->yaw << 8;
    self->srt.pitch = 0;
    self->srt.roll = 0;

    //Set scale
    {
        self->srt.scale = objSetup->scale * (1.0f / 64.0f);
        if (self->srt.scale == 0.0f) {
            self->srt.scale = 1.0f;
        }
        self->srt.scale *= self->def->scale;
    }
}

static int SeqDoor_handleCustomFlags(Object* self, SeqDoor_Data* objData, SeqDoor_Setup* objSetup) {
    //Optionally replay the door's sequence if its state gamebit changes outside of animCallback
    if (objData->flags & SeqDoor_CUSTOMFLAG_80_Sync_With_State_Gamebit) {
        if (objSetup->gamebitRestoreState != NO_GAMEBIT) {
            s32 stateGamebitValue = mainGetBits(objSetup->gamebitRestoreState);

            if (objData->prevStateGamebitValue != stateGamebitValue) {
                objData->state = stateGamebitValue;
                objData->prevStateGamebitValue = stateGamebitValue;
                objData->startSequence = TRUE;

                //Clear this flag so the preemptTime won't be used
                if (objData->flags & SeqDoor_CUSTOMFLAG_8_Delay_Play_Until_Gamebit_Set) {
                    objData->flags &= ~SeqDoor_CUSTOMFLAG_10_Was_Already_Open;
                }

                //Put the door back at its initial position when returning to closed state
                if (objData->state == SeqDoor_STATE_0_Closed) {
                    SeqDoor_resetTransform(self, objData, objSetup);
                }

                return FALSE;
            }
        }
    }

    //Optionally unload the door at the end of playing its sequence
    if ((objData->flags & SeqDoor_CUSTOMFLAG_4_Played) && 
        (objData->flags & SeqDoor_CUSTOMFLAG_40_Unload_At_End_of_Sequence)
    ) {
        objFreeObject(self);
#ifdef DEBUG_SEQDOOR
        recomp_printf("%s finished sequence, unloading.\n", self->def->name);
#endif
        return TRUE;
    }

    //Optionally unload the door just after it loads, if its gamebit was already set before setup
    if ((objData->flags & SeqDoor_CUSTOMFLAG_10_Was_Already_Open) && 
        (objData->flags & SeqDoor_CUSTOMFLAG_20_Unload_If_Already_Open)
    ) {
        objFreeObject(self);
#ifdef DEBUG_SEQDOOR
        recomp_printf("%s was already open! Unloading.\n", self->def->name);
#endif
        return TRUE;
    }

    //Optionally delay playing the sequence until the door's gamebitRestoreState is set
    if (
        (objData->flags & SeqDoor_CUSTOMFLAG_8_Delay_Play_Until_Gamebit_Set) &&
        ((objData->flags & SeqDoor_CUSTOMFLAG_4_Played) == FALSE)
    ) {
        if ((objSetup->gamebitRestoreState != NO_GAMEBIT)) {
            objData->state = mainGetBits(objSetup->gamebitRestoreState);
            objData->startSequence = (objData->state == SeqDoor_STATE_1_Open);
#ifdef DEBUG_SEQDOOR
            if (objData->startSequence) {
                recomp_printf("%s: delayed sequence play about to start...\n", self->def->name);
            }
#endif
        }
    }

    return FALSE;
}

RECOMP_PATCH void SeqDoor_obj_Control(Object* self) {
    SeqDoor_Data* objData;
    SeqDoor_Setup* objSetup;
    s32 enabledActors;

    objSetup = (SeqDoor_Setup*)self->setup;
    objData = (SeqDoor_Data*)self->data;

#ifdef DEBUG_SEQDOOR
    if (objData->lastController != 1) {
        objData->lastController = 1;
        recomp_printf("%s: running control\n", self->def->name);
    }
#endif

    //@recomp: Handle custom flags (exiting early from control if needed)
    if (SeqDoor_handleCustomFlags(self, objData, objSetup)) {
        return;
    }

    if (objData->startSequence) {
        //Skip the door's objSeq to its preemptTime, if the door's not in its initial state
        if (objSetup->preemptTime && (objData->state != SeqDoor_STATE_0_Closed)
            && (((objData->flags & SeqDoor_CUSTOMFLAG_8_Delay_Play_Until_Gamebit_Set) == FALSE) || ((objData->flags & SeqDoor_CUSTOMFLAG_8_Delay_Play_Until_Gamebit_Set) && (objData->flags & SeqDoor_CUSTOMFLAG_10_Was_Already_Open))) //@recomp
        ) {
#ifdef DEBUG_SEQDOOR
            recomp_printf("%s setting preempt time %d (state %d)\n", self->def->name, objSetup->preemptTime, objData->state);
#endif
            enabledActors = objSetup->preemptEnabledActors;
            gDLL_3_Animation->vtbl->preempt_sequence_time(self, objSetup->preemptTime);
        } else {
            enabledActors = -1;
        }
        
        //Play the door's sequence
        if (objSetup->objSeqIdx != -1) {
            gDLL_3_Animation->vtbl->start_obj_sequence(objSetup->objSeqIdx, self, enabledActors);
#ifdef DEBUG_SEQDOOR
            recomp_printf("%s starting obj sequence! (enabledActors: %d)\n", self->def->name, enabledActors);
#endif

            //@recomp: flag that the sequence has played
            objData->flags |= SeqDoor_CUSTOMFLAG_4_Played;
        }

        objData->startSequence = FALSE;
    }
}

RECOMP_PATCH int SeqDoor_animCallback(Object *self, Object *animObj, AnimObj_Data *animData, s8 prevCallbackValue) {
    SeqDoor_Data *objData;
    SeqDoor_Setup *objSetup;
    TextureAnimator *animator;
    u32 activatedA;
    s32 activatedB;
    s32 i;

    objData = (SeqDoor_Data*)self->data;
    objSetup = (SeqDoor_Setup*)self->setup;
  
#ifdef DEBUG_SEQDOOR
    if (objData->lastController != 2) {
        objData->lastController = 2;
        recomp_printf("%s: running animCallback\n", self->def->name);
    }
#endif

    //@recomp: check if state gamebit changed outside of sequence
    if (objData->flags & SeqDoor_CUSTOMFLAG_80_Sync_With_State_Gamebit) {
        if (objSetup->gamebitRestoreState != NO_GAMEBIT) {
            s32 stateGamebitValue = mainGetBits(objSetup->gamebitRestoreState);

            if ((objData->prevStateGamebitValue != stateGamebitValue)) {
#ifdef DEBUG_SEQDOOR
                recomp_printf("%s: state gamebit (%x) changed outside of animCallback (value: %d -> %d), restarting sequence!\n", 
                    self->def->name, objSetup->gamebitRestoreState, objData->prevStateGamebitValue, stateGamebitValue);
#endif
                objData->state = stateGamebitValue;
                objData->prevStateGamebitValue = stateGamebitValue;
                objData->startSequence = TRUE;
                gDLL_3_Animation->vtbl->end_obj_sequence(self->seqSlot);

                //Put the door back at its initial position when returning to closed state
                if (objData->state == SeqDoor_STATE_0_Closed) {
                    SeqDoor_resetTransform(self, objData, objSetup);
                }
                return 1;
            }
        }
    }

    //Change animated texture frames using a TextureAnimator (used by WCSlabDoor to light up the Moon/Sun icons on the door)
    if (self->def->numAnimatedFrames != 0) {
        //Light up moon icon (textureAnimator0)
        if (objData->flags & SeqDoor_FLAG_1_Activated_A) {
            animator = objExprGetTexAnimator(self, 0, 0);
            if (animator != NULL) {
                animator->frame = 0x100;
            }
        }

        //Light up sun icon (textureAnimator1)
        if (objData->flags & SeqDoor_FLAG_2_Activated_B) {
            animator = objExprGetTexAnimator(self, 1, 0);
            if (animator != NULL) {
                animator->frame = 0x100;
            }
        }
    }

    //State Machine (NOTE: only runs while the door is in a sequence!)
    {
        if (objData->state == SeqDoor_STATE_0_Closed) {
            activatedA = mainGetBits(objSetup->gamebitOpenA);
            activatedB = FALSE;

            //Check if gamebitOpenB is set too (if gamebitOpenB wasn't specified, just ignore it and assume it's set)
            if ((objSetup->gamebitOpenB == NO_GAMEBIT) || mainGetBits(objSetup->gamebitOpenB)) {
                activatedB = TRUE;
            }

            //Flag that gamebitOpenA is set, and play a sound if this will queue textureAnimator0
            if (activatedA && ((objData->flags & SeqDoor_FLAG_1_Activated_A) == FALSE)) {
                if (self->def->numAnimatedFrames != 0) {
                    dll_amSfx->Play(self, SOUND_9A3_Magic_Reverse_Cymbal, MAX_VOLUME, NULL, NULL, 0, NULL);
                }
                objData->flags |= SeqDoor_FLAG_1_Activated_A;
            }

            //Flag that gamebitOpenB is set, and play a sound if this will queue textureAnimator1
            if (activatedB && ((objData->flags & SeqDoor_FLAG_2_Activated_B) == FALSE)) {
                if (self->def->numAnimatedFrames != 0) {
                    dll_amSfx->Play(self, SOUND_9A3_Magic_Reverse_Cymbal, MAX_VOLUME, NULL, NULL, 0, NULL);
                }
                objData->flags |= SeqDoor_FLAG_2_Activated_B;
            }

            //Advance state when gamebitOpenA and gamebitOpenB are set (or when just gamebitOpenA is set, if gamebitOpenB isn't specified)
            //@recomp: change condition slightly, so the custom flags don't trip this up
            // if (objData->flags == (SeqDoor_FLAG_1_Activated_A | SeqDoor_FLAG_2_Activated_B)) {
            if ((objData->flags & SeqDoor_FLAG_1_Activated_A) && (objData->flags & SeqDoor_FLAG_2_Activated_B)) {
                SeqDoor_setCameraPositionGamebits(objData, objSetup);
                objData->state = SeqDoor_STATE_2_Opening;
            }
        } else if ((objData->state == SeqDoor_STATE_1_Open) && (mainGetBits(objSetup->gamebitOpenA) == FALSE)) {
            //Start closing if gamebitOpenA unsets while the door is open
            objData->state = SeqDoor_STATE_3_Closing;
        }

        //Advance states from opening->open and closing->closed, based on objSeq messages
        if (objData->state == SeqDoor_STATE_2_Opening) {
            for (i = 0; i < animData->messageCount; i++) {
                if (animData->messages[i] == SeqDoor_SEQCMD_2_Finished_Opening) {
                    objData->state = SeqDoor_STATE_1_Open;
                    if (objSetup->gamebitRestoreState != NO_GAMEBIT) {
                        mainSetBits(objSetup->gamebitRestoreState, SeqDoor_STATE_1_Open);
                    }
                }
            }
        } else if (objData->state == SeqDoor_STATE_3_Closing) {
            for (i = 0; i < animData->messageCount; i++) {
                if (animData->messages[i] == SeqDoor_SEQCMD_1_Finished_Closing) {
                    SeqDoor_setCameraPositionGamebits(objData, objSetup);
                    objData->state = SeqDoor_STATE_0_Closed;

                    //@recomp: only clear vanilla flags, retain custom flags
                    // objData->flags = 0;
                    objData->flags &= ~(SeqDoor_FLAG_1_Activated_A | SeqDoor_FLAG_2_Activated_B); 
                    if (objSetup->gamebitRestoreState != NO_GAMEBIT) {
                        mainSetBits(objSetup->gamebitRestoreState, SeqDoor_STATE_0_Closed);
                    }
                }
            }
        }
    }

    //@recomp
    objData->prevStateGamebitValue = mainGetBits(objSetup->gamebitRestoreState);

    return !(objData->state == SeqDoor_STATE_2_Opening) && !(objData->state == SeqDoor_STATE_3_Closing);
}

RECOMP_PATCH u32 SeqDoor_obj_GetDataSize(Object *self, u32 offsetAddr) {
    return sizeof(SeqDoor_Data);
}
