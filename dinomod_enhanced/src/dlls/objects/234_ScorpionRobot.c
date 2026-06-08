#include "modding.h"

#include "dlls/engine/18_objfsa.h"
#include "dlls/engine/27.h"
#include "dlls/engine/33_BaddieControl.h"
#include "dlls/objects/210_player.h"
#include "game/objects/interaction_arrow.h"
#include "sys/gfx/projgfx.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "sys/objmsg.h"
#include "sys/rand.h"
#include "dll.h"

#include "recomp/dlls/objects/234_ScorpionRobot_recomp.h"

typedef struct {
/*0*/ f32 animDelta;
/*4*/ s16 turnStart;
/*6*/ s16 turnDir;
/*8*/ s16 spin;
/*A*/ s16 spinSpeed;
/*C*/ u16 fireCooldown;
/*E*/ u8 enteredState : 1;
/*E*/ u8 fire : 1;
/*E*/ u8 animFinished : 1;
} ScorpionRobot_Data;

enum ScorpionRobotModAnims {
    SCORP_ROBO_MODANIM_0_Unfold = 0,
    SCORP_ROBO_MODANIM_1 = 1, // walking?
    SCORP_ROBO_MODANIM_2_Fold = 2,
    SCORP_ROBO_MODANIM_3_Firing = 3,
    SCORP_ROBO_MODANIM_4_DamageRecoil = 4,
    SCORP_ROBO_MODANIM_5_Die = 5,
    SCORP_ROBO_MODANIM_6_TurnRight = 6,
    SCORP_ROBO_MODANIM_7_TurnLeft = 7
};

enum ScorpionRobotStates {
    SCORP_ROBO_STATE_0_Spinning = 0,
    SCORP_ROBO_STATE_1_WaitForPlayer = 1,
    SCORP_ROBO_STATE_2_Unfold = 2,
    SCORP_ROBO_STATE_3_Attacking = 3,
    SCORP_ROBO_STATE_4_Fold = 4,
    SCORP_ROBO_STATE_5_DamageRecoil = 5,
    SCORP_ROBO_STATE_6_Dying = 6,
    SCORP_ROBO_STATE_7_Dead = 7
};

/*0x0*/ extern s8 sHitDamageMap[];
/*0x1C*/ extern u16 sHitSounds[];
/*0x24*/ extern s32 data_24[];

/*0x0*/ extern ObjFSA_StateCallback sStateCallbacks[8];
/*0x20*/ extern ObjFSA_StateCallback sAIStateCallbacks[1];

extern s32 ScorpionRobot_is_obj_in_range(Object* self, Object* obj, f32 range);
extern void ScorpionRobot_func_AA0(ScorpionRobot_Data* objdata);
extern void ScorpionRobot_func_B00(ScorpionRobot_Data* objdata);

RECOMP_PATCH void ScorpionRobot_setup(Object* self, Baddie_Setup* setup, s32 reset) {
    u8 baddieFlags;
    Baddie* baddie = self->data;
    ScorpionRobot_Data* objdata;
    
    baddieFlags = 0x2 | 0x4 | 0x10;
    if (reset != 0) {
        baddieFlags |= 1;
    }
    if (!(setup->unk2B & 0x20)) {
        baddieFlags |= 8;
    }
    gDLL_33_BaddieControl->vtbl->setup(self, setup, baddie, 8, 1, 0x108, baddieFlags, 20.0f);
    
    self->animCallback = NULL;

    objdata = baddie->objdata;
    bzero(objdata, sizeof(ScorpionRobot_Data));
    objdata->spin = 0;
    objdata->enteredState = 1;
    objdata->spinSpeed = 0x10E1;

    func_80023D30(self, SCORP_ROBO_MODANIM_2_Fold, 1.0f, 0);
    baddie->fsa.animState = SCORP_ROBO_STATE_0_Spinning;
    baddie->fsa.logicState = 0;
    baddie->fsa.flags |= OBJFSA_FLAG_1000000;
    baddie->fsa.hitpoints = 12;
    baddie->unk3B6 = 0;
    baddie->unk3B4 = 0;
    // @recomp: Enable collider to keep robo inbounds
    //baddie->fsa.unk4.mode = DLL27MODE_DISABLED;
    baddie->fsa.unk4.flags |= DLL27FLAG_2;
    func_800267A4(self);
}

RECOMP_PATCH void ScorpionRobot_control(Object* self) {
    s16 sp90[] = {0x206, 0x167, 0x165, 0x206}; // texture IDs
    Baddie_Setup* setup = (Baddie_Setup*)self->setup;
    Baddie* baddie = self->data;
    ScorpionRobot_Data* objdata = baddie->objdata;
    s32 var_v1;
    Object* player = get_player();
    f32 sp70[3];
    SRT hitSRT;
    s32 sp54;
    s32 _pad;
    
    if (self->unkDC == 0) {
        if (self->unkE0 == 0) {
            self->srt.transl.x = setup->base.x;
            self->srt.transl.y = setup->base.y;
            self->srt.transl.z = setup->base.z;
            gDLL_3_Animation->vtbl->start_obj_sequence(setup->unk2E, self, -1);
            self->unkE0 = 1;
            return;
        }
        if (gDLL_33_BaddieControl->vtbl->func11(self, baddie, 0) != 0) {
            objdata->spin += objdata->spinSpeed;
            objdata->animFinished = func_80024108(self, objdata->animDelta, gUpdateRateF, NULL);
            if (gUpdateRate < objdata->fireCooldown) {
                objdata->fireCooldown -= gUpdateRate;
            } else {
                objdata->fireCooldown = 0;
            }
            if (objdata->animFinished) {
                if (self->curModAnimId == SCORP_ROBO_MODANIM_2_Fold) {
                    func_800267A4(self);
                } else if (self->curModAnimId == SCORP_ROBO_MODANIM_0_Unfold) {
                    func_8002674C(self);
                }
            }
            if (baddie->fsa.target != NULL) {
                sp70[0] = baddie->fsa.target->globalPosition.x - self->globalPosition.x;
                sp70[1] = baddie->fsa.target->globalPosition.y - self->globalPosition.y;
                sp70[2] = baddie->fsa.target->globalPosition.z - self->globalPosition.z;
                baddie->fsa.targetDist = sqrtf(SQ(sp70[0]) + SQ(sp70[1]) + SQ(sp70[2]));
            }
            if (!(baddie->unk3B0 & 0x20)) {
                gDLL_33_BaddieControl->vtbl->func14(self, baddie, &baddie->unk3B2, -1, -1,  baddie->unk3A6, baddie->unk3A4);
            }
            gDLL_33_BaddieControl->vtbl->func20(self, &baddie->fsa, &baddie->unk34C, baddie->unk39E, NULL, 0, 0, 0);
            if (baddie->fsa.hitpoints > 0) {
                if (gDLL_33_BaddieControl->vtbl->check_hit(self, &baddie->fsa, &baddie->unk34C, baddie->unk39E, NULL, sHitDamageMap, 0, &baddie->unk3A8, &hitSRT) != 0) {
                    var_v1 = ((DLL_Unknown*)player->linkedObject->dll)->vtbl->func[19].withOneVoidArgS32(player->linkedObject);
                    if (var_v1 >= 4) {
                        var_v1 = 3;
                    }
                    hitSRT.scale = (f32) sp90[var_v1];
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_323, &hitSRT, PARTFXFLAG_200000 | PARTFXFLAG_1, -1, NULL);
                    hitSRT.transl.x -= self->srt.transl.x;
                    hitSRT.transl.y -= self->srt.transl.y;
                    hitSRT.transl.z -= self->srt.transl.z;
                    hitSRT.scale = (f32) sp90[var_v1];
                    sp54 = 4;
                    while (sp54--) {
                        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_324, &hitSRT, PARTFXFLAG_2, -1, NULL);
                    }
                    gDLL_6_AMSFX->vtbl->play(self, sHitSounds[rand_next(0, 3)], MAX_VOLUME, NULL, NULL, 0, NULL);
                    gDLL_6_AMSFX->vtbl->play(self, SOUND_6E7_ScorpionRobot_Damaged, MAX_VOLUME, NULL, NULL, 0, NULL);

                    // @recomp: Enter damage recoil state
                    gDLL_18_objfsa->vtbl->set_anim_state(self, &baddie->fsa, SCORP_ROBO_STATE_5_DamageRecoil);
                    objdata->enteredState = 1;
                }
            } else {
                if ((baddie->fsa.animState != SCORP_ROBO_STATE_7_Dead) && (baddie->fsa.animState != SCORP_ROBO_STATE_6_Dying)) {
                    gDLL_6_AMSFX->vtbl->play(self, SOUND_6F8_ScorpionRobot_Destroyed, MAX_VOLUME, NULL, NULL, 0, NULL);
                    gDLL_18_objfsa->vtbl->set_anim_state(self, &baddie->fsa, SCORP_ROBO_STATE_6_Dying);
                    objdata->enteredState = 1;
                    baddie->fsa.target = NULL;
                }
            }

            // @recomp: Gravity so robo can move down slopes
            if (baddie->fsa.unk4.mode == DLL27MODE_1) {
                self->velocity.y -= 0.1f * gUpdateRateF;
            }

            gDLL_33_BaddieControl->vtbl->func10(self, &baddie->fsa, 0.0f, -1);
            baddie->unk3AC = self->animObj;
            self->animObj = NULL;
            gDLL_18_objfsa->vtbl->tick(self, &baddie->fsa, gUpdateRateF, gUpdateRateF, sStateCallbacks, sAIStateCallbacks);
            self->animObj = baddie->unk3AC;

            // @recomp: Run collider logic to keep robo from going out of bounds. Note: We do this manually instead of
            //          letting objfsa do it since objfsa has additional velocity logic that conflicts.
            gDLL_27->vtbl->func_1E8(self, &baddie->fsa.unk4, gUpdateRateF);
            gDLL_27->vtbl->func_5A8(self, &baddie->fsa.unk4);
            gDLL_27->vtbl->func_624(self, &baddie->fsa.unk4, gUpdateRateF);
        }
    }
}

RECOMP_PATCH void ScorpionRobot_print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    Baddie* baddie = self->data;
    //s32 _pad;
    ScorpionRobot_Data* objdata = baddie->objdata;

    if ((visibility != 0) && (self->unkDC == 0)) {
        draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);
        if ((objdata->fire) && (baddie->fsa.target != NULL)) {
            gDLL_6_AMSFX->vtbl->play(self, SOUND_115_ScorpionRobot_LaserFire, MAX_VOLUME, NULL, NULL, 0, NULL);

            // @recomp: Actually fire a laser (modified from BigScorpionRobot)
            SRT srcSRT;
            SRT dstSRT;
            ModelInstance* modelInst = self->modelInsts[self->modelInstIdx];
            s32 bone = self->def->pAttachPoints->bones[self->modelInstIdx]; // model index as a bone index?
            MtxF* mtx = (MtxF *)&((f32 **)modelInst->matrices[modelInst->unk34 & 1])[bone << 4];
            DLL_IProjgfx* projgfx = dll_load_deferred(DLL_ID_193, 1);
            srcSRT.transl.x = mtx->m[3][0] + gWorldX;
            srcSRT.transl.y = mtx->m[3][1];
            srcSRT.transl.z = mtx->m[3][2] + gWorldZ;
            srcSRT.scale = 1.0f;
            srcSRT.yaw = 0;
            srcSRT.roll = 0;
            srcSRT.pitch = 0;
            // Use 0.8 here to target a much older player pos, otherwise we are way too accurate
            func_80014D34(0.8f, &dstSRT.transl.x, &dstSRT.transl.y, &dstSRT.transl.z);
            dstSRT.yaw = 0;
            dstSRT.roll = 0;
            dstSRT.pitch = 0;
            dstSRT.scale = 1.0f;
            // Aim a little higher than big robo since we're shooting more laterally than it
            dstSRT.transl.y += 5.0f;
            projgfx->vtbl->func0(baddie->fsa.target, 0, &srcSRT, 1, -1, 7, &dstSRT);
            if (projgfx != NULL) {
                dll_unload(projgfx);
            }
        }
        objdata->fire = 0;
    }
}

RECOMP_PATCH s32 ScorpionRobot_state_0_spinning(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie = self->data;
    ScorpionRobot_Data* objdata = baddie->objdata;
    UnkCurvesStruct* sp54 = baddie->unk3F8;
    f32 sp50 = self->velocity.x;
    f32 sp4C = self->velocity.z;
    f32 temp;
    f32 temp_fs0;
    f32 temp_fa1;
    Object* player = get_player();
    
    fsa->unk341 = 0;
    if (objdata->enteredState) {
        objdata->enteredState = 0;
        baddie->unk3B4 = 0;
        baddie->unk3B6 = 0;
        if ((baddie->unk3F8 != NULL) && (gDLL_26_Curves->vtbl->func_4288(baddie->unk3F8, self, 500.0f, data_24, -1) != 0)) {
            baddie->unk3B2 &= ~8;
            gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, SCORP_ROBO_STATE_1_WaitForPlayer);
            objdata->enteredState = 1;
            return 0;
        }
        baddie->unk3B2 |= 8;
    }

    if (baddie->unk3B2 & 8) {
        temp_fs0 = sp54->unk68.x - self->srt.transl.x;
        temp_fa1 = sp54->unk68.z - self->srt.transl.z;
        temp = 10.0f / sqrtf(SQ(temp_fs0) + SQ(temp_fa1));
        if (((func_800053B0(sp54, temp) != 0) || (sp54->unk10 != 0)) 
                && (gDLL_26_Curves->vtbl->func_4704(sp54) != 0) 
                && (gDLL_26_Curves->vtbl->func_4288(baddie->unk3F8, self, 500.0f, data_24, -1) != 0)) {
            baddie->unk3B2  &= ~8;
            gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, SCORP_ROBO_STATE_1_WaitForPlayer);
            objdata->enteredState = 1;
            return 0;
        }
        temp_fs0 = (sp54->unk68.x - self->srt.transl.x) * 0.02f;
        temp_fa1 = (sp54->unk68.z - self->srt.transl.z) * 0.02f;
        if (temp_fs0 > 0.25f) {
            temp_fs0 = 0.25f;
        } else if (temp_fs0 < -0.25f) {
            temp_fs0 = -0.25f;
        }
        if (temp_fa1 > 0.25f) {
            temp_fa1 = 0.25f;
        } else if (temp_fa1 < -0.25f) {
            temp_fa1 = -0.25f;
        }
        self->velocity.x += temp_fs0;
        self->velocity.z += temp_fa1;
    }

    self->velocity.x *= 0.97f;
    self->velocity.z *= 0.97f;
    fsa->flags |= 0x4000;

    // TODO: the way this calculates the tilt in vanilla is really janky
    temp_fs0 = self->velocity.x - sp50;
    temp_fa1 = self->velocity.z - sp4C;
    s16 prevYaw = self->srt.yaw;
    self->srt.yaw = atan2f_to_s(temp_fs0, temp_fa1);
    // @recomp: Counter yaw change due to tilt to keep spin at a more constant rate
    s32 yawDelta = (u16)rotation16_sub_wrap(prevYaw, self->srt.yaw);
    objdata->spin -= yawDelta;
    
    self->srt.pitch = (s16) (sqrtf((temp_fs0 * temp_fs0) + (temp_fa1 * temp_fa1)) * 10000.0f);
    func_80026128(self, 0xA, 1, -1);
    self->objhitInfo->unk5D = 0xA;
    self->objhitInfo->unk5E = 1;
    func_80028D2C(self);
    obj_move(self, self->velocity.x, self->velocity.y, self->velocity.z);
    if (ScorpionRobot_is_obj_in_range(self, player, (f32) baddie->unk3E2) != 0
            // @recomp: Don't target player if they're dead or untargetable (following BaddieControl logic)
            && ((DLL_210_Player*)player->dll)->vtbl->get_health(player) > 0 
            && ((DLL_210_Player*)player->dll)->vtbl->func66(player, 1) != 0) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, SCORP_ROBO_STATE_2_Unfold);
        objdata->enteredState = 1;
        fsa->target = player;
    }
    ScorpionRobot_func_AA0(objdata);
    return 0;
}

RECOMP_PATCH s32 ScorpionRobot_state_1_wait_for_player(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie = self->data;
    ScorpionRobot_Data* objdata = baddie->objdata;
    Object* player = get_player();
    
    fsa->unk341 = 0;
    if (ScorpionRobot_is_obj_in_range(self, player, (f32) baddie->unk3E2) != 0) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, SCORP_ROBO_STATE_2_Unfold);
        objdata->enteredState = 1;
        fsa->target = player;
    }
    objdata->spinSpeed = 0;
    // @recomp: Disable objhit pushing if we're a static spawn. Prevents the VFP robos from getting flung
    //          by the nearby medium metal crates.
    self->objhitInfo->unk5B = 0;
    return 0;
}

RECOMP_PATCH s32 ScorpionRobot_state_3_attacking(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie = self->data;
    ScorpionRobot_Data* objdata = baddie->objdata;
    s16 temp_v0_4;
    s32 modanimIdx;
    s16 temp;

    fsa->unk341 = 1;
    if (objdata->enteredState) {
        objdata->enteredState = 0;

        // @recomp: Moved initial anim reset to state entry
        func_80023D30(self, SCORP_ROBO_MODANIM_0_Unfold, 1.0f, 0);
        objdata->animDelta = 0.0f;
    }
    self->velocity.x *= 0.85f;
    self->velocity.z *= 0.85f;
    fsa->flags |= 0x4000;
    self->srt.pitch -= (self->srt.pitch >> 2);
    func_80026128(self, 0xA, 1, -1);
    // @recomp: Disable touch damage while standing still (still active during unfold and turning)
    if ((self->curModAnimId == SCORP_ROBO_MODANIM_0_Unfold) && (self->animProgress == 1.0f)) {
        self->objhitInfo->unk5D = 0;
        self->objhitInfo->unk5E = 0;
    } else {
        self->objhitInfo->unk5D = 0xA;
        self->objhitInfo->unk5E = 1;
    }
    func_80028D2C(self);
    obj_move(self, self->velocity.x, self->velocity.y, self->velocity.z);
    if (objdata->animFinished) {
        // @recomp: Don't always reset to anim 0 when an anim finishes (causes stutter during turn)
        //func_80023D30(self, SCORP_ROBO_MODANIM_0_Unfold, 1.0f, 0);
        objdata->animDelta = 0.0f;
    }
    if ((self->curModAnimId == SCORP_ROBO_MODANIM_7_TurnLeft) || (self->curModAnimId == SCORP_ROBO_MODANIM_6_TurnRight)) {
        // @recomp: The turning anims are set as looped anims, so anim progress will roll over
        //          to zero and screw up the turning logic here due to other edits we made. If
        //          the anim finished/looped, assume 1.0 progress.
        f32 progress = !objdata->animFinished ? self->animProgress : 1.0f;
        self->srt.yaw = (s16) ((f32) objdata->turnStart + (5461.3335f * (f32) objdata->turnDir * progress));
    }
    // @recomp: Allow entering this block at the end of the turning anims
    //if ((self->curModAnimId == SCORP_ROBO_MODANIM_0_Unfold) && (self->animProgress == 1.0f)) {
    if (objdata->animFinished || self->animProgress == 1.0f) {
        if (fsa->target != NULL) {
            temp = (u16)arctan2_f(
                    self->srt.transl.x - fsa->target->srt.transl.x, 
                    self->srt.transl.z - fsa->target->srt.transl.z);
            temp_v0_4 = (u16)rotation16_sub_wrap(
                temp, 
                self->srt.yaw + objdata->spin);
            // @recomp: Require a tighter angle to the player (careful not to go too low, otherwise 
            //          they can get stuck in a turning loop)
            if (temp_v0_4 > 0xC00) {
                modanimIdx = SCORP_ROBO_MODANIM_6_TurnRight;
                objdata->turnDir = -1;
            } else if (temp_v0_4 < -0xC00) {
                modanimIdx = SCORP_ROBO_MODANIM_7_TurnLeft;
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
                objdata->fireCooldown = rand_next(0x5A, 0xD2);
                objdata->fire = 1;
                objdata->animDelta = 0.04f;
                func_80023D30(self, SCORP_ROBO_MODANIM_3_Firing, 0.0f, 0);
            } else {
                // @recomp: Moved anim reset down here so there's no stutter when turning more than once in a row
                if (self->curModAnimId != 0) {
                    func_80023D30(self, SCORP_ROBO_MODANIM_0_Unfold, 1.0f, 0);
                    objdata->animDelta = 0.0f;
                }
            }
        }
    }
    if (ScorpionRobot_is_obj_in_range(self, fsa->target, (f32) baddie->unk3E2 * 1.5f) == 0) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, SCORP_ROBO_STATE_4_Fold);
        objdata->enteredState = 1;
        fsa->target = NULL;
    }
    ScorpionRobot_func_B00(objdata);
    return 0;
}

RECOMP_PATCH s32 ScorpionRobot_state_5_damage_recoil(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie = self->data;
    ScorpionRobot_Data* objdata = baddie->objdata;

    fsa->unk341 = 0;
    if (objdata->enteredState) {
        objdata->enteredState = 0;
        func_80023D30(self, SCORP_ROBO_MODANIM_4_DamageRecoil, 0.0f, 0);
        //objdata->unk0 = 0.02f;
        objdata->animDelta = 0.02f * 0.5f; // @recomp: Slow down anim a little
    }
    func_80026128(self, 0xA, 1, -1);
    // self->objhitInfo->unk5D = 0xA;
    // self->objhitInfo->unk5E = 1;
    // @recomp: Disable touch damage while recoiling
    self->objhitInfo->unk5D = 0;
    self->objhitInfo->unk5E = 0;
    func_80028D2C(self);
    if ((self->curModAnimId == SCORP_ROBO_MODANIM_4_DamageRecoil) && (self->animProgress == 1.0f)) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, SCORP_ROBO_STATE_3_Attacking);
        objdata->enteredState = 1;
    }
    ScorpionRobot_func_B00(objdata);
    return 0;
}

RECOMP_PATCH s32 ScorpionRobot_state_6_dying(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie = self->data;
    ScorpionRobot_Data* objdata = baddie->objdata;

    fsa->unk341 = 0;
    if (objdata->enteredState) {
        objdata->enteredState = 0;
        func_80023D30(self, SCORP_ROBO_MODANIM_5_Die, 0.0f, 0);
        objdata->animDelta = 0.0042f;

        // @recomp: Disable touch damage while dying
        self->objhitInfo->unk5D = 0;
        self->objhitInfo->unk5E = 0;
    }
    self->velocity.x *= 0.97f;
    self->velocity.z *= 0.97f;
    fsa->flags |= OBJSTATE_PRINT_DISABLED;
    self->srt.pitch -= (self->srt.pitch >> 2);
    obj_move(self, self->velocity.x, self->velocity.y, self->velocity.z);
    if ((self->curModAnimId == SCORP_ROBO_MODANIM_5_Die) && (self->animProgress == 1.0f)) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, SCORP_ROBO_STATE_7_Dead);
        objdata->enteredState = 1;
        fsa->target = NULL;
    }
    ScorpionRobot_func_B00(objdata);
    return 0;
}

RECOMP_PATCH s32 ScorpionRobot_state_7_dead(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie = self->data;
    ScorpionRobot_Data* objdata = baddie->objdata;

    fsa->unk341 = 3;
    if (objdata->enteredState) {
        objdata->enteredState = 0;

        // @recomp: Explode into particles
        s32 partCount = 30;
        while (partCount--) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_555, NULL, PARTFXFLAG_2, -1, NULL);
        }

        gDLL_33_BaddieControl->vtbl->func18(self, baddie->unk3E0, -1, 0);
        gDLL_18_objfsa->vtbl->func21(self, fsa, 0x3C, 0xA, 0);
        baddie->unk3B4 = 0;
        main_set_bits(baddie->unk39C, 1);
        if (self->setup == NULL) {
            obj_destroy_object(self);
        }
        // Tell player to break z-lock
        obj_send_mesg_many(0, OBJMSG_SEND_ALL | OBJMSG_SEND_IGNORE_SENDER, self, 0xE0000, self);
        // @recomp: Disable z-lock
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
    }

    if (self->opacity >= (gUpdateRate * 3)) {
        self->opacity -= (gUpdateRate * 3);
    } else {
        self->opacity = 0;
    }
    ScorpionRobot_func_B00(objdata);
    return 0;
}
