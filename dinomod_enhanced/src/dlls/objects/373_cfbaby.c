#include "modding.h"

#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/26_curves.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/common/foodbag.h"
#include "game/gamebits.h"
#include "game/objects/interaction_arrow.h"
#include "sys/gfx/animseq.h"
#include "sys/gfx/textable.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "sys/objmsg.h"
#include "sys/objlib.h"
#include "sys/print.h"
#include "dll.h"
#include "macros.h"

#include "recomp/dlls/objects/373_CFCloudBaby_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
        // Considers things (the player, beans) to be close below this distance.
/*18*/ s16 closeMaxDist;
        // Can be rescued by the player below this distance.
/*1A*/ s16 rescueMaxDist;
/*1C*/ u8 initialState;
/*1D*/ u8 yaw8;
        // Set when initially rescued.
/*1E*/ s16 rescuedGamebit;
        // Swaps to this state after the player gets close for the first time.
/*20*/ u8 initialState2;
        // Set when the rescued timer completes and is considered 
        // to be in the throne room now.
/*22*/ s16 reachedPerchGamebit;
} CFCloudBaby_Setup;

typedef struct {
    Vec3f unk0;
    Vec3f unkC;
    Vec3f unk18;
    Vec3f unk24;
    f32 tValue;
    f32 unk34;
} CFCloudBaby_Data_4;

typedef struct {
    s32 rescuedTimer;
    CFCloudBaby_Data_4 unk4;
    HeadAnimation unk3C;
    HeadAnimation unk60;
    f32 beanXDir;
    f32 beanZDir;
    f32 unk8C;
    f32 animChange;
    f32 beanScale;
    s32 swapInitialState;
    s32 swappedInitialState;
    s32 unkA0;
    s32 attackingPlayer;
    s32 unkA8;
    s32 state;
    u8 prevState;
    s32 distracted;
    s16 origYaw;
    s16 unkBA;
    u8 _unkBC[0xFC - 0xBC];
    Object* bean;
    Vec3f savedPos; // from before being distracted
    UnkCurvesStruct unk10C;
    u8 flags;
} CFCloudBaby_Data;

enum CFCloudBabyStates {
    CFCLOUDBABY_STATE_0_FleeIfPlayerIsClose = 0,
    CFCLOUDBABY_STATE_1 = 1,
    CFCLOUDBABY_STATE_2 = 2,
    CFCLOUDBABY_STATE_3_WaitForKyte = 3,
    CFCLOUDBABY_STATE_4_PeckIfPlayerIsClose = 4,
    CFCLOUDBABY_STATE_5_DistractedByKyte = 5,
    CFCLOUDBABY_STATE_6_InChest = 6,
    CFCLOUDBABY_STATE_7_RescuedWhileEatingBean = 7,
    CFCLOUDBABY_STATE_8_WaitingForBean = 8,
    CFCLOUDBABY_STATE_9_WalkToBean = 9,
    CFCLOUDBABY_STATE_10_EatBean = 10,
    CFCLOUDBABY_STATE_11_EatingBean = 11,
    CFCLOUDBABY_STATE_12_Rescued = 12
};

enum CFCloudBabyFlags {
    CFCLOUDBABY_RescueTimerActive = 0x1,
    // @recomp: Custom flags
    CFCLOUDBABY_AlreadyPerched = 0x2
};

extern s16 data_0[4];

extern int CFCloudBaby_animCallback(Object* actor, Object* animObj, AnimObj_Data* animObjData, s8);
extern void CFCloudBaby_turnToFaceObj(Object* self, Object* obj, CFCloudBaby_Data* objdata);
extern s32 CFCloudBaby_moveToPos(Object* self, f32 targX, f32 targY, f32 targZ, f32 speed);
extern s32 CFCloudBaby_navigatePathToThroneRoom(Object* self, UnkCurvesStruct* arg1, f32 speed, f32 arg3, f32 arg4, f32 arg5);
extern s32 CFCloudBaby_moveToPathStart(Object* self, CurveSetup* curve, CFCloudBaby_Data_4* arg2, f32* tValue, f32 speed);
extern f32 CFCloudBaby_func_1DB0(CFCloudBaby_Data_4* arg0, Vec3f* arg1, Vec3f* arg2, Vec3f* arg3, s32 arg4);
extern void CFCloudBaby_setupPathToThroneRoom(Object* self, f32 speed);
extern s32 CFCloudBaby_checkForKyte(Object* self, CFCloudBaby_Data* objdata);

RECOMP_PATCH void CFCloudBaby_obj_Setup(Object* self, CFCloudBaby_Setup* setup, s32 reset) {
    CFCloudBaby_Data* objdata;

    func_8002674C(self);
    objInitMesgQueue(self, 4);
    self->animCallback = CFCloudBaby_animCallback;
    self->srt.yaw = setup->yaw8 << 8;
    objAddObjectType(self, OBJTYPE_CFCloudBaby);
    objAddObjectType(self, OBJTYPE_Baddie);
    objdata = self->data;
    objdata->swapInitialState = FALSE;
    objdata->swappedInitialState = FALSE;
    objdata->unkA0 = 0;
    objdata->attackingPlayer = FALSE;
    objdata->unkA8 = 0;
    objdata->state = setup->initialState;
    objdata->distracted = 0;
    objdata->rescuedTimer = 0;
    objdata->bean = NULL;
    objdata->origYaw = self->srt.yaw;
    objdata->flags = 0;
    if (mainGetBits(setup->reachedPerchGamebit) != 0) {
        func_800267A4(self);
        self->srt.flags |= OBJFLAG_INVISIBLE;
        objdata->flags &= ~CFCLOUDBABY_RescueTimerActive;
        // @recomp: objDisable doesn't work in objsetup, set a flag to do it on the first tick instead
        //objDisable(self);
        objdata->flags |= CFCLOUDBABY_AlreadyPerched;
        objFreeObjectType(self, OBJTYPE_CFCloudBaby);
        objFreeObjectType(self, OBJTYPE_Baddie);
    } 
    // @recomp: If we were unloaded between being rescued and setting the perch bit, return
    //          to the 'being rescued' state to avoid a softlock (otherwise the perch bit
    //          will never get set).
    else if (setup->rescuedGamebit != BIT_ALWAYS_1 && mainGetBits(setup->rescuedGamebit) != 0) {
        objdata->rescuedTimer = 1;
    }
    // @recomp: Sync rescue dist with lock icon interact dist so the icon is grey until the player can actually interact
    obj_func_80023BF8(self, setup->rescueMaxDist, 0, 0, 0, 0);
}

RECOMP_PATCH void CFCloudBaby_obj_Control(Object* self) {
    CFCloudBaby_Data* objdata;
    CFCloudBaby_Setup* setup;
    Object* player;
    Object* sidekick;
    Object* bean;
    f32 magnitude;
    s32 isPlayerClose;
    Object* foodbag;

    setup = (CFCloudBaby_Setup*)self->setup;
    objdata = self->data;
    player = objGetPlayer();
    sidekick = objGetSidekick();

    // @recomp: Handle the case of disabling on spawn here instead of in setup since it doesn't work there
    if (objdata->flags & CFCLOUDBABY_AlreadyPerched) {
        objDisable(self);
        return;
    }

    if (objdata->rescuedTimer != 0) {
        // rescued, flying away
        objdata->flags |= CFCLOUDBABY_RescueTimerActive;
        if (objdata->rescuedTimer == 480) {
            CFCloudBaby_setupPathToThroneRoom(self, 3.0f);
        }
        objdata->state = CFCLOUDBABY_STATE_0_FleeIfPlayerIsClose;
        if (objdata->rescuedTimer > 0) {
            // @bug: At 60 Hz this will reduce the timer to 0 making it impossible to rescue the CloudRunner
            //       in the chest as the player has no way of leaving the treasure room (see below) fast
            //       enough to let the perch bit be set. At 30 Hz and below the timer will become negative 
            //       (it always starts at 1 in this build), which will let this rescue logic continue to 
            //       stall until the player leaves. This will also be an issue if the player enters a seq 
            //       on the tick this timer hits zero for the same reasons.
            objdata->rescuedTimer -= gUpdateRate;
            // @recomp: fix above mentioned bug
            if (objdata->rescuedTimer == 0) {
                objdata->rescuedTimer = -1;
            }
        }
        if (objdata->rescuedTimer <= 0) {
            if ((mainGetBits(BIT_CRF_Player_In_Treasure_Room) == FALSE) && !(player->stateFlags & OBJSTATE_IN_SEQ)) {
                if (setup->reachedPerchGamebit != -1) {
                    mainSetBits(setup->reachedPerchGamebit, 1);
                }
                STUBBED_PRINTF(" The Birdy End is Nigh ");
                objdata->rescuedTimer = 0;
                func_800267A4(self);
                self->srt.flags |= OBJFLAG_INVISIBLE;
                objdata->flags &= ~CFCLOUDBABY_RescueTimerActive;
                objDisable(self);
                objFreeObjectType(self, OBJTYPE_CFCloudBaby);
                objFreeObjectType(self, OBJTYPE_Baddie);
                gDLL_1_cmdmenu->vtbl->energy_bar_fadeout();
            }
            self->srt.flags |= OBJFLAG_INVISIBLE;
            return;
        }
        diPrintf("Bird Timer %i ", objdata->rescuedTimer);
        if (CFCloudBaby_navigatePathToThroneRoom(self, &objdata->unk10C, 3.0f, objdata->beanXDir, objdata->unk8C, objdata->beanZDir) != 0) {
            self->opacity = 0;
        }
        return;
    }

    if (objdata->state == CFCLOUDBABY_STATE_6_InChest) {
        // rescuing this CloudRunner is handled by the chest
        return;
    }

    self->unkAF |= ARROW_FLAG_8_No_Targetting;
    if (!objdata->swappedInitialState && objdata->swapInitialState && (mainGetBits(setup->rescuedGamebit) != 0)) {
        objdata->swappedInitialState = TRUE;
        objdata->state = setup->initialState2;
    }
    isPlayerClose = vec3Distance(&self->globalPosition, &player->globalPosition) < (f32) setup->closeMaxDist;
    if (isPlayerClose || 
            (objdata->state == CFCLOUDBABY_STATE_4_PeckIfPlayerIsClose) || 
            (objdata->state == CFCLOUDBABY_STATE_5_DistractedByKyte) || 
            (objdata->state == CFCLOUDBABY_STATE_0_FleeIfPlayerIsClose)) {
        switch (objdata->state) {
        case CFCLOUDBABY_STATE_0_FleeIfPlayerIsClose:
            objdata->swapInitialState = FALSE;
            /* fallthrough */
        case CFCLOUDBABY_STATE_1:
            if (isPlayerClose) {
                gDLL_3_Animation->vtbl->start_obj_sequence(objdata->state + 1, self, -1);
            }
            CFCloudBaby_checkForKyte(self, objdata);
            objdata->swapInitialState = TRUE;
            // @recomp: Set/advance idle animation
            if (objdata->unkA8 != 0) {
                objdata->unkA8 = 0;
                objAnimSet(self, 0, 0.0f, 0);
            } else {
                objAnimAdvance(self, 0.0064f, gUpdateRateF, NULL);
            }
            break;
        case CFCLOUDBABY_STATE_2:
        case CFCLOUDBABY_STATE_8_WaitingForBean:
        case CFCLOUDBABY_STATE_9_WalkToBean:
        case CFCLOUDBABY_STATE_10_EatBean:
        case CFCLOUDBABY_STATE_11_EatingBean:
            // flee away from player
            gDLL_3_Animation->vtbl->start_obj_sequence(1, self, -1);
            objdata->state = CFCLOUDBABY_STATE_8_WaitingForBean;
            break;
        case CFCLOUDBABY_STATE_4_PeckIfPlayerIsClose:
            if (objdata->attackingPlayer) {
                objdata->unkA8 = 0;
                objdata->attackingPlayer = (s32) (objAnimAdvance(self, 0.0064f, gUpdateRateF, NULL) == 0);
            } else if ((mathRnd(0, 10) == 1) && (vec3Distance(&self->globalPosition, &player->globalPosition) < (f32) setup->rescueMaxDist)) {
                // peck at player
                objAnimSet(self, 0x12, 0.0f, 0);
                dll_amSfx->Play(self, SOUND_8B, 0x7E, NULL, NULL, 0, NULL);
                objdata->attackingPlayer = TRUE;
            } else {
                CFCloudBaby_turnToFaceObj(self, player, objdata);
            }
            if (objdata->attackingPlayer && (self->animProgress > 0.5f)) {
                // activate damage hitbox to deal damage with the peck
                func_80026128(self, 9, 1, 0);
            } else {
                func_80026160(self);
            }
            /* fallthrough */
        case CFCLOUDBABY_STATE_3_WaitForKyte:
            CFCloudBaby_checkForKyte(self, objdata);
            break;
        case CFCLOUDBABY_STATE_7_RescuedWhileEatingBean:
            // rescued
            gDLL_1_cmdmenu->vtbl->energy_bar_create(0, 5, TEXTABLE_571, TEXTABLE_572, 5);
            mainIncrementBits(BIT_CRF_Num_Rescued_Baby_CloudRunners);
            gDLL_1_cmdmenu->vtbl->energy_bar_set(5 - mainGetBits(BIT_CRF_Num_Rescued_Baby_CloudRunners));
            self->srt.yaw = objdata->origYaw;
            gDLL_3_Animation->vtbl->start_obj_sequence(4, self, -1);
            objdata->rescuedTimer = 1;
            mainSetBits(setup->rescuedGamebit, 1);
            self->unkDC = 0;
            break;
        case CFCLOUDBABY_STATE_5_DistractedByKyte:
            func_80026160(self);
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
            if ((vec3Distance(&self->globalPosition, &player->globalPosition) < (f32) setup->rescueMaxDist) 
                    && (self->unkAF & ARROW_FLAG_1_Interacted)) {
                // rescue
                joyDisableButtons(0, A_BUTTON);
                self->srt.yaw = objdata->origYaw;
                gDLL_3_Animation->vtbl->start_obj_sequence(4, self, -1);
                objdata->rescuedTimer = 1;
                gDLL_1_cmdmenu->vtbl->energy_bar_create(0, 5, TEXTABLE_571, TEXTABLE_572, 5);
                mainIncrementBits(BIT_CRF_Num_Rescued_Baby_CloudRunners);
                gDLL_1_cmdmenu->vtbl->energy_bar_set(5 - mainGetBits(BIT_CRF_Num_Rescued_Baby_CloudRunners));
                objdata->state = CFCLOUDBABY_STATE_12_Rescued;
                mainSetBits(setup->rescuedGamebit, 1);
                self->unkDC = 0;
                break;
            } else if (objdata->distracted) {
                if (((DLL_ISidekick*)sidekick->dll)->vtbl->Func24(sidekick) == 0) {
                    // no longer being distracted, move back to original pos
                    objGetAnimChange(self, 0.5f, &objdata->animChange);
                    if (CFCloudBaby_moveToPos(self, objdata->savedPos.x, objdata->savedPos.y, objdata->savedPos.z, 0.5f) != 0) {
                        objdata->distracted = FALSE;
                        objdata->state = objdata->prevState;
                    }
                    objAnimAdvance(self, objdata->animChange, (f32) gUpdateRate, NULL);
                } else {
                    // allow interaction and watch kyte
                    self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
                    CFCloudBaby_turnToFaceObj(self, sidekick, objdata);
                }
                break;
            }
            /* fallthrough */
        case CFCLOUDBABY_STATE_12_Rescued:
        default:
            CFCloudBaby_turnToFaceObj(self, player, objdata);
            break;
        }
    } else if (objdata->state >= CFCLOUDBABY_STATE_8_WaitingForBean) {
        switch (objdata->state) {
        case CFCLOUDBABY_STATE_11_EatingBean:
            objdata->state = CFCLOUDBABY_STATE_8_WaitingForBean;
            /* fallthrough */
        case CFCLOUDBABY_STATE_8_WaitingForBean:
            foodbag = ((DLL_210_Player*)player->dll)->vtbl->func66(player, 0xF);
            // @bug: bean will be uninitialized if the foodbag is null
            // @recomp: initialize bean var
            bean = NULL;
            if (foodbag != NULL) {
                bean = ((DLL_IFoodbag*)foodbag->dll)->vtbl->get_nearest_placed_food_of_type(foodbag, self, 
                    FOOD_Red_Bean | FOOD_Brown_Bean | FOOD_Blue_Bean);
            }
            if ((bean != NULL) && (vec3Distance(&self->globalPosition, &bean->globalPosition) < (f32) setup->closeMaxDist)) {
                objdata->state = CFCLOUDBABY_STATE_9_WalkToBean;
                objdata->origYaw = objAngleToObjectXZ(self, bean, NULL) + self->srt.yaw;
                objdata->beanXDir = bean->srt.transl.x - self->srt.transl.x;
                objdata->beanZDir = bean->srt.transl.z - self->srt.transl.z;
                if ((objdata->beanXDir != 0.0f) || (objdata->beanZDir != 0.0f)) {
                    magnitude = sqrtf(SQ(objdata->beanXDir) + SQ(objdata->beanZDir));
                    objdata->unkBA = (s16) ((magnitude / 0.5f) - 18.0f);
                    objdata->beanXDir *= (0.5f / magnitude);
                    objdata->beanZDir *= (0.5f / magnitude);
                    objAnimSet(self, 9, 0.0f, 0);
                    objGetAnimChange(self, 0.5f, &objdata->animChange);
                    objdata->bean = bean;
                    objdata->beanScale = bean->srt.scale;
                } else {
                    return;
                }
            }
            break;
        case CFCLOUDBABY_STATE_9_WalkToBean:
            objdata->unkBA -= 1;
            if (objdata->unkBA < 0) {
                objdata->state = CFCLOUDBABY_STATE_10_EatBean;
                self->srt.yaw = objdata->origYaw;
            } else {
                self->srt.yaw += (objdata->origYaw - self->srt.yaw) / 8;
                self->srt.transl.x += objdata->beanXDir;
                self->srt.transl.z += objdata->beanZDir;
            }
            objAnimAdvance(self, objdata->animChange, (f32) gUpdateRate, NULL);
            break;
        case CFCLOUDBABY_STATE_10_EatBean:
            gDLL_3_Animation->vtbl->start_obj_sequence(3, self, -1);
            break;
        default:
            CFCloudBaby_turnToFaceObj(self, player, objdata);
            break;
        }
    } else {
        if (objdata->swapInitialState) {
            CFCloudBaby_turnToFaceObj(self, player, objdata);
        } else {
            // @recomp: Set/advance idle animation
            if (objdata->unkA8 != 0) {
                objdata->unkA8 = 0;
                objAnimSet(self, 0, 0.0f, 0);
            } else {
                objAnimAdvance(self, 0.0064f, gUpdateRateF, NULL);
            }
        }
    }

    if (mathRnd(0, 30) == 0) {
        objExpr_func_80034B94(self, &objdata->unk60, data_0[mathRnd(0, 3)]);
    }
    objExpr_func_80034BC0(self, &objdata->unk60);
}
