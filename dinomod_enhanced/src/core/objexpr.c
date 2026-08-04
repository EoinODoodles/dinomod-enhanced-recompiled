#include "modding.h"

#include "game/objects/object.h"
#include "sys/objexpr.h"
#include "sys/main.h"
#include "sys/rand.h"

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

extern struct {
    u8 unk0_0 : 1;
} D_800B2E00;

extern s32 D_80091720[];

RECOMP_PATCH s32 objExpr_func_800334A4(Object* obj, Object* lookat, Vec3f* refPoint, HeadAnimation* anims, s16* arg4, f32 yOffset, s16 arg6, s16 arg7) {
    f32 dx;
    f32 dy;
    f32 dz; // fa1
    f32 xzDist;
    s16 sp84[2];
    s16 goal[2];
    s32 temp_lo;
    s16 var_a0;
    s16 var_a1;
    s32 pad;
    s32 var_v1;
    SeqJoint* seqJoint;
    u8 sp6B;
    s32 temp_ft0;
    s16* var_t3;
    s32 i; // s3
    s32 j;

    sp6B = 0;
    dx = refPoint->x - lookat->srt.transl.x;
    dz = refPoint->z - lookat->srt.transl.z;
    dy = (refPoint->y + yOffset) - lookat->srt.transl.y;
    xzDist = sqrtf(SQ(dx) + SQ(dz));
    sp84[0] = (s16)(u16)mathAtan2f(dx, dz) - (obj->srt.yaw & 0xFFFF);
    CIRCLE_WRAP(sp84[0]);
    sp84[1] = arg7 - (-mathAtan2f(xzDist, dy) & 0xFFFF);
    CIRCLE_WRAP(sp84[1]);
    if (D_800B2E00.unk0_0) {
        sp84[0] -= 0x8000;
        sp84[1] = -sp84[1];
    }
    for (i = 0; i < 10; i++) {
        seqJoint = objExpr_func_80034804(obj, D_80091720[i]);
        if (seqJoint == NULL) {
            // @recomp: Reset flag that flips the lookat direction. For some reason, this early return does not reset
            //          the flag like the end of the function does. This causes the flag to leak over into other calls
            //          causing various NPCs to have a messed up head lookat. Notably, the shop keeper sets this flag.
            D_800B2E00.unk0_0 = 0;
            return sp6B;
        }
        for (j = 0; j < 2; j++) {
            if (j % 2) {
                var_a0 = arg4[i + 15U] * 182.04f;
            } else {
                var_a0 = arg4[i] * 182.04f;
            }
            goal[j] = sp84[j];
            if (var_a0 < sp84[j]) {
                goal[j] = var_a0;
                sp84[j] -= var_a0;
            } else if (sp84[j] < -var_a0) {
                goal[j] = -var_a0;
                sp84[j] += var_a0;
            } else {
                sp84[j] = 0;
            }
        }
        if (anims != NULL) {
            anims->headGoalAngle = goal[0];
            objExpr_func_80034250(anims, seqJoint);
            anims[1].headGoalAngle = goal[1];
            objExpr_func_80034518(anims + 1, seqJoint, 10.0f, 500.0f);
            anims += 2;
        } else {
            var_t3 = arg4 + 15;
            var_a1 = (seqJoint->yaw + goal[0]) >> 1;
            var_a1 -= seqJoint->yaw;
            temp_lo = ((s16) (-arg4[i] * 182.04f) / 10) * gUpdateRate;
            if (var_a1 < temp_lo) {
                var_a1 = temp_lo;
            } else {
                temp_lo = ((s16) (arg4[i] * 182.04f) / 10) * gUpdateRate;
                if (temp_lo < var_a1) {
                    var_v1 = temp_lo;
                } else {
                    var_v1 = var_a1;
                }
                var_a1 = var_v1;
            }
            var_a0 = (seqJoint->pitch + goal[1]) >> 1;
            var_a0 -= seqJoint->pitch;
            temp_ft0 = (s16) (var_t3[i] * 182.04f);
            pad = (- temp_ft0 / 10) * gUpdateRate;
            if (var_a0 < pad) {
                var_a0 = pad;
            } else {
                if (((temp_ft0 / 20) * gUpdateRate) < var_a0) {
                    var_v1 = ((temp_ft0 / 20) * gUpdateRate);
                } else {
                    var_v1 = var_a0;
                }
                var_a0 = var_v1;
            }
            seqJoint->pitch += var_a0;
            seqJoint->yaw += var_a1;
        }
        if (i == 0) {
            var_v1 = (goal[0] - 4) < seqJoint->yaw;
            if (var_v1 != 0) {
                var_v1 = seqJoint->yaw < (goal[0] + 4);
            }
            sp6B = var_v1;
        }
    }
    D_800B2E00.unk0_0 = 0;
    return sp84[0];
}

/* 
    These patches address behaviour issues while SnowHorn are asleep.
    The ROM patch version of Dinomod Enhanced patches the SnowHorn DLL's update function directly, but
    a different approach is taken in recomp for the moment since the matched SnowHorn function is producing
    slightly different object behaviour. Hopefully this core patch can be replaced with a direct SnowHorn DLL
    function edit once the SnowHorn DLL's decomp is more complete! It currently accomplishes the same thing, in any case.
*/

//Prevent head turn while asleep
RECOMP_PATCH void objExpr_func_80033B68(Object* obj, HeadAnimation* headAnim, f32 arg2) {
    SeqJoint* headJoint;
    s32 var_v0;

    //@recomp: checks if the object is a SnowHorn, and returns early if the SnowHorn is asleep
    if (obj->controlNo == OBJCONTROL_SnowHorn){
        if (obj->curModAnimId == MODANIM_SnowHorn_Sleep_Intro ||
            obj->curModAnimId == MODANIM_SnowHorn_Sleep){
            return;
        }
    }

    headJoint = objExpr_func_80034804(obj, 0);
    if (headJoint == NULL) {
        return;
    }

    if (headJoint->pitch != 0) {
        headJoint->pitch = (headJoint->pitch * 3) / 4;
    }

    if (arg2 < 0.0f) {
        arg2 = -arg2;
    }
    if (arg2 <= 0.1f) {
        objExpr_func_80033C54(obj, headAnim, arg2, headJoint);
    } else {
        objExpr_func_80033FD8(obj, headAnim, arg2, headJoint);
    }
    
    var_v0 = arg2 > 0.1f ? 1 : 0;
    headAnim->headTurnState = (var_v0 << 8) | (headAnim->headTurnState & 0xFF);
}

// Prevent SnowHorn from blinking while asleep, 
// and ensure all randomised blink animation is framerate independent
RECOMP_PATCH void objExprEyeIdle(Object* obj, HeadAnimation* headAnimator) {
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

    eyelidR = objExprGetTexAnimator(obj, HEAD_ANIMATION_TAG_Eyelid_R, 0);
    eyelidL = objExprGetTexAnimator(obj, HEAD_ANIMATION_TAG_Eyelid_L, 0);

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
                if (mathRnd(0, 1000) > 985) {
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

    objExprEyeDart(obj, headAnimator, 0);
}
