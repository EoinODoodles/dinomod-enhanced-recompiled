#include "modding.h"
#include "recomputils.h"

#include "common_objsetups.h"

#include "dlls/engine/27.h"
#include "dlls/engine/53_movelib.h"
#include "dlls/objects/common/collectable.h"
#include "game/gamebits.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "sys/objmsg.h"
#include "sys/print.h"
#include "sys/joypad.h"
#include "sys/rand.h"
#include "dll.h"
#include "macros.h"

#include "recomp/dlls/objects/369_CFGuardian_recomp.h"

typedef struct {
/*000*/ MoveLibData movedata;
/*4B8*/ HeadAnimation exprHeadAnim;
/*4DC*/ HeadAnimation eyeIdleHeadAnim;
/*500*/ u32 unk500; // unused sound handle
/*504*/ u8 _unk504[0x50C - 0x504];
/*50C*/ Object* dustObjs[6];
/*524*/ Collectable_Setup* dustSetups[6];
/*53C*/ UnkCurvesStruct unk53C;
/*644*/ u8 _unk644[0x67C - 0x644];
/*67C*/ f32 animRate;
/*680*/ DLL27_Data collider;
/*8E0*/ SRT walkTarget;
/*8F8*/ u8 state;
/*8F9*/ u8 _unk8F9[0x908 - 0x8F9];
/*908*/ s32 unk908;
/*90C*/ s32 windLiftState;
/*910*/ u8 talkState;
/*911*/ s8 talkSeqSelector;
/*912*/ u8 flags;
} CFGuardian_Data;

enum CFGuardianState {
    CFGUARDIAN_STATE_InCell = 0,
    CFGUARDIAN_STATE_WaitingToBeFreed = 1,
    CFGUARDIAN_STATE_LeavingCell = 2, // walking through prison
    CFGUARDIAN_STATE_WaitingAtWindLift = 3,
    CFGUARDIAN_STATE_WaitingForWindLiftPower = 4,
    CFGUARDIAN_STATE_5 = 5, // unused
    CFGUARDIAN_STATE_InWindLift = 6,
    CFGUARDIAN_STATE_LeavingWindLift = 7, // leaving top of wind lift
    CFGUARDIAN_STATE_Courtyard_WaitingForKyte = 8, // waiting for kyte to be freed
    CFGUARDIAN_STATE_Courtyard_WaitingForCloudBaby = 9, // waiting for cloud baby rescue
    CFGUARDIAN_STATE_WalkingToRaceArea = 10, // walk to drained water
    CFGUARDIAN_STATE_Vanish = 11, // vanish (after reaching drained water)
    CFGUARDIAN_STATE_WaitingForRaceCompletion = 12,
    CFGUARDIAN_STATE_WaitingForSpellStone = 13,
    CFGUARDIAN_STATE_NoOp_SpellStoneActivated = 14,
    CFGUARDIAN_STATE_NoOp_Vanished = 15
};

enum CFGuardianModAnim {
    CFGUARDIAN_MODANIM_Idle = 0,
    CFGUARDIAN_MODANIM_1 = 1, // grunt? used when giving krystal the illusion spell?
    CFGUARDIAN_MODANIM_WalkSlow = 2,
    CFGUARDIAN_MODANIM_Run = 3,
    CFGUARDIAN_MODANIM_4 = 4, // charge (running with head down)
    CFGUARDIAN_MODANIM_5 = 5, // charge start?
    CFGUARDIAN_MODANIM_6 = 6, // charge end?
    CFGUARDIAN_MODANIM_7 = 7, // wobbly idle right
    CFGUARDIAN_MODANIM_8 = 8, // wobbly idle left
    CFGUARDIAN_MODANIM_Floating = 9,
    CFGUARDIAN_MODANIM_10 = 10, // look right
    CFGUARDIAN_MODANIM_11 = 11, // look right -> forward
    CFGUARDIAN_MODANIM_12 = 12, // head down, point
    CFGUARDIAN_MODANIM_13 = 13, // idle after giving something?
    CFGUARDIAN_MODANIM_14 = 14, // idle, head down a little to be sneaky (used in courtyard)
    CFGUARDIAN_MODANIM_SummoningStart = 15,
    CFGUARDIAN_MODANIM_SummoningLoop = 16,
    CFGUARDIAN_MODANIM_SummoningEnd = 17,
    CFGUARDIAN_MODANIM_Stretch = 18, // big stretch, right leg
    CFGUARDIAN_MODANIM_Jump = 19,
    CFGUARDIAN_MODANIM_DrinkingStart = 20,
    CFGUARDIAN_MODANIM_DrinkingLoop = 21,
    CFGUARDIAN_MODANIM_DrinkingStop = 22,
    CFGUARDIAN_MODANIM_23 = 23, // something with his hands
    CFGUARDIAN_MODANIM_24 = 24, // giving something from his invisible "bag"
    CFGUARDIAN_MODANIM_25 = 25, // grunt? other direction
    CFGUARDIAN_MODANIM_Walk = 26
};

enum CFGuardianSeq {
    CFGUARDIAN_SEQ_0 = 0,
    CFGUARDIAN_SEQ_GiveIllusionSpell = 1,
    CFGUARDIAN_SEQ_GivePowerRoomKey = 2,
    CFGUARDIAN_SEQ_3 = 3, // walk through prison? unused?
    CFGUARDIAN_SEQ_IllusionSpellHint = 4,
    CFGUARDIAN_SEQ_LetsGetOutOfHere = 5,
    CFGUARDIAN_SEQ_SummoningBoneDust = 6,
    CFGUARDIAN_SEQ_KyteLocationHint = 7,
    CFGUARDIAN_SEQ_WindLiftPowerHint = 8,
    CFGUARDIAN_SEQ_BabyCloudRunnerReminder = 9,
    CFGUARDIAN_SEQ_SpellStoneActivationReminder = 10,
    CFGUARDIAN_SEQ_SpellStoneRaceReminder = 11,
    CFGUARDIAN_SEQ_PowerCrystalsHint = 12
};

enum CFGuardianFlags {
    CFGUARDIAN_FLAG_1 = 0x1,
    CFGUARDIAN_FLAG_FollowingCurvePath = 0x2,
    CFGUARDIAN_FLAG_WalkToTarget = 0x4,
};

enum CFGuardianTalkState {
    CFGUARDIAN_TALK_Disabled = 0,
    CFGUARDIAN_TALK_Enabled = 1,
    CFGUARDIAN_TALK_SpokenTo = 2
};

// size: 0xC
typedef struct {
/*0*/ s32 key;
/*4*/ s32 values[2];
} CFGuardianMapStruct;

extern Vec3f sColliderTestPoints[4];
extern f32 sColliderTestRadii[4];

extern s32 sStateAnimMap[];
extern CFGuardianMapStruct sTalkSeqStateMap[];
extern s32 sTalkSeqStateMapLength;
extern s32 sAnimTransitionMapLength;
extern CFGuardianMapStruct sAnimTransitionMap[];
extern u16 sModAnimSfx[];
extern s16 sExprSfx[][2];

extern int CFGuardian_animCallback(Object* actor, Object* animObj, AnimObj_Data* animObjData, s8);
extern s32 CFGuardian_updateBoneDust(Object* self, Object** dustObjs, s16 rotX, s16 rotY, s16 rotZ, s32 count);
extern SRT* CFGuardian_curveToWalkTarget(CurveSetup* curve, SRT* srt);
extern CurveSetup* CFGuardian_findCurveNode(Object* self, s32 curveTag, Vec3f* pos, s32 arg3);
extern s32 CFGuardian_walkTo(Object* self, SRT* target, f32 speed, f32* animChange);
extern s32 CFGuardian_followCurvePath(Object* self, UnkCurvesStruct* arg1, f32 speed, u8 curveTag, f32* animChange);
extern s32 CFGuardian_mapLookup(CFGuardianMapStruct* map, s32 key, s32 mapLength, s32 selector);
extern s32 CFGuardian_doModAnimSfx(Object* self, UnkFunc_80024108Struct* animState, u16* sounds);
extern void CFGuardian_func_25AC(Object* self, UnkCurvesStruct* arg1, s32 arg2, s32 arg3, f32 arg4);;

RECOMP_PATCH void CFGuardian_obj_Setup(Object* self, CFGuardian_Setup* setup, s32 reset) {
    CFGuardian_Data* objdata;
    s32 _pad;
    s32 _pad2;
    u8 sp48[] = {0x00, 0x01, 0x06, 0x06};
    s16 sp3C[] = {0x0005, 0x000f, 0x000f, 0x0000, 0x0000};
    u8 sp38[4] = {1, 1, 1, 1};

    objdata = self->data;
    if (objdata != NULL) {
        objInitMesgQueue(self, 4);
        objAddObjectType(self, OBJTYPE_WindLiftable);
        objdata->state = mainGetBits(BIT_CFGuardian_State);
        recomp_printf(" Initalise Guardian State %i \n", objdata->state); // @recomp
        recomp_printf(" GUARDIAN POS : %f %f %f \n", setup->base.x, setup->base.y, setup->base.z); // @recomp
        self->srt.transl.x = setup->base.x;
        self->srt.transl.y = setup->base.y;
        self->srt.transl.z = setup->base.z;
        self->unkDC = 1;
        // @recomp: Correctly handle starting off in path following states, otherwise the curve path
        //          struct won't get initialized correctly and the guardian will warp to 0,0,0.
        switch (objdata->state) {
        case CFGUARDIAN_STATE_LeavingCell:
            CFGuardian_func_25AC(self, &objdata->unk53C, /*curves excluding the start*/1, 0, 1000.0f);
            break;
        case CFGUARDIAN_STATE_LeavingWindLift:
            self->unkDC = 0;
            break;
        case CFGUARDIAN_STATE_WalkingToRaceArea:
            CFGuardian_func_25AC(self, &objdata->unk53C, /*curves excluding the start*/1, 2, 1000.0f);
            break;
        }
        self->animCallback = (AnimationCallback)CFGuardian_animCallback;
        self->srt.yaw = setup->yaw8 << 8;
        objdata->windLiftState = 0;
        objdata->unk908 = 6;
        objdata->flags = 0;
        objdata->animRate = 0.0f;
        gDLL_27->vtbl->init(&objdata->collider, 
            DLL27FLAG_2000000, 
            DLL27FLAG_40000 | DLL27FLAG_HAS_TERRAIN_COLLIDER | DLL27FLAG_80 | DLL27FLAG_2 | DLL27FLAG_1, 
            DLL27MODE_1);
        gDLL_27->vtbl->setup_terrain_collider(&objdata->collider, 4, sColliderTestPoints, sColliderTestRadii, sp38);
        gDLL_27->vtbl->setup_hits_collider(&objdata->collider, 4, sColliderTestPoints, sColliderTestRadii, 8);
        objdata->movedata.unk4A9 |= 0x28;
        objdata->talkState = CFGUARDIAN_TALK_Enabled;
        objdata->talkSeqSelector = 0;
        mainCreateTempDLL(DLL_ID_MOVELIB); // @recomp: Moved to here so the below code has access to movelib
        if (mainGetBits(BIT_CRF_WindLifts_Powered) != 0) {
            objdata->state = CFGUARDIAN_STATE_InWindLift;
        }
        // @recomp: Don't return to windlift state if we already exited it
        if (mainGetBits(BIT_CRF_Prison_Guardian_Exited_WindLift) != 0) {
            if (mainGetBits(BIT_CF_Floor_Destroyed) != 0) {
                objdata->state = CFGUARDIAN_STATE_Courtyard_WaitingForCloudBaby;
                // @recomp: Due to other changes below, it's possible for this bit to still be set at this point. We only want
                //          the forced talkseq to play for the Kyte hint so just clear it if Kyte was freed.
                mainSetBits(BIT_Force_CFGuardian_TalkSeq, 0);
            } else {
                objdata->state = CFGUARDIAN_STATE_Courtyard_WaitingForKyte;
            }
        }
        
        // @recomp: If the water draining seq starts playing but the player left the guardian in the wind lift,
        //          just move him into the courtyard anyway so the cutscene looks right.
        if (self->mapID != MAP_CLOUDRUNNER_DUNGEON && mainGetBits(BIT_CRF_Throne_Room_Quest_Complete) != 0) {
            if (mainGetBits(BIT_CRF_Prison_Guardian_Exited_WindLift) == 0) {
                SRT transform;
                if (((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func7(14, &transform)) {
                    self->srt.transl = transform.transl;
                    self->srt.yaw = transform.yaw;

                    objdata->state = CFGUARDIAN_STATE_WalkingToRaceArea;
                    self->unkDC = 0;
                }
            }
        }
        if (mainGetBits(BIT_Played_Seq_02B2_CF_Courtyard_Water_Drains) != 0) {
            // @recomp: Don't use race map states for the dungeon copy of the guardian
            if (self->mapID == MAP_CLOUDRUNNER_DUNGEON) {
                objdata->state = CFGUARDIAN_STATE_Vanish;
            } else {
                objdata->state = CFGUARDIAN_STATE_WaitingForRaceCompletion;
            }
        }
        func_8002674C(self);
        //mainCreateTempDLL(DLL_ID_MOVELIB); // @recomp: Moved to earlier in func
        ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func2(self, &objdata->movedata, -0x1FFF, 0x2800, 3);
        ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func5(&objdata->movedata, 0x12C, 0x78);
        ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func6(&objdata->movedata, 0, sp3C, 3);
    }
}

RECOMP_PATCH s32 CFGuardian_control(Object* self) {
    CFGuardian_Data* objdata;
    Object* player;
    Object* nearbyBaddie;
    f32 baddieDist;
    f32 trackHeight;
    s32 modAnimId;
    s32 seqno2;
    s32 seqno;
    u32 mesgID;
    void* mesgArg;
    s32 sp6C;
    UnkFunc_80024108Struct animState;
    Vec3f sp44;
    f32 var_fa0;
    // @recomp: new vars
    f32 playerDist;

    mesgID = 0;
    mesgArg = NULL;
    sp6C = 1;
    baddieDist = 1000.0f;
    trackHeight = 1.0f;
    objdata = self->data;
    objdata->flags &= ~CFGUARDIAN_FLAG_FollowingCurvePath;
    diPrintf("Guardian ");
    objdata->animRate = 0.005f;
    player = objGetPlayer();
    switch (objdata->state) {
    case CFGUARDIAN_STATE_InCell:
        if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->state = CFGUARDIAN_STATE_WaitingToBeFreed;
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
        }
        sp6C = 0;
        break;
    case CFGUARDIAN_STATE_WaitingToBeFreed:
        if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
        }
        if (mainGetBits(BIT_CRF_Prison_Guardian_Cell_Door_Open) != 0) {
            objdata->state = CFGUARDIAN_STATE_LeavingCell;
            objAnimSet(self, CFGUARDIAN_MODANIM_Walk, 0, 0);
            self->unkDC = 0;
            mainSetBits(BIT_CRF_BoneHead_Guardian_Freed, 1);
        }
        sp6C = 0;
        break;
    case CFGUARDIAN_STATE_LeavingCell:
        if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
        }
        objdata->flags |= CFGUARDIAN_FLAG_FollowingCurvePath;
        if (CFGuardian_followCurvePath(self, &objdata->unk53C, 0.7f, 0, &objdata->animRate) != 0) {
            objdata->state = CFGUARDIAN_STATE_WaitingAtWindLift;
        }
        break;
    case CFGUARDIAN_STATE_WaitingAtWindLift:
        // @recomp: if the player talks to the guardian before it automatically starts the seq, the key won't be given
        //          and then the automatic code will play it a second time. just handle the talk interaction correctly.
        if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
            mainSetBits(BIT_CRF_Power_Room_Key, 1);
            objdata->state = CFGUARDIAN_STATE_WaitingForWindLiftPower;
        } else if ((objdata->movedata.unk498 == 1) && (player == objdata->movedata.prevLookat) && (vec3Distance(&self->globalPosition, &player->globalPosition) < 80.0f)) {
            gDLL_3_Animation->vtbl->start_obj_sequence(CFGUARDIAN_SEQ_GivePowerRoomKey, self, -1);
            mainSetBits(BIT_CRF_Power_Room_Key, 1);
            objdata->state = CFGUARDIAN_STATE_WaitingForWindLiftPower;
        }
        break;
    case CFGUARDIAN_STATE_WaitingForWindLiftPower:
        if (mainGetBits(BIT_CRF_WindLifts_Powered) != 0) {
            objdata->state = CFGUARDIAN_STATE_InWindLift;
            objdata->talkSeqSelector = 0;
        } else if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
            objdata->talkSeqSelector = (objdata->talkSeqSelector + 1) % 2;
        }
        break;
    case CFGUARDIAN_STATE_InWindLift:
        diPrintf(" UpWind Lift ");
        if (objdata->windLiftState != 0) {
            if (objdata->windLiftState >= 2) {
                self->velocity.x = 0;
                self->velocity.z = 0;
                self->srt.transl.y += self->velocity.y * gUpdateRateF;
                trackGetHeightNearest(self, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, &trackHeight, 0);
                self->srt.yaw = (0xC0 << (self->srt.yaw + 8)) >> 1;
                self->objhitInfo->unk58 &= ~0x400;
                if (trackHeight <= 1.0f) {
                    STUBBED_PRINTF(" LANDING ");
                    objdata->windLiftState = 2;
                    self->srt.transl.y -= trackHeight;
                    objdata->talkState = CFGUARDIAN_TALK_Disabled;
                    self->unkDC = 0;
                    objAnimSet(self, CFGUARDIAN_MODANIM_Idle, 0, 0);
                    CFGuardian_curveToWalkTarget(CFGuardian_findCurveNode(self, 0, NULL, 2), &objdata->walkTarget);
                    if (self->srt.transl.y <= objdata->walkTarget.transl.y) {
                        var_fa0 = objdata->walkTarget.transl.y - self->srt.transl.y;
                    } else {
                        var_fa0 = -(objdata->walkTarget.transl.y - self->srt.transl.y);
                    }
                    if (var_fa0 < 150.0f) {
                        // @recomp: Don't become liftable again once exited, otherwise the windlift will constantly
                        //          try to lift the guardian while they attempt to walk away. This objtype isn't
                        //          needed again after this point.
                        //objAddObjectType(self, OBJTYPE_WindLiftable);
                        objdata->state = CFGUARDIAN_STATE_LeavingWindLift;
                        objAnimSet(self, CFGUARDIAN_MODANIM_Walk, 0, 0);
                    }
                // @recomp: Don't gain speed when already on the floor, otherwise the guardian will clip through
                //          the windlift grate if the lift is reversed and he falls. (This patch is from default.dol!)
                } else {
                    self->velocity.y -= 0.12f;
                }
            } else {
                var_fa0 = ABS_EXPR(self->velocity.y * 400.0f);
                self->srt.yaw += var_fa0;
                objdata->animRate = 0.04f;
                if (mainGetBits(BIT_CRF_Prison_Guardian_Exited_WindLift) != 0) {
                    STUBBED_PRINTF("Guardian Out of WindLIft Boyo !!! ");
                    objAnimSet(self, CFGUARDIAN_MODANIM_Idle, 0, 0);
                    objAnim_func_80024D74(self, 0x32);
                    self->velocity.y = 0;
                    objFreeObjectType(self, OBJTYPE_WindLiftable);
                    self->velocity.x = 0;
                    self->velocity.y = -0.001f;
                    self->velocity.z = 0;
                    objdata->windLiftState = 2;
                    objdata->flags &= ~CFGUARDIAN_FLAG_1;
                }
            }
            if (objdata->windLiftState < 2) {
                self->srt.transl.x += gUpdateRateF * self->velocity.x;
                self->srt.transl.z += gUpdateRateF * self->velocity.z;
                gDLL_27->vtbl->func_1E8(self, &objdata->collider, gUpdateRateF);
                gDLL_27->vtbl->func_5A8(self, &objdata->collider);
                gDLL_27->vtbl->func_624(self, &objdata->collider, gUpdateRateF);
                if (objdata->collider.hitsTouchBits != 0) {
                    self->velocity.x = -self->velocity.x * 0.8f;
                    self->velocity.z = -self->velocity.z * 0.8f;
                }
                sp44.f[0] = self->srt.transl.x - self->prevLocalPosition.x;
                sp44.f[1] = self->srt.transl.y - self->prevLocalPosition.y;
                sp44.f[2] = self->srt.transl.z - self->prevLocalPosition.z;
                sp44.f[0] *= 0.95f * (1.0f / gUpdateRateF);
                sp44.f[1] *= 0.95f * (1.0f / gUpdateRateF);
                sp44.f[2] *= 0.95f * (1.0f / gUpdateRateF);
                self->velocity.x += sp44.f[0];
                self->velocity.y += sp44.f[1];
                self->velocity.z += sp44.f[2];
                self->velocity.x *= 0.3f;
                self->velocity.y *= 0.3f;
                self->velocity.z *= 0.3f;
                diPrintf(" Xvel %f Zvel %f \n", &self->velocity.x, &self->velocity.z);
            }
        } else if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
        }
        break;
    case CFGUARDIAN_STATE_LeavingWindLift:
        if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
        }
        objdata->flags |= CFGUARDIAN_FLAG_FollowingCurvePath;
        if (CFGuardian_followCurvePath(self, &objdata->unk53C, 0.3f, 1, &objdata->animRate) != 0) {
            objdata->state = CFGUARDIAN_STATE_Courtyard_WaitingForKyte;
            objAnim_func_80024D74(self, 0x32);
        }
        break;
    case CFGUARDIAN_STATE_Courtyard_WaitingForKyte:
        nearbyBaddie = objGetNearestTypeTo(OBJTYPE_Baddie, self, &baddieDist);
        if ((nearbyBaddie != NULL) && (baddieDist < 200.0f)) {
            ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func1(&objdata->movedata, nearbyBaddie);
            self->unkAF |= ARROW_FLAG_10_Greyed_Out;
        }
        // @recomp: Less janky distance check
        playerDist = vec3DistanceXZ(&player->globalPosition, &self->globalPosition);
        if ((baddieDist > 200.0f) && (playerDist < 80.0f)) {
            self->unkAF &= ~ARROW_FLAG_10_Greyed_Out;
            if (!(objdata->flags & CFGUARDIAN_FLAG_WalkToTarget) && (sStateAnimMap[objdata->state] != CFGUARDIAN_MODANIM_Idle)) {
                STUBBED_PRINTF(" Stand ");
                ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func9(15, &objdata->walkTarget);
                objdata->flags |= (CFGUARDIAN_FLAG_WalkToTarget | CFGUARDIAN_FLAG_1);
                sStateAnimMap[objdata->state] = CFGUARDIAN_MODANIM_Idle;
            }
            if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
                objdata->talkState = CFGUARDIAN_TALK_Enabled;
                objdata->talkSeqSelector = (objdata->talkSeqSelector + 1) % 2;
            }
        } else if ((baddieDist <= 200.0f) || (playerDist >= 135.0f)) { // @recomp: new condition
            if (!(objdata->flags & CFGUARDIAN_FLAG_WalkToTarget) && (sStateAnimMap[objdata->state] != CFGUARDIAN_MODANIM_14)) {
                STUBBED_PRINTF(" Idle Tow ");
                objdata->talkState = CFGUARDIAN_TALK_SpokenTo;
                objdata->flags |= (CFGUARDIAN_FLAG_WalkToTarget | CFGUARDIAN_FLAG_1);
                ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func7(14, &objdata->walkTarget);
                sStateAnimMap[objdata->state] = CFGUARDIAN_MODANIM_14;
            }
        }
        if ((objdata->flags & CFGUARDIAN_FLAG_WalkToTarget) && (CFGuardian_walkTo(self, &objdata->walkTarget, 0.5f, &objdata->animRate) != 0)) {
            objAnimSet(self, CFGUARDIAN_MODANIM_Walk, 0, 0);
            objdata->flags &= ~(CFGUARDIAN_FLAG_WalkToTarget | CFGUARDIAN_FLAG_1);
        }
        if (mainGetBits(BIT_CF_Floor_Destroyed) != 0) {
            objdata->state = CFGUARDIAN_STATE_Courtyard_WaitingForCloudBaby;
            objdata->talkSeqSelector = 0;
            // @recomp: Clear for the same reasons as in obj_Setup
            mainSetBits(BIT_Force_CFGuardian_TalkSeq, 0);
        }
        break;
    case CFGUARDIAN_STATE_Courtyard_WaitingForCloudBaby:
        nearbyBaddie = objGetNearestTypeTo(OBJTYPE_Baddie, self, &baddieDist);
        if ((nearbyBaddie != NULL) && (baddieDist < 200.0f)) {
            ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func1(&objdata->movedata, nearbyBaddie);
            // @bug: if the target arrow was greyed out due to nearby baddies in the previous state,
            //       the arrow will remain greyed out even if the baddies move away as this state
            //       never clears the flag. this makes it impossible to initiate this state's talk seq
            // @recomp: fix the above mentioned bug
            self->unkAF |= ARROW_FLAG_10_Greyed_Out;
        }
        // @recomp: fix the above mentioned bug
        self->unkAF &= ~ARROW_FLAG_10_Greyed_Out;
        // @recomp: Less janky distance check
        playerDist = vec3DistanceXZ(&player->globalPosition, &self->globalPosition);
        if ((baddieDist > 200.0f) && (playerDist < 80.0f)) {
            if (!(objdata->flags & CFGUARDIAN_FLAG_WalkToTarget) && (sStateAnimMap[objdata->state] != CFGUARDIAN_MODANIM_Idle)) {
                STUBBED_PRINTF(" Stand ");
                ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func9(15, &objdata->walkTarget);
                objdata->flags |= (CFGUARDIAN_FLAG_WalkToTarget | CFGUARDIAN_FLAG_1);
                sStateAnimMap[objdata->state] = CFGUARDIAN_MODANIM_Idle;
            }
            if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
                objdata->talkState = CFGUARDIAN_TALK_Enabled;
                objdata->talkSeqSelector = (objdata->talkSeqSelector + 1) % 2;
            }
        } else if ((baddieDist <= 200.0f) || (playerDist >= 135.0f)) { // @recomp: new condition
            if (!(objdata->flags & CFGUARDIAN_FLAG_WalkToTarget) && (sStateAnimMap[objdata->state] != CFGUARDIAN_MODANIM_14)) {
                STUBBED_PRINTF(" Idle Tow ");
                objdata->talkState = CFGUARDIAN_TALK_SpokenTo;
                objdata->flags |= (CFGUARDIAN_FLAG_WalkToTarget | CFGUARDIAN_FLAG_1);
                ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func7(14, &objdata->walkTarget);
                sStateAnimMap[objdata->state] = CFGUARDIAN_MODANIM_14;
            }
        }
        if ((objdata->flags & CFGUARDIAN_FLAG_WalkToTarget) && (CFGuardian_walkTo(self, &objdata->walkTarget, 0.5f, &objdata->animRate) != 0)) {
            objAnimSet(self, CFGUARDIAN_MODANIM_Walk, 0, 0);
            objdata->flags &= ~(CFGUARDIAN_FLAG_WalkToTarget | CFGUARDIAN_FLAG_1);
        }
        if (mainGetBits(BIT_CRF_Throne_Room_Quest_Complete) != 0) {
            objdata->state = CFGUARDIAN_STATE_WalkingToRaceArea;
            objAnimSet(self, CFGUARDIAN_MODANIM_Walk, 0, 0);
            self->unkDC = 0;
        }
        break;
    case CFGUARDIAN_STATE_WalkingToRaceArea:
        if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
        }
        objdata->flags |= CFGUARDIAN_FLAG_FollowingCurvePath;
        if (CFGuardian_followCurvePath(self, &objdata->unk53C, 0.6f, 2, &objdata->animRate) != 0) {
            objdata->state = CFGUARDIAN_STATE_Vanish;
        }
        break;
    case CFGUARDIAN_STATE_Vanish:
        if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
        }
        self->opacity = 0;
        self->objhitInfo->unk58 &= ~0x1;
        objDisable(self);
        self->srt.flags |= OBJSTATE_PRINT_DISABLED;
        objdata->state = CFGUARDIAN_STATE_NoOp_Vanished;
        break;
    case CFGUARDIAN_STATE_WaitingForRaceCompletion:
        if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
        }
        if (mainGetBits(BIT_CRF_Race_Guardian_Reminder) != 0) {
            // player trying to leave!
            gDLL_2_Camera->vtbl->set_target_object(self);
            gDLL_3_Animation->vtbl->start_obj_sequence(CFGUARDIAN_SEQ_SpellStoneRaceReminder, self, -1);
            mainSetBits(BIT_CRF_Race_Guardian_Reminder, 0);
        }
        if (mainGetBits(BIT_Play_Seq_02A9_CF_Race_End) != 0) {
            objdata->state = CFGUARDIAN_STATE_WaitingForSpellStone;
        }
        break;
    case CFGUARDIAN_STATE_WaitingForSpellStone:
        if (objdata->talkState == CFGUARDIAN_TALK_SpokenTo) {
            objdata->talkState = CFGUARDIAN_TALK_Enabled;
        }
        if (mainGetBits(BIT_CRF_Race_Guardian_Reminder) != 0) {
            // player trying to leave!
            gDLL_2_Camera->vtbl->set_target_object(self);
            gDLL_3_Animation->vtbl->start_obj_sequence(CFGUARDIAN_SEQ_SpellStoneActivationReminder, self, -1);
            mainSetBits(BIT_CRF_Race_Guardian_Reminder, 0);
        }
        if (mainGetBits(BIT_Played_Seq_02AB_CF_SpellStone_Activation) != 0) {
            objdata->state = CFGUARDIAN_STATE_NoOp_SpellStoneActivated;
        }
        break;
    case CFGUARDIAN_STATE_NoOp_SpellStoneActivated:
        break;
    }
    STUBBED_PRINTF("GD"); // unknown location
    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func0(self, &objdata->movedata);
    while (objRecvMesg(self, &mesgID, NULL, &mesgArg) != 0) {
        switch (mesgID) {
        case 15:
            // enter windlift
            STUBBED_PRINTF(" Guardian In Elevatoe "); // guessed location
            objdata->state = CFGUARDIAN_STATE_InWindLift;
            objAnimSet(self, CFGUARDIAN_MODANIM_Floating, 0, 0);
            objAnim_func_80024D74(self, 0xFA);
            objdata->windLiftState = 1;
            self->velocity.z = 0.0f;
            self->velocity.y = 0.0f;
            self->velocity.x = 0.0f;
            self->objhitInfo->unk58 |= 0x400;
            objdata->talkState = CFGUARDIAN_TALK_Disabled;
            objdata->flags |= CFGUARDIAN_FLAG_1;
            break;
        case 16:
            // exit windlift
            // @recomp: Patched windlifts send an exit message when they unload. For the guardian,
            //          we don't want to consider this as a real exit.
            if (((s32)mesgArg) & (1 << 12)) {
                break;
            }
            STUBBED_PRINTF("Guardian Out of WindLIft "); // guessed location
            objAnimSet(self, CFGUARDIAN_MODANIM_Idle, 0, 0);
            objAnim_func_80024D74(self, 0x32);
            self->velocity.x = 0.0f;
            self->velocity.y = -0.001f;
            self->velocity.z = 0.0f;
            objdata->windLiftState = 2;
            objdata->flags &= ~CFGUARDIAN_FLAG_1;
            break;
        }
    }
    if (self->unkAF & ARROW_FLAG_1_Interacted) {
        joyDisableButtons(0, A_BUTTON);
        if (gDLL_1_cmdmenu->vtbl->was_this_item_used(BIT_SpellStone_CRF) != 0) {
            mainSetBits(BIT_Play_Seq_02AB_CF_SpellStone_Activation, 1);
        } else if (objdata->talkState == CFGUARDIAN_TALK_Enabled) {
            seqno = CFGuardian_mapLookup(sTalkSeqStateMap, objdata->state, sTalkSeqStateMapLength, objdata->talkSeqSelector);
            if (seqno != -1) {
                objdata->talkState = CFGUARDIAN_TALK_SpokenTo;
                gDLL_3_Animation->vtbl->start_obj_sequence(seqno, self, -1);
            }
        }
    }
    if (mainGetBits(BIT_Force_CFGuardian_TalkSeq) != 0) {
        seqno2 = CFGuardian_mapLookup(sTalkSeqStateMap, objdata->state, sTalkSeqStateMapLength, objdata->talkSeqSelector);
        // @recomp: Only play when the player is close by and not in a seq
        if (seqno2 != -1 && !(player->stateFlags & OBJSTATE_IN_SEQ) && vec3Distance(&self->globalPosition, &player->globalPosition) < 80.0f) {
            objdata->talkState = CFGUARDIAN_TALK_SpokenTo;
            gDLL_3_Animation->vtbl->start_obj_sequence(seqno2, self, -1);
            mainSetBits(BIT_Force_CFGuardian_TalkSeq, 0);
        }
    }
    if ((sStateAnimMap[objdata->state] != -1) && !(objdata->flags & CFGUARDIAN_FLAG_1) && (self->curModAnimId != sStateAnimMap[objdata->state])) {
        objAnimSet(self, sStateAnimMap[objdata->state], 0, 0);
        objAnim_func_80024D74(self, 0x50);
        STUBBED_PRINTF(" Set Anim ");
    }
    if (objAnimAdvance(self, objdata->animRate, (f32) gUpdateRate, &animState) != 0) {
        if (objdata->flags & CFGUARDIAN_FLAG_1) {
            if ((self->curModAnimId != CFGUARDIAN_MODANIM_Walk) && (self->curModAnimId != CFGUARDIAN_MODANIM_Floating)) {
                objdata->flags &= ~CFGUARDIAN_FLAG_1;
                STUBBED_PRINTF(" OVeride Set ");
            }
        } else if ((mathRnd(0, 6) == 0) && (sp6C != 0)) {
            modAnimId = CFGuardian_mapLookup(sAnimTransitionMap, self->curModAnimId, sAnimTransitionMapLength, mathRnd(0, 1));
            if (modAnimId != -1) {
                objAnim_func_80024D74(self, 0x28);
                objAnimSet(self, modAnimId, 0, 0);
                objdata->flags |= CFGUARDIAN_FLAG_1;
            } else {
                objAnimSet(self, sStateAnimMap[objdata->state], 0, 0);
            }
            STUBBED_PRINTF(" animnum %i "); // unknown location
        }
    }
    STUBBED_PRINTF(" Make Sound "); // guessed location
    CFGuardian_doModAnimSfx(self, &animState, sModAnimSfx);
    if (mathRnd(0, 60) == 0) {
        objExpr_func_80034B54(self, &objdata->exprHeadAnim, sExprSfx[mathRnd(0, 4)], 0);
    }
    objExpr_func_80034BC0(self, &objdata->exprHeadAnim);
    objExprEyeIdle(self, &objdata->eyeIdleHeadAnim);
    CFGuardian_updateBoneDust(self, objdata->dustObjs, 0x500, 0, 0, 6);
    if (mainGetBits(BIT_CFGuardian_State) != objdata->state) {
        mainSetBits(BIT_CFGuardian_State, objdata->state);
        recomp_printf(" Set State %i \n"); // @recomp
        // @recomp: Only save position on state changes. This avoids a whole category of state desyncs where the position
        //          of the guardian when spawned may not match what their state (and the rest of the game state) is.
        //          See the CFLevelControl patches for more info.
        recomp_printf("Saving guardian pos  %f,%f,%f\n", self->srt.transl.x, self->srt.transl.y, self->srt.transl.z);
        mapSaveObject(self->setup, self->mapID, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z);
    }
    // @recomp: Moved
    //mapSaveObject(self->setup, self->mapID, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z);
    return 0;
}

RECOMP_PATCH s32 CFGuardian_walkTo(Object* self, SRT* target, f32 speed, f32* animChange) {
    f32 dirX;
    f32 dirY;
    f32 dirZ;
    f32 dist;
    s16 angle;
    s32 _pad;

    if (target == NULL) {
        return 0;
    }
    dirX = target->transl.x - self->srt.transl.x;
    dirY = target->transl.y - self->srt.transl.y;
    dirZ = target->transl.z - self->srt.transl.z;
    dist = sqrtf(SQ(dirX) + SQ(dirY) + SQ(dirZ));
    if (dist < (speed * 5.0f)) {
        return 1;
    }
    guNormalize(&dirX, &dirY, &dirZ);
    self->velocity.x = dirX * speed * gUpdateRateF;
    self->velocity.y = dirY * speed * gUpdateRateF;
    self->velocity.z = dirZ * speed * gUpdateRateF;
    // @recomp: Don't flip angle. I'm not sure why this was done but it makes the guardian face
    //          the wrong angle when in the courtyard...
    angle = (target->yaw - (self->srt.yaw & 0xFFFF))/* + 0x8000*/;
    CIRCLE_WRAP(angle);
    self->srt.yaw += ((((f32) angle + 0.5f) * (speed * gUpdateRateF)) / dist);
    objMove(self, self->velocity.x, self->velocity.y, self->velocity.z);
    if (self->curModAnimId != CFGUARDIAN_MODANIM_Walk) {
        objAnimSet(self, CFGUARDIAN_MODANIM_Walk, 0.0f, 0);
    }
    objGetAnimChange(self, speed, animChange);
    return 0;
}