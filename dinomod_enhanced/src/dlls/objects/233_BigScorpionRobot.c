#include "modding.h"

#include "dlls/engine/18_objfsa.h"
#include "dlls/engine/33_BaddieControl.h"
#include "dlls/objects/210_player.h"
#include "game/objects/object.h"
#include "sys/objects.h"
#include "sys/rand.h"
#include "dll.h"

#include "recomp/dlls/objects/233_BigScorpionRobot_recomp.h"

typedef struct {
/*0*/ f32 animDelta;
/*4*/ s16 turnStart;
/*6*/ s16 turnDir;
/*8*/ s16 spin;
/*A*/ s16 spinSpeed;
/*C*/ u16 fireCooldown;
/*E*/ u8 hitCount;
/*F*/ u8 enteredState : 1;
/*F*/ u8 fire : 1;
/*F*/ u8 animFinished : 1;
/*F*/ u8 playedDestroyedSound : 1;
} BigScorpionRobot_Data;

enum BigScorpionRobotModAnims {
    BIGSCORP_ROBO_MODANIM_0_Unfold = 0,
    BIGSCORP_ROBO_MODANIM_1 = 1, // walking?
    BIGSCORP_ROBO_MODANIM_2_Fold = 2,
    BIGSCORP_ROBO_MODANIM_3_Firing = 3,
    BIGSCORP_ROBO_MODANIM_4_DamageRecoil = 4,
    BIGSCORP_ROBO_MODANIM_5_Die = 5,
    BIGSCORP_ROBO_MODANIM_6_TurnRight = 6,
    BIGSCORP_ROBO_MODANIM_7_TurnLeft = 7
};

enum BigScorpionRobotStates {
    BIGSCORP_ROBO_STATE_0_Idle = 0,
    BIGSCORP_ROBO_STATE_1_Unfold = 1,
    BIGSCORP_ROBO_STATE_2_Attacking = 2,
    BIGSCORP_ROBO_STATE_3_Fold = 3,
    BIGSCORP_ROBO_STATE_4_DamageRecoil = 4,
    BIGSCORP_ROBO_STATE_5_Dying = 5,
    BIGSCORP_ROBO_STATE_6_Dead = 6
};

extern s32 BigScorpionRobot_is_obj_in_range(Object* self, Object* obj, f32 range);
extern void BigScorpionRobot_func_B88(BigScorpionRobot_Data* objdata);

RECOMP_PATCH s32 BigScorpionRobot_state_0_idle(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie = self->data;
    BigScorpionRobot_Data* objdata = baddie->objdata;
    Object* player = get_player();
    
    fsa->unk341 = 0;
    if (BigScorpionRobot_is_obj_in_range(self, player, (f32) baddie->unk3E2) != 0
            // @recomp: Don't target player if they're dead or untargetable (following BaddieControl logic)
            && ((DLL_210_Player*)player->dll)->vtbl->get_health(player) > 0 
            && ((DLL_210_Player*)player->dll)->vtbl->func66(player, 1) != 0) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, BIGSCORP_ROBO_STATE_1_Unfold);
        objdata->enteredState = 1;
        fsa->target = player;
    }
    objdata->spinSpeed = 0;
    return 0;
}

RECOMP_PATCH s32 BigScorpionRobot_state_2_attacking(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie = self->data;
    BigScorpionRobot_Data* objdata = baddie->objdata;
    s16 temp_v0_4;
    s32 modanimIdx;
    s16 temp;

    // @recomp: Fix rotation desync (this field is a leftover from the smaller robos that spin)
    objdata->spin = 0;
    
    fsa->unk341 = 1;
    if (objdata->enteredState) {
        objdata->enteredState = 0;

        // @recomp: Moved initial anim reset to state entry
        func_80023D30(self, BIGSCORP_ROBO_MODANIM_0_Unfold, 1.0f, 0);
        objdata->animDelta = 0.0f;
    }
    func_80026128(self, 0xA, 1, -1);
    self->objhitInfo->unk5D = 0xA;
    self->objhitInfo->unk5E = 1;
    func_80028D2C(self);
    if (objdata->animFinished) {
        // @recomp: Don't always reset to anim 0 when an anim finishes (causes stutter during turn)
        //func_80023D30(self, BIGSCORP_ROBO_MODANIM_0_Unfold, 1.0f, 0);
        objdata->animDelta = 0.0f;
    }
    if ((self->curModAnimId == 7) || (self->curModAnimId == 6)) {
        // @recomp: The turning anims are set as looped anims, so anim progress will roll over
        //          to zero and screw up the turning logic here due to other edits we made. If
        //          the anim finished/looped, assume 1.0 progress.
        f32 progress = !objdata->animFinished ? self->animProgress : 1.0f;
        self->srt.yaw = (s16) ((f32) objdata->turnStart + (5461.3335f * (f32) objdata->turnDir * progress));
    }
    // @recomp: Allow entering this block at the end of the turning anims
    //if ((self->curModAnimId == 0) && (self->animProgress == 1.0f)) {
    if (objdata->animFinished || self->animProgress == 1.0f) {
        if (fsa->target != NULL) {
            temp = (u16)arctan2_f(
                self->srt.transl.x - fsa->target->srt.transl.x, 
                self->srt.transl.z - fsa->target->srt.transl.z);
            temp_v0_4 = (u16)rotation16_sub_wrap(
                temp, 
                self->srt.yaw + objdata->spin);
            if (temp_v0_4 > 0x2000) {
                modanimIdx = BIGSCORP_ROBO_MODANIM_6_TurnRight;
                objdata->turnDir = -1;
            } else if (temp_v0_4 < -0x2000) {
                modanimIdx = BIGSCORP_ROBO_MODANIM_7_TurnLeft;
                objdata->turnDir = 1;
            } else {
                modanimIdx = -1;
            }
            if (modanimIdx != -1) {
                objdata->animDelta = 0.04f;
                objdata->turnStart = self->srt.yaw;
                func_80023D30(self, modanimIdx, 0.0f, 0);
                gDLL_6_AMSFX->vtbl->play(self, SOUND_6E5_ScorpionRobot_Moving, MAX_VOLUME, NULL, NULL, 0, NULL);
            } else if (objdata->fireCooldown == 0) {
                objdata->fireCooldown = rand_next(0x96, 0x138);
                objdata->fire = 1;
                objdata->animDelta = 0.04f;
                func_80023D30(self, BIGSCORP_ROBO_MODANIM_3_Firing, 0.0f, 0);
            } else {
                // @recomp: Moved anim reset down here so there's no stutter when turning more than once in a row
                if (self->curModAnimId != 0) {
                    func_80023D30(self, BIGSCORP_ROBO_MODANIM_0_Unfold, 1.0f, 0);
                    objdata->animDelta = 0.0f;
                }
            }
        }
    }
    if (BigScorpionRobot_is_obj_in_range(self, fsa->target, (f32) baddie->unk3E2 * 1.2f) == 0) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, BIGSCORP_ROBO_STATE_3_Fold);
        objdata->enteredState = 1;
        fsa->target = NULL;
    }
    BigScorpionRobot_func_B88(objdata);
    return 0;
}
