#include "modding.h"

#include "common.h"
#include "sys/objlib.h"

#include "recomp/dlls/objects/394_CFTreasureDoor_recomp.h"

typedef struct {
    u8 preemptState;
    u8 state;
    s32 texAnimTimer;
} CFTreasureDoor_Data;

typedef enum {
    CFTreasureDoor_STATE_0_Initial,
    CFTreasureDoor_STATE_1_Find_Other_Door,
    CFTreasureDoor_STATE_2_Finished
} CFTreasureDoor_States;

typedef enum {
    CFTreasureDoor_PREEMPT_STATE_1_Preempt_Needed = 1,
    CFTreasureDoor_PREEMPT_STATE_2_Preempt_Finished = 2
} CFTreasureDoor_ObjSeq_PreemptStates;

extern s16 dBabyPerchedGamebits[];

RECOMP_PATCH void CFTreasureDoor_obj_Control(Object* self) {
    CFTreasureDoor_Data* objData;
    TextureAnimator* texAnim;
    Object* otherDoor;
    s32 frameIdx;
    s32 gamebitIdx;
    f32 distance;
    u8 preemptState;

    objData = self->data;
    gamebitIdx = 0;
    distance = 6000.0f;

    //Preempt the door into its open position if the Treasure Room doors have already been opened
    {
        preemptState = objData->preemptState;
        if (preemptState == CFTreasureDoor_PREEMPT_STATE_2_Preempt_Finished) {
            return;
        }

        switch (preemptState) {
        case CFTreasureDoor_PREEMPT_STATE_1_Preempt_Needed:
            gDLL_3_Animation->vtbl->preempt_sequence_time(self, 85);
            gDLL_3_Animation->vtbl->start_obj_sequence(0, self, 1);
            objData->preemptState = CFTreasureDoor_PREEMPT_STATE_2_Preempt_Finished;
            return;   
        }
    }

    //When this door's sequence finishes: find the other door, and set the "both doors open" gamebit 
    {
        if (self->unkDC != 0) {
            self->unkDC = 0;
            self->unkE0 = 1;
        }
        
        if (self->unkE0 != 0) {
            distance = 80000.0f;
            otherDoor = objFindClosestObject(self, OBJ_CFTreasureDoor, &distance);
            if ((otherDoor != NULL) && (otherDoor->unkE0 != 0)) {
                mainSetBits(BIT_477_CRF_Treasure_Room_Doors_Opened, TRUE);
                self->unkE0 = 0;
            }
        }
    }
    
    //Decide which texAnimator frame to use
    {
        objData->texAnimTimer += gUpdateRate;
        frameIdx = 0;

        if (mainGetBits(BIT_CRF_Play_Seq_005D_Treasure_Room_Door_Opening_Sequences)) {
            //Blink all lights
            if ((objData->texAnimTimer % 79) / 20) {
                frameIdx = 5;
            }
        } else {
            //Cycle through the perch lights, and light up the ones that have a CloudRunner Baby on them
            gamebitIdx = (objData->texAnimTimer % 79) / 20;
            if (mainGetBits(dBabyPerchedGamebits[gamebitIdx])) {
                frameIdx = gamebitIdx + 1;
            }
        }
    }
    
    //@debug code: set the current CloudRunner Baby gamebit using B!
    // @recomp: remove dev code
    // if (joyGetPressed(0) & B_BUTTON) {
    //     mainSetBits(dBabyPerchedGamebits[gamebitIdx], TRUE);
    // }
    
    if (objData->state != CFTreasureDoor_STATE_2_Finished) {
        //Set door's texture frame (lighting up green panels for each CloudRunner baby rescued)
        texAnim = objExprGetTexAnimator(self, 0, 0);
        if (texAnim != NULL) {
            texAnim->frame = frameIdx << 8;
        }
        
        //Check if Treasure Door opening sequence should play
        if (mainGetBits(BIT_CRF_Play_Seq_005D_Treasure_Room_Door_Opening_Sequences)) {
            objData->state = CFTreasureDoor_STATE_1_Find_Other_Door;
        }
        
        //Play the other door's opening sequence
        if (objData->state == CFTreasureDoor_STATE_1_Find_Other_Door) {
            distance = 80000.0f; 
            otherDoor = objFindClosestObject(self, OBJ_CFTreasureDoor, &distance);
            
            if ((otherDoor && (otherDoor->unkDC == 0)) || (otherDoor == NULL)) {
                self->unkDC = 1;
                gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);
                objData->state = CFTreasureDoor_STATE_2_Finished;
            }
        }
    }
}
