#include "modding.h"

#include "game/objects/object.h"
#include "game/gamebits.h"
#include "dlls/engine/53_movelib.h"
#include "dlls/objects/496_SnowHorn.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objanim.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "dll.h"
#include "macros.h"

#include "recomp/dlls/objects/712_DIMSnowHorn1_recomp.h"

extern s16 dWalkingAnims[2];
extern f32 dWalkSpeedThresholds[4];

typedef struct {
    ObjFSA_Data fsa;
    MoveLibData moveData;
    HeadAnimation headAnim;
    Vec3f particleAttachCoords[4];
    s32 unk858;
    s32 unk85C;
    Vec3f riderPosition;
    s8 unk86C[0x8FC - 0x86C];
    s16 fidgetTimer;
    s16 minTurn;
    s16 energy;
    u8 mountState;
    u8 unk903;
    u8 characterIdx;
    u8 chatSequenceIdx;
    u8 flags;
    u8 dismountToRight;
    u8 mountFromLeft;
} DIMSnowHorn_Data;

typedef enum {
    DIMSnowHorn_ASTATE_0_Init,                      //Switches state based on the character
    DIMSnowHorn_ASTATE_1_Shackled_Idle,             //Shackled SnowHorn standing around
    DIMSnowHorn_ASTATE_2_Shackled_Fidget,           //Shackled SnowHorn scratching back/shaking off snow
    DIMSnowHorn_ASTATE_3_Leap_Idle,                 //Leap of Faith cave SnowHorn standing around
    DIMSnowHorn_ASTATE_4_Leap_Fidget,               //Leap of Faith cave SnowHorn scratching back/shaking off snow
    DIMSnowHorn_ASTATE_5_Famished_Fallen,           //Famished SnowHorn being attacked by a SharpClaw
    DIMSnowHorn_ASTATE_6_Famished_Met,              //Famished SnowHorn saved from the SharpClaw, but still needs feeding
    DIMSnowHorn_ASTATE_7_Famished_Fed_Once,     //Fanished SnowHorn fed one Alpine Root, and waiting for a second one
    DIMSnowHorn_ASTATE_8_Vehicle_Idle,              //SnowHorn standing around, waiting for the player
    DIMSnowHorn_ASTATE_9_Vehicle_Sit,               //SnowHorn sitting down and getting back up again after an input (unused?)
    DIMSnowHorn_ASTATE_10_Vehicle_Turn_on_Spot,     //SnowHorn turning (without walking forward)
    DIMSnowHorn_ASTATE_11_Vehicle_Walking,          //SnowHorn walking (can turn slightly)
    DIMSnowHorn_ASTATE_12_Vehicle_Tusk_Attack       //SnowHorn attacking with tusks
} DIMSnowHorn_AnimStates;

// Prevent a softlock that occurs when the SnowHorn runs out of energy (originally by MusicalProgrammer)
RECOMP_PATCH s32 DIMSnowHorn_animState11VehicleWalk(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    u8 one;
    DIMSnowHorn_Data *objData = self->data;
    s32 pad;
    f32 walkFactor;
    f32 walkSpeed;
    f32 animProgress;
    s16 curModAnimId;
    s32 animChanged;
    s32 returnValue;
    s32 startingWalk;
    s32 walkAnimIdx;
    f32 *thresholds;

    fsa->flags |= OBJFSA_FLAG_200000;

    if (fsa->enteredAnimState) {
        self->srt.yaw += fsa->unk32A * M_1_DEGREE;
        fsa->unk328 = 0;
        fsa->unk32A = 0;
    }

    //Handle joystick deadzone
    if (fsa->analogInputPower < 0.05f){
        fsa->analogInputPower = 0.0f;
        fsa->unk328 = 0;
        fsa->unk32A = 0;
    }

    //Turn while walking
    if (fsa->unk328 < 90){
        self->srt.yaw += ((fsa->unk32A * updateRate) / 36.0f) * 182.0f;
    } else {
        return FSA_NEXTSTATE_SYNC(DIMSnowHorn_ASTATE_8_Vehicle_Idle);
    }

    //Handle walk speed
    {
        walkFactor = fsa->analogInputPower;
        if (walkFactor < 0.0f){
            walkFactor = 0.0f;
        }
        if (walkFactor > 1.0f){
            walkFactor = 1.0f;
        }

        // Handle SnowHorn running out of energy 
        //@recomp: allow SnowHorn to continue moving when exhausted
        if (objData->energy == 0){
            //walkFactor = 0;
        }

        walkSpeed = walkFactor * 0.85f;
        if (walkSpeed < 0){
            walkSpeed = 0;
        }

        fsa->speed += ((walkSpeed - fsa->speed) / fsa->unk2B0) * updateRate;
        if (self->srt.pitch > 0){
            walkSpeed -= mathSinfInterp(self->srt.pitch) * 0.3f;
        } else {
            walkSpeed -= mathSinfInterp(self->srt.pitch) * 0.15f;
        }

        if (walkSpeed < dWalkSpeedThresholds[2]){
            walkSpeed = dWalkSpeedThresholds[2];
        }

        fsa->unk278 += ((walkSpeed - fsa->unk278) / fsa->unk2B0) * updateRate;
    }

    //Handle animations
    {
        animChanged = FALSE;
        one = 1;

        animProgress = self->animProgress;

        //Find the current walkAnimIdx
        {
            for (walkAnimIdx = 0; self->curModAnimId != dWalkingAnims[walkAnimIdx] && walkAnimIdx < ARRAYCOUNT_S(dWalkingAnims); walkAnimIdx++);
            
            if (walkAnimIdx > 1){
                walkAnimIdx = 0;
            }

            if (self->curModAnimId == SnowHorn_MODANIM2_8_Walk_Intro){
                walkAnimIdx = 1;
            }
        }

        //Compare the walk speed with the walk anims' min/max speed thresholds, to determine whether anim should change
        thresholds = &dWalkSpeedThresholds[walkAnimIdx * 2];
        if (fsa->speed < thresholds[0]){
            animChanged = TRUE;
            if (walkAnimIdx == 1){
                return FSA_NEXTSTATE_SYNC(DIMSnowHorn_ASTATE_8_Vehicle_Idle);
            }
            walkAnimIdx -= one;
        } else if (thresholds[1] <= fsa->speed){
            animChanged = TRUE;
            if (walkAnimIdx == 0){
                animProgress = 0.0f;
            }
            walkAnimIdx++;
        }

        //Check whether a walk intro animation should be played
        startingWalk = TRUE;
        if (fsa->unk33A && (self->curModAnimId == SnowHorn_MODANIM2_8_Walk_Intro)){
            animChanged = TRUE;
            startingWalk = FALSE;
        }

        //Play/advance walk animations
        if (animChanged){
            if ((walkAnimIdx == 1) && startingWalk){
                objAnimSet(self, SnowHorn_MODANIM2_8_Walk_Intro, animProgress, 0);
            } else {
                objAnimSet(self, dWalkingAnims[walkAnimIdx], animProgress, 0);
            }
        }
        objGetAnimChange(self, fsa->unk278, &fsa->animTickDelta);
    }

    if (fsa->unk310 & A_BUTTON){
        return FSA_NEXTSTATE_SYNC(DIMSnowHorn_ASTATE_12_Vehicle_Tusk_Attack);
    } else {
        return 0;
    }
}
