#include "PR/os.h"
#include "modding.h"

#include "PR/ultratypes.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/rand.h"

#include "segment_334F0.h"

typedef enum {
    BLINK_Wait = 0,
    BLINK_Animate = 1,
    BLINK_Eyelid_Close_Finished = 0x80
} BlinkStates;

typedef enum {
    HEAD_TURN_Goal_Reached = 0,
    HEAD_TURN_Wait = 1,
    HEAD_TURN_Animate = 2
} HeadTurnStates;

typedef enum {
    HEAD_ANIMATION_TAG_Pupil_L = 0,
    HEAD_ANIMATION_TAG_Pupil_R = 1,
    HEAD_ANIMATION_TAG_Eyelid_L = 4,
    HEAD_ANIMATION_TAG_Eyelid_R = 5
} HeadAnimationTags;

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
    if (obj->group == 40){
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

//Prevent SnowHorn from blinking while asleep
RECOMP_PATCH void func_80032A08(Object* obj, HeadAnimation* arg1) {
    s32* eyelidR;
    s32* eyelidL;
    s32 eyelidValue;

    //@recomp: checks if the object is a SnowHorn, and returns early if the SnowHorn is asleep
    if (obj->group == 40){
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

    eyelidValue = *eyelidL;

    switch (arg1->blinkState & 0xF) {
    case BLINK_Wait:
        if (arg1->blinkDelayTimer > 0) {
            //Wait for timer to run out
            arg1->blinkDelayTimer -= gUpdateRate;
        } else if (rand_next(0, 1000) > 985) {
            //1.5% chance of going into a blink
            arg1->blinkState = BLINK_Animate;
            arg1->blinkDelayTimer = 0;
        }
        break;
    case BLINK_Animate:
        if (arg1->blinkState & BLINK_Eyelid_Close_Finished) {
            //Animate eyelid opening
            eyelidValue -= 0x100;
            if (eyelidValue < 0) {
                eyelidValue = 0;
                arg1->blinkState = BLINK_Wait;
                arg1->blinkDelayTimer = 0;
            }
        } else {
            //Animate eyelid closing
            eyelidValue += 0x100;
            if (eyelidValue > 0x200) {
                eyelidValue -= 0x200;
                if (eyelidValue < 0) {
                    eyelidValue = 0;
                    arg1->blinkState = BLINK_Wait;
                } else {
                    arg1->blinkState = (s8)(BLINK_Eyelid_Close_Finished + BLINK_Animate);
                }
                arg1->blinkDelayTimer = 0;
            }
        }
        *eyelidR = eyelidValue;
        *eyelidL = eyelidValue;
        break;
    }

    func_80034678(obj, arg1, 0);
}
