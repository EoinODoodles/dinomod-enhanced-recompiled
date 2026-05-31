#include "modding.h"

#include "common.h"

typedef struct {
    /** Head aim */
    /* 0x00 */ s8 aimIsActive;
    /* 0x01 */ u8 blinkFrames;      //@recomp: using unused field
    /* 0x02 */ u8 blinkRollTimer;   //@recomp: using unused field
    /* 0x03 */ u8 pad3;
    /* 0x04 */ f32 headAimX;
    /* 0x08 */ f32 headAimY;
    /* 0x0C */ f32 headAimZ;
    /* 0x10 */ f32 headAimUnk;
    
    /** Randomised head turn */
    /* 0x14 */ s16 headGoalAngle;   
    /* 0x16 */ s16 headStartAngle;
    /* 0x18 */ s16 unk18;           //unused?
    /* 0x1A */ s16 headTurnState;
    /* 0x1C */ s16 headTurnDelay;   //random delay before next head turn

    /** Randomised blinks */
    /* 0x1E */ s8 blinkState;
    /* 0x1F */ s8 blinkDelayTimer;

    /** Randomised pupil darts */
    /* 0x20 */ s8 pupilSpeed;       
    /* 0x21 */ s8 pupilDelayTimer;  //frames until next eye dart
    /* 0x22 */ s8 pupilGoal;        //goal position for current eye dart
} HeadAnimation_Recomp;

enum SnowHornAnims {
    MODANIM_SnowHorn_Idle = 0,
    MODANIM_SnowHorn_Talk = 2,
    MODANIM_SnowHorn_Walk = 3,
    MODANIM_SnowHorn_Sleep_Intro = 4,
    MODANIM_SnowHorn_Sleep = 5,
    MODANIM_SnowHorn_Wake_Up = 6,
    MODANIM_SnowHorn_Hit_React = 47
};

/* 
    These patches address behaviour issues while SnowHorn are asleep.
    The ROM patch version of Dinomod Enhanced patches the SnowHorn DLL's update function directly, but
    a different approach is taken in recomp for the moment since the matched SnowHorn function is producing
    slightly different object behaviour. Hopefully this core patch can be replaced with a direct SnowHorn DLL
    function edit once the SnowHorn DLL's decomp is more complete! It currently accomplishes the same thing, in any case.
*/

//Prevent head turn while asleep
RECOMP_PATCH void func_80033B68(Object* obj, HeadAnimation* arg1, f32 arg2) {
    s16* neckJoint;
    s32 var_v0;

    //@recomp: checks if the object is a SnowHorn, and returns early if the SnowHorn is asleep
    if (obj->controlNo == OBJCONTROL_SnowHorn){
        if (obj->curModAnimId == MODANIM_SnowHorn_Sleep_Intro ||
            obj->curModAnimId == MODANIM_SnowHorn_Sleep){
            return;
        }
    }

    neckJoint = func_80034804(obj, 0);
    if (neckJoint == NULL) {
        return;
    }

    if (neckJoint[0] != 0) {
        neckJoint[0] = (neckJoint[0] * 3) / 4;
    }

    if (arg2 < 0.0f) {
        arg2 = -arg2;
    }
    if (arg2 <= 0.1f) {
        func_80033C54(obj, arg1, arg2, neckJoint);
    } else {
        func_80033FD8(obj, arg1, arg2, neckJoint);
    }
    
    var_v0 = arg2 > 0.1f ? 1 : 0;
    arg1->headTurnState = (var_v0 << 8) | (arg1->headTurnState & 0xFF);
}

// Prevent SnowHorn from blinking while asleep, 
// and ensure all randomised blink animation is framerate independent
RECOMP_PATCH void func_80032A08(Object* obj, HeadAnimation* headAnimator) {
    TextureAnimator* eyelidR;
    TextureAnimator* eyelidL;
    s32 eyelidValue;
    HeadAnimation_Recomp* headAnim = (HeadAnimation_Recomp*)headAnimator;
    u8 currentFrame;

    //@recomp: checks if the object is a SnowHorn, and returns early if the SnowHorn is asleep
    if (obj->controlNo == OBJCONTROL_SnowHorn){
        if (obj->curModAnimId == MODANIM_SnowHorn_Sleep_Intro ||
            obj->curModAnimId == MODANIM_SnowHorn_Sleep){
            return;
        }
    }

    eyelidR = func_800348A0(obj, HEAD_ANIMATION_TAG_Eyelid_R, 0);
    eyelidL = func_800348A0(obj, HEAD_ANIMATION_TAG_Eyelid_L, 0);

    if (!eyelidR || !eyelidL) {
        return;
    }

    eyelidValue = eyelidL->frame;

    //@recomp: framerate independent blinking
    switch (headAnim->blinkState & 0xF) {
    case BLINK_Wait:
        headAnim->blinkFrames = 0;
        if (headAnim->blinkDelayTimer > 0) {
            //Wait for timer to run out
            headAnim->blinkDelayTimer -= gUpdateRate;
        } else {
            //@recomp: framerate independent probability
            headAnim->blinkRollTimer += gUpdateRate;
            if (headAnim->blinkRollTimer >= 2){
                headAnim->blinkRollTimer = 0;
                if (rand_next(0, 1000) > 985) {
                    //(Every 3 frames) 1.5% chance of going into a blink
                    headAnim->blinkState = BLINK_Animate;
                    headAnim->blinkDelayTimer = 0;
                }
            }
        }
        break;
    case BLINK_Animate:
        //@recomp: framerate independent texture flipbooking
        headAnim->blinkFrames += gUpdateRate;
        
        currentFrame = headAnim->blinkFrames/3;
        // diPrintf("blink: %d\n", currentFrame);
        switch (currentFrame){
            case 0:
                eyelidValue = 0x100;
                break;
            case 1:
            case 2:
                eyelidValue = 0x200;
                break;
            case 3:
                eyelidValue = 0x100;
                break;
            default:
                eyelidValue = 0x000;
                headAnim->blinkState = BLINK_Wait;
                headAnim->blinkDelayTimer = 0;
                headAnim->blinkFrames = 0;
                break;
        }

        eyelidR->frame = eyelidValue;
        eyelidL->frame = eyelidValue;
        break;
    }

    func_80034678(obj, headAnimator, 0);
}
