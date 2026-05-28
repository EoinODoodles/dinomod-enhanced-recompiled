#include "modding.h"
#include "recomputils.h"
#include "custom_gamebits.h"

#include "dlls/engine/27.h"
#include "dlls/engine/53_movelib.h"
#include "dlls/objects/common/sidekick.h"
#include "game/gamebits.h"
#include "game/objects/interaction_arrow.h"
#include "sys/curves.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/objtype.h"
#include "sys/rand.h"
#include "macros.h"
#include "dll.h"

#include "recomp/dlls/objects/508_SHthorntail_recomp.h"

typedef struct {
/*000*/ MoveLibData movedata;
/*4B8*/ s8 state;
/*4B9*/ u8 _unk4B9;
/*4BA*/ u8 flags;
/*4BB*/ u8 mapAct;
/*4BC*/ s8 grazingAltAnimSelector;
/*4BD*/ u8 nextSeq; // next seq in rotation to play on interact
/*4BE*/ u8 _unk4BE[0x4C0 - 0x4BE];
/*4C0*/ u8* talkSeqs;
/*4C4*/ u8 talkSeqsCount; // number of seqs in rotation
/*4C5*/ u8 _unk4C5[0x4D0 - 0x4C5];
/*4D0*/ s16 eatingTimer;
/*4D2*/ s16 grazingTimer;
/*4D4*/ s16 drinkTimer;
/*4D6*/ s16 targetAngle;
/*4D8*/ s16 angleToTarget;
/*4DA*/ s16 startAngle;
/*4DC*/ s16 progressionBlockerGamebit;
/*4E0*/ s32 unk4E0;
/*4E4*/ DLL27_Data collider;
/*744*/ u8 _unk744[0x75C - 0x744];
/*75C*/ CurveSetup* prevCurve;
/*760*/ CurveSetup* currentCurve;
/*764*/ CurveSetup* targetCurve;
/*768*/ u8 _unk768[0x804 - 0x768];
/*804*/ f32 modAnimDelta;
/*808*/ f32 walkSpeed;
/*80C*/ u8 _unk80C[0x840 - 0x80C];
/*840*/ f32 playerDist;
/*844*/ f32 distToTargetCurve;
/*848*/ f32 unk848;
/*84C*/ f32 unk84C;
/*850*/ HeadAnimation headAnim;
/*874*/ u8 unk874;
/*875*/ u8 _unk875[0x878 - 0x875];
// @recomp
    u8 wakingFromDistract;
} SHthorntail_Data;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 thorntail; // which Thorntail this is
/*19*/ u8 yaw;          //@recomp: custom param
/*1A*/ s16 gamebitAway; //@recomp: ThornTail doesn't show up when set
} SHthorntail_Setup;

enum ThorntailSeq {
    THORNTAILSEQ_0_HaveYouWokenTheSwapStone = 0, // 0x275
    THORNTAILSEQ_1_LeaveMeAloneStranger = 1, // 0x276
    THORNTAILSEQ_2_ALogForAFish = 2, // 0x277
    THORNTAILSEQ_3_IDontWantThat = 3, // 0x278
    THORNTAILSEQ_4_NoLogsToday = 4, // 0x279
    THORNTAILSEQ_5_NoLogsToday2 = 5, // 0x27A
    THORNTAILSEQ_6_HowsRandorn = 6, // 0x27B
    THORNTAILSEQ_7_ItsANiceDay = 7, // 0x27C
    THORNTAILSEQ_8_TheSharpClawDrainedOurRiver = 8, // 0x27D
    THORNTAILSEQ_9_Unknown = 9, // 0x27E
    THORNTAILSEQ_10_ThornsBlockingBurrows = 10, // 0x27F
    THORNTAILSEQ_11_ArentLikeTheyUsedToBe = 11 // 0x280
};

enum ThorntailFlags {
    THORNTAILFLAG_ModAnimDone = 0x1,
    THORNTAILFLAG_AtTarget = 0x2,
    THORNTAILFLAG_Walking = 0x4,
    THORNTAILFLAG_ForceModAnimChange = 0x8,
    THORNTAILFLAG_GoToNextNode = 0x10,
    THORNTAILFLAG_ProgressionBlocker = 0x20,
    THORNTAILFLAG_RotateTalkSeqs = 0x40
};

enum ThorntailState {
    THORNTAILSTATE_Grazing = 0,
    THORNTAILSTATE_Grazing_LookingAround = 1,
    THORNTAILSTATE_Grazing_Swallowing = 2,
    THORNTAILSTATE_MoveToTarget = 3,
    THORNTAILSTATE_TurnLeft = 4,
    THORNTAILSTATE_TurnRight = 5,
    THORNTAILSTATE_StartDrinking = 6,
    THORNTAILSTATE_Drinking = 7,
    THORNTAILSTATE_DoneDrinking = 8,
    THORNTAILSTATE_Idle = 9,
    THORNTAILSTATE_BlockingProgression = 10,
    THORNTAILSTATE_WakingUp = 11,
    THORNTAILSTATE_LookingAround = 12,
    THORNTAILSTATE_Sleeping = 13,
    THORNTAILSTATE_FallingAsleep = 14
};

enum ThorntailStateFlags {
    THORNTAILSTATEFLAG_NoTargeting = 0x1,
    THORNTAILSTATEFLAG_Asleep = 0x2
};

enum ThorntailIndex {
    THORNTAIL_1_Sleepy = 1,     // The sleepy-voiced ThornTail near the tree hollow/river crossing
    THORNTAIL_2_Log_Trader = 2, // The ThornTail near the burrows behind Rocky
    THORNTAIL_3_Elder = 3,       // The elderly ThornTail near Willow Grove
    // @recomp:
    THORNTAIL_4_Willow_Grove = 4 //Extra ThornTail blocking Willow Grove (like the one MusicalProgrammer added in older patches) TODO: config for this?
};

enum ThorntailCurveSubtype {
    // trader middle nodes, can always navigate to.
    THORNTAILCURVE_0 = 0,
    // in front of well (sleepy), in front of burrows (trader), spore patch (elder).
    // never returned to.
    THORNTAILCURVE_3 = 3,
    // river deadend (for drinking) (sleepy).
    // can navigate to if unk4E0 == 9
    THORNTAILCURVE_4 = 4,
    // elder nodes, most of sleepy nodes, trader dead end.
    // can navigate to if unk4E0 != 9
    THORNTAILCURVE_8 = 8,
    // some of sleepy's nodes are this.
    // can always navigate to.
    THORNTAILCURVE_9 = 9
};

extern CurveSetup* thorntail_find_closest_curve(f32 x, f32 y, f32 z, s32 curveSubtype);
extern int thorntail_anim_callback(Object *actor, Object *animObj, AnimObj_Data *animObjData, s8 a3);
extern void thorntail_common_control(Object* self, SHthorntail_Data* objdata, SHthorntail_Setup* setup);
extern void thorntail_sleepy_setup(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup);
extern void thorntail_sleepy_init(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup);
extern void thorntail_trader_setup(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup);
extern void thorntail_elder_setup(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup);
extern void thorntail_update_shadow(Object* self);
extern void thorntail_sleepy_control(Object* self, SHthorntail_Data* objdata, SHthorntail_Setup* setup);
extern void thorntail_trader_control(Object* self, SHthorntail_Data* objdata, SHthorntail_Setup* setup);
extern void thorntail_elder_control(Object* self, SHthorntail_Data* objdata, SHthorntail_Setup* setup);

extern Vec3f data_0[];
extern f32 data_30[];
extern Unk80026DF4 data_40[];
extern u8 sStateFlagMap[];
/*0x16C*/ extern s16 sModAnimMap[];
/*0x1B8*/ extern f32 data_1B8[];
/*0x1FC*/ extern u8 sElderTalkSeqs[1];

RECOMP_PATCH void thorntail_setup(Object *self, SHthorntail_Setup *setup, s32 reset) {
    SHthorntail_Data *objdata = self->data;
    s32 _pad;
    u8 sp3C[] = {1, 1, 1, 1}; // data_224

    switch (setup->thorntail) {
    case THORNTAIL_1_Sleepy:
        thorntail_sleepy_setup(self, objdata, setup);
        break;
    case THORNTAIL_2_Log_Trader:
        thorntail_trader_setup(self, objdata, setup);
        break;
    case THORNTAIL_3_Elder:
        thorntail_elder_setup(self, objdata, setup);
        break;
    //@recomp: custom extra ThornTail
    case THORNTAIL_4_Willow_Grove:
        self->srt.yaw = setup->yaw << 8;
        return;
    }

    gDLL_27->vtbl->init(&objdata->collider, DLL27FLAG_4000000 | DLL27FLAG_2000000, DLL27FLAG_400, DLL27MODE_1);
    gDLL_27->vtbl->setup_terrain_collider(&objdata->collider, 4, data_0, data_30, sp3C);
    gDLL_27->vtbl->reset(self, &objdata->collider);

    self->shadow->flags |= (OBJ_SHADOW_FLAG_TOP_DOWN | OBJ_SHADOW_FLAG_USE_OBJ_YAW | OBJ_SHADOW_FLAG_CUSTOM_OBJ_POS | OBJ_SHADOW_FLAG_CUSTOM_DIR);
    self->shadow->distFadeMaxOpacity = 128;
    self->shadow->distFadeMinOpacity = 90;
    self->animCallback = thorntail_anim_callback;

    objdata->state = -1;

    create_temp_dll(53);
    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func2(self, &objdata->movedata, -0x1FFF, 0x2AAA, 3);
    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func5(&objdata->movedata, 400, 30);
    objdata->movedata.unk4A9 &= ~0x8;

    obj_add_object_type(self, OBJTYPE_40);
}

// Behaviour for the custom sleeping ThornTail
static void thorntail_control_willow_grove(Object* self, SHthorntail_Data* objdata, SHthorntail_Setup* setup) {
    UnkFunc_80024108Struct animInfo;
    TextureAnimator* texAnimA;
    TextureAnimator* texAnimB;

    //Check if gamebit set (wandered off)
    if ((setup->gamebitAway > (NO_GAMEBIT + 1)) && main_get_bits(setup->gamebitAway)) {
        obj_destroy_object(self);
        return;
    }

    //Go to sleep
    if (self->curModAnimId != 4) {
        func_80023D30(self, 4, 0.0f, 0);

        //Close eyes
        texAnimA = func_800348A0(self, 5, 0);
        texAnimB = func_800348A0(self, 4, 0);
        if (texAnimA != NULL) {
            texAnimA->frame = 0x200;
        }
        if (texAnimB != NULL) {
            texAnimB->frame = 0x200;
        }

        self->srt.scale = self->def->scale * 1.15f;
    }

    self->unkAF |= ARROW_FLAG_8_No_Targetting;

    //Advance animation
    func_80024108(self, 0.005f, gUpdateRateF, &animInfo);
}

RECOMP_PATCH void thorntail_control(Object* self) {
    SHthorntail_Data* objdata;
    SHthorntail_Setup* setup;
    Object* player;

    objdata = self->data;
    setup = (SHthorntail_Setup*)self->setup;
    player = get_player();
    self->unkAF &= ~ARROW_FLAG_8_No_Targetting;

    //@recomp
    if (setup->thorntail == THORNTAIL_4_Willow_Grove) {
        thorntail_control_willow_grove(self, objdata, setup);
        return;
    }

    if (func_80026DF4(self, data_40, 15, objdata->unk874, &objdata->unk84C) == 0) {
        objdata->unk874 = 0;
        objdata->mapAct = gDLL_29_Gplay->vtbl->get_act(self->mapID);
        objdata->playerDist = vec3_distance(&self->globalPosition, &player->globalPosition);

        switch (setup->thorntail) {
        case THORNTAIL_1_Sleepy:
            thorntail_sleepy_control(self, objdata, setup);
            break;
        case THORNTAIL_2_Log_Trader:
            thorntail_trader_control(self, objdata, setup);
            break;
        case THORNTAIL_3_Elder:
            thorntail_elder_control(self, objdata, setup);
            break;
        }
        
        thorntail_update_shadow(self);

        gDLL_27->vtbl->func_1E8(self, &objdata->collider, gUpdateRateF);
        gDLL_27->vtbl->func_5A8(self, &objdata->collider);
        gDLL_27->vtbl->func_624(self, &objdata->collider, gUpdateRateF);

        if (!(sStateFlagMap[objdata->state] & 2)) {
            func_80032A08(self, &objdata->headAnim);
        }
    }
}

RECOMP_PATCH void thorntail_free(Object *self, s32 onlySelf) {
    SHthorntail_Setup* objSetup = (SHthorntail_Setup*)self->setup;

    //@recomp
    if (objSetup && objSetup->thorntail == THORNTAIL_4_Willow_Grove) {
        return;
    }

    remove_temp_dll(53);
    obj_free_object_type(self, OBJTYPE_40);
}

RECOMP_PATCH u32 thorntail_get_data_size(Object *self, u32 offsetAddr) {
    // @recomp: Use custom objdata size
    return sizeof(SHthorntail_Data);
}

static void thorntail_anim_end_callback(Object *self, Object *override, AnimObj_Data* animObjData) {
    // Reset bones from seq (for some reason these don't get correctly reset)
    // TODO: figure out what's actually going on here. the head lookat during talk seqs for Thorntails
    //       don't really look right a lot of the time to begin with.
    s32 *boneList = func_800349B0();
    for (s32 i = 0; i < 9; i++){
        s16* bone = func_80034804(self, boneList[i]);
        if (bone != NULL){
            bone[0] = 0;
            bone[1] = 0;
            bone[2] = 0;
        }
    }
}

// @recomp: Register end of seq callback
RECOMP_HOOK_DLL(thorntail_anim_callback) void thorntail_anim_callback_hook(Object *actor, Object *animObj, AnimObj_Data *animObjData, s8 a3) {
    animObjData->unkF4 = thorntail_anim_end_callback;
}

RECOMP_PATCH void thorntail_common_control(Object* self, SHthorntail_Data* objdata, SHthorntail_Setup* setup) {
    s32 ignoreUID;
    s32 allowBranch;
    s32 numBranches;
    s32 numNodeOptions;
    s32 branches[4];
    s32 i;
    CurveSetup* branch;
    CurveSetup* nodeOptions[4];
    s32 animIdx;
    f32 vz;
    f32 vx;
    f32 time;
    UnkFunc_80024108Struct sp3C;
    TextureAnimator* eyelidR;
    TextureAnimator* eyelidL;

    objdata->flags &= ~THORNTAILFLAG_Walking;
    if (self->unkAF & ARROW_FLAG_1_Interacted) {
        gDLL_3_Animation->vtbl->start_obj_sequence(objdata->talkSeqs[objdata->nextSeq], self, -1);
        if (objdata->flags & THORNTAILFLAG_RotateTalkSeqs) {
            objdata->nextSeq++;
            if (objdata->nextSeq >= objdata->talkSeqsCount) {
                objdata->nextSeq = 0;
            }
        }
        return;
    }
    if (objdata->flags & THORNTAILFLAG_GoToNextNode) {
        // If at navigable deadend, don't ignore the node we came from so we don't get stuck
        if ((objdata->prevCurve != NULL) && (objdata->currentCurve->type1D.unk34 != THORNTAILCURVE_4)) {
            ignoreUID = objdata->prevCurve->uID;
        } else {
            ignoreUID = -1;
        }
        // Lookup branches for the current curve node
        numBranches = gDLL_26_Curves->vtbl->func_4F0(objdata->currentCurve, ignoreUID, branches);
        numNodeOptions = 0;
        for (i = 0; i < numBranches; i++) {
            branch = gDLL_26_Curves->vtbl->func_39C(branches[i]);
            if (branch != NULL) {
                allowBranch = FALSE;
                switch (objdata->unk4E0) {
                case 9:
                    if ((branch->type1D.unk34 == THORNTAILCURVE_9) || 
                        (branch->type1D.unk34 == THORNTAILCURVE_4) || 
                        (branch->type1D.unk34 == THORNTAILCURVE_0)) {
                        allowBranch = TRUE;
                    }
                    break;
                case 8:
                default:
                    if ((branch->type1D.unk34 == THORNTAILCURVE_8) || 
                        (branch->type1D.unk34 == THORNTAILCURVE_9) || 
                        (branch->type1D.unk34 == THORNTAILCURVE_0)) {
                        allowBranch = TRUE;
                    }
                    break;
                }
                if (allowBranch) {
                    nodeOptions[numNodeOptions] = branch;
                    numNodeOptions += 1;
                }
            }
        }
        if (numNodeOptions != 0) {
            // Choose random eligible branch
            objdata->targetCurve = nodeOptions[rand_next(0, numNodeOptions - 1)];
            objdata->prevCurve = objdata->currentCurve;
            objdata->flags &= ~THORNTAILFLAG_AtTarget;
        } else {
            // No eligible branches, fallback to the closest subtype 0 node.
            // @bug: This is why the Thorntail by Willow Grove walks up the wall into the SwapStone area
            //       sometimes. Subtype 0 nodes are only found on the log trader's curve network.
            
            // STUBBED_PRINTF("THORNTAIL: help cannot find a node\n");
            // STUBBED_PRINTF("Thorntail %d, is on a network with a deadend\n", setup->base.uID); // default.dol
            // @recomp: Restore printf
            recomp_eprintf("Thorntail %d, is on a network with a deadend\n", setup->base.uID);
            
            // @recomp: Don't try to find a fallback node, otherwise anyone other than the trader will navigate
            //          through a wall into the SwapStone area. This matches what default.dol does.
            // objdata->targetCurve = thorntail_find_closest_curve(self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, THORNTAILCURVE_0);
            // if (objdata->targetCurve != NULL) {
            //     objdata->prevCurve = objdata->currentCurve;
            //     objdata->flags &= ~THORNTAILFLAG_AtTarget;
            // }

            // @recomp: Instead of a fallback node, flip the curve selector in an attempt to get unstuck
            objdata->unk4E0 = objdata->unk4E0 == 8 ? 9 : 8;
        }
        objdata->flags &= ~THORNTAILFLAG_GoToNextNode;
    }
    if (objdata->flags & THORNTAILFLAG_AtTarget) {
        // Currently at target node
        objdata->grazingTimer -= gUpdateRate;
        if (objdata->grazingTimer < 0) {
            objdata->grazingTimer = 0;
        }
        if (!(sStateFlagMap[objdata->state] & THORNTAILSTATEFLAG_Asleep) && (gDLL_7_Newday->vtbl->func8(&time) != 0)) {
            // Time changed to night, force state switch to falling asleep
            objdata->state = THORNTAILSTATE_FallingAsleep;
        }
        switch (objdata->state) {
        case THORNTAILSTATE_Grazing:
            objdata->eatingTimer -= gUpdateRate;
            if (objdata->eatingTimer < 0) {
                objdata->eatingTimer = 0;
            }
            if ((objdata->flags & THORNTAILFLAG_ModAnimDone) && (objdata->eatingTimer <= 0)) {
                // After eating (but not necessarily done grazing), play an alternate animation for a bit
                if (objdata->grazingAltAnimSelector <= 0) {
                    objdata->state = THORNTAILSTATE_Grazing_Swallowing;
                } else {
                    objdata->state = THORNTAILSTATE_Grazing_LookingAround;
                }
            }
            if (objdata->grazingTimer <= 0) {
                if (objdata->unk4E0 == 9) {
                    objdata->flags |= THORNTAILFLAG_GoToNextNode;
                } else {
                    objdata->state = THORNTAILSTATE_LookingAround;
                }
            }
            // @recomp: Remove debug option to force Thorntail to move on to the next node. This can result
            // in Sleepy failing to find a node and wandering off through a wall.
            /*else if (joy_get_pressed(0) & D_JPAD) {
                objdata->unk4E0 = objdata->unk4E0 == 8 ? 9 : 8;
                objdata->flags |= THORNTAILFLAG_GoToNextNode;
            }*/
            break;
        case THORNTAILSTATE_LookingAround:
            // Delays movement to the next node
            if (objdata->flags & THORNTAILFLAG_ModAnimDone) {
                objdata->unk4E0 = 8;
                objdata->flags |= THORNTAILFLAG_GoToNextNode;
            }
            break;
        case THORNTAILSTATE_Grazing_LookingAround:
            if (objdata->flags & THORNTAILFLAG_ModAnimDone) {
                objdata->state = THORNTAILSTATE_Grazing;
                objdata->grazingAltAnimSelector = objdata->grazingAltAnimSelector - 1;
                objdata->eatingTimer = rand_next(500, 800);
            }
            break;
        case THORNTAILSTATE_Grazing_Swallowing:
            if (objdata->flags & THORNTAILFLAG_ModAnimDone) {
                objdata->state = THORNTAILSTATE_Grazing;
                objdata->grazingAltAnimSelector = rand_next(1, 2);
                objdata->eatingTimer = rand_next(500, 800);
            }
            break;
        case THORNTAILSTATE_StartDrinking:
            if (objdata->flags & THORNTAILFLAG_ModAnimDone) {
                objdata->state = THORNTAILSTATE_Drinking;
                objdata->drinkTimer = rand_next(1000, 2000);
            }
            break;
        case THORNTAILSTATE_Drinking:
            objdata->drinkTimer -= gUpdateRate;
            if (objdata->drinkTimer < 0) {
                objdata->drinkTimer = 0;
            }
            if ((objdata->flags & THORNTAILFLAG_ModAnimDone) && (objdata->drinkTimer <= 0)) {
                objdata->state = THORNTAILSTATE_DoneDrinking;
            }
            break;
        case THORNTAILSTATE_DoneDrinking:
            if (objdata->flags & THORNTAILFLAG_ModAnimDone) {
                objdata->state = THORNTAILSTATE_Idle;
            }
            break;
        case THORNTAILSTATE_Idle:
            if (objdata->flags & THORNTAILFLAG_ModAnimDone) {
                objdata->unk4E0 = 8;
                objdata->flags |= THORNTAILFLAG_GoToNextNode;
            }
            break;
        case THORNTAILSTATE_BlockingProgression:
            // Sleep regardless of time of day to block player progression
            if (main_get_bits(objdata->progressionBlockerGamebit) != 0) {
                objdata->flags &= ~THORNTAILFLAG_ProgressionBlocker;
                objdata->flags |= THORNTAILFLAG_ForceModAnimChange;
                objdata->state = THORNTAILSTATE_WakingUp;
            }
            break;
        case THORNTAILSTATE_WakingUp:
            if (objdata->flags & THORNTAILFLAG_ModAnimDone) {
                objdata->unk4E0 = 8;
                objdata->flags |= THORNTAILFLAG_GoToNextNode;
            }
            break;
        case THORNTAILSTATE_FallingAsleep:
            if (objdata->flags & THORNTAILFLAG_ModAnimDone) {
                objdata->state = THORNTAILSTATE_Sleeping;
            }
            break;
        case THORNTAILSTATE_Sleeping:
            if (gDLL_7_Newday->vtbl->func8(&time) == 0) {
                objdata->state = THORNTAILSTATE_WakingUp;
            }
            break;
        default:
            if (objdata->flags & THORNTAILFLAG_ProgressionBlocker) {
                // Acting as a progression blocker
                objdata->state = THORNTAILSTATE_BlockingProgression;
            } else if (objdata->currentCurve->type1D.unk34 == THORNTAILCURVE_4) {
                // At river
                // @recomp: Don't try drinking from the river if it's still dry
                if (main_get_bits(DINOMOD_BIT_921_SH_RiverUnblocked)) {
                    objdata->state = THORNTAILSTATE_StartDrinking;
                } else {
                    objdata->state = THORNTAILSTATE_LookingAround;
                    objdata->unk4E0 = 8; // required, otherwise sleepy gets stuck
                }
            } else if (gDLL_7_Newday->vtbl->func8(&time) != 0) {
                // Nighttime
                objdata->state = THORNTAILSTATE_FallingAsleep;
            } else {
                // Eat that lovely SwapStone Hollow grass
                objdata->state = THORNTAILSTATE_Grazing;
                objdata->eatingTimer = rand_next(500, 800);
                objdata->grazingTimer = rand_next(2000, 3000);
            }
            break;
        }
    } else {
        // Moving toward next curve node
        objdata->distToTargetCurve = vec3_distance_xz(&self->globalPosition, &objdata->targetCurve->pos);
        if ((objdata->distToTargetCurve < 30.0f) && ((objdata->flags & THORNTAILFLAG_ModAnimDone) != 0)) {
            // Reached target curve
            objdata->flags |= THORNTAILFLAG_AtTarget;
            objdata->currentCurve = objdata->targetCurve;
            if (((objdata->unk4E0 == 9) && (objdata->currentCurve->type1D.unk34 != THORNTAILCURVE_4)) 
                    || (objdata->currentCurve->type1D.unk34 == THORNTAILCURVE_0)) {
                objdata->state = -1;
                objdata->flags |= THORNTAILFLAG_GoToNextNode;
            }
        } else {
            // Not at target curve yet
            switch (objdata->state) {
            case THORNTAILSTATE_Grazing:
            default:
                vx = objdata->targetCurve->pos.x - objdata->prevCurve->pos.x;
                vz = objdata->targetCurve->pos.z - objdata->prevCurve->pos.z;
                objdata->targetAngle = arctan2_f(-vx, -vz);
                objdata->startAngle = self->srt.yaw;
                objdata->angleToTarget = self->srt.yaw - (objdata->targetAngle & 0xFFFF);
                CIRCLE_WRAP(objdata->angleToTarget);
                if ((objdata->angleToTarget < 500) && (objdata->angleToTarget > -500)) {
                    objdata->state = THORNTAILSTATE_MoveToTarget;
                } else {
                    if (objdata->angleToTarget > M_90_DEGREES) {
                        objdata->angleToTarget = M_90_DEGREES;
                    } else if (objdata->angleToTarget < -M_90_DEGREES) {
                        objdata->angleToTarget = -M_90_DEGREES;
                    }
                    if (objdata->angleToTarget < 0) {
                        objdata->state = THORNTAILSTATE_TurnLeft;
                        objdata->unk848 = (f32) -objdata->angleToTarget;
                    } else {
                        objdata->state = THORNTAILSTATE_TurnRight;
                        objdata->unk848 = (f32) objdata->angleToTarget;
                    }
                    objdata->unk848 = (f32) (objdata->unk848 / M_90_DEGREES_F);
                    if (objdata->unk848 > 1.0f) {
                        objdata->unk848 = 1.0f;
                    } else if (objdata->unk848 < 0.0f) {
                        objdata->unk848 = 0.0f;
                    }
                }
                break;
            case THORNTAILSTATE_MoveToTarget:
                if ((objdata->unk4E0 == 8) && (objdata->targetCurve->type1D.unk34 != THORNTAILCURVE_0)) {
                    objdata->unk848 = (objdata->distToTargetCurve - 35.0f) / 45.0f;
                    if (objdata->unk848 < 0.0f) {
                        objdata->unk848 = 0.0f;
                    } else if (objdata->unk848 > 1.0f) {
                        objdata->unk848 = 1.0f;
                    }
                } else {
                    objdata->unk848 = 1.0f;
                }
                vx = objdata->targetCurve->pos.x - self->srt.transl.x;
                vz = objdata->targetCurve->pos.z - self->srt.transl.z;
                if (objdata->distToTargetCurve != 0.0f) {
                    vx /= objdata->distToTargetCurve;
                    vz /= objdata->distToTargetCurve;
                }
                objdata->flags |= THORNTAILFLAG_Walking;
                objdata->walkSpeed = 0.28f;
                self->velocity.x = objdata->walkSpeed * vx;
                self->velocity.z = objdata->walkSpeed * vz;
                obj_move(self, self->velocity.x * gUpdateRateF, 0.0f, self->velocity.z * gUpdateRateF);
                func_8002493C(self, objdata->walkSpeed, &objdata->modAnimDelta);
                break;
            case THORNTAILSTATE_TurnLeft:
            case THORNTAILSTATE_TurnRight:
                if (objdata->flags & THORNTAILFLAG_ModAnimDone) {
                    objdata->startAngle = self->srt.yaw;
                    objdata->angleToTarget = self->srt.yaw - (objdata->targetAngle & 0xFFFF);
                    CIRCLE_WRAP(objdata->angleToTarget);
                    if ((objdata->angleToTarget < 500) && (objdata->angleToTarget > -500)) {
                        objdata->state = THORNTAILSTATE_MoveToTarget;
                        objdata->unk848 = 1.0f;
                    } else {
                        if (objdata->angleToTarget > M_90_DEGREES) {
                            objdata->angleToTarget = M_90_DEGREES;
                        } else if (objdata->angleToTarget < -M_90_DEGREES) {
                            objdata->angleToTarget = M_90_DEGREES;
                        }
                        if (objdata->angleToTarget < 0) {
                            objdata->unk848 = (f32) -objdata->angleToTarget;
                        } else {
                            objdata->unk848 = (f32) objdata->angleToTarget;
                        }
                        objdata->unk848 = (f32) (objdata->unk848 / M_90_DEGREES_F);
                        if (objdata->unk848 > 1.0f) {
                            objdata->unk848 = 1.0f;
                        } else if (objdata->unk848 < 0.0f) {
                            objdata->unk848 = 0.0f;
                        }
                        objdata->flags |= THORNTAILFLAG_ForceModAnimChange;
                    }
                } else {
                    self->srt.yaw = (s16) ((f32) objdata->startAngle - (self->animProgress * (f32) objdata->angleToTarget));
                }
                break;
            }
        }
    }
    if (objdata->state != -1) {
        animIdx = objdata->state * 2;
        // Update model animation
        if ((sModAnimMap[animIdx] != self->curModAnimId) || (objdata->flags & THORNTAILFLAG_ForceModAnimChange)) {
            func_80023D30(self, sModAnimMap[animIdx], 0.0f, 0);
            if (!(objdata->flags & THORNTAILFLAG_Walking)) {
                objdata->modAnimDelta = data_1B8[objdata->state];
            }
            objdata->flags &= ~(THORNTAILFLAG_ForceModAnimChange | THORNTAILFLAG_ModAnimDone);
        }
        // Set layered model animation(?), if one is defined
        if (sModAnimMap[animIdx + 1] != -1) {
            func_80025540(self, sModAnimMap[animIdx + 1], (s32) (objdata->unk848 * 1023.0f));
        }
    }
    // Update z lock state
    if (sStateFlagMap[objdata->state] & THORNTAILSTATEFLAG_NoTargeting) {
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
    }
    // Update eyes
    eyelidR = func_800348A0(self, HEAD_ANIMATION_TAG_Eyelid_R, 0);
    eyelidL = func_800348A0(self, HEAD_ANIMATION_TAG_Eyelid_L, 0);
    if (sStateFlagMap[objdata->state] & THORNTAILSTATEFLAG_Asleep) {
        if (eyelidR != NULL) {
            eyelidR->frame = (2 << 8);
        }
        if (eyelidL != NULL) {
            eyelidL->frame = (2 << 8);
        }
    } else {
        if (eyelidR != NULL) {
            eyelidR->frame = 0;
        }
        if (eyelidL != NULL) {
            eyelidL->frame = 0;
        }
    }
    // Check if model animation completed
    if (func_80024108(self, objdata->modAnimDelta, gUpdateRateF, &sp3C) != 0) {
        objdata->flags |= THORNTAILFLAG_ModAnimDone;
    } else {
        objdata->flags &= ~THORNTAILFLAG_ModAnimDone;
    }
}

RECOMP_PATCH void thorntail_sleepy_setup(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup) {
    static u8 arentLikeTheyUsedToSeq = THORNTAILSEQ_11_ArentLikeTheyUsedToBe;
    static u8 itsANiceDaySeq = THORNTAILSEQ_7_ItsANiceDay;

    objdata->flags |= THORNTAILFLAG_RotateTalkSeqs;
    objdata->progressionBlockerGamebit = BIT_SH_Move_Thorntail_Blocking_Hollow_Log;
    thorntail_sleepy_init(self, objdata, setup);

    // @recomp: Change talk seq on game state change (if none of these cases are picked, the
    //          usual "have you woken the swapstone" will play).
    if (main_get_bits(DINOMOD_BIT_921_SH_RiverUnblocked)) {
        objdata->talkSeqs = &itsANiceDaySeq;
        objdata->talkSeqsCount = 1;
        objdata->flags &= ~THORNTAILFLAG_RotateTalkSeqs;
    } else if (main_get_bits(BIT_Talked_to_Rocky)) {
        objdata->talkSeqs = &arentLikeTheyUsedToSeq;
        objdata->talkSeqsCount = 1;
        objdata->flags &= ~THORNTAILFLAG_RotateTalkSeqs;
    }
}

RECOMP_PATCH void thorntail_elder_init(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup) {
    static u8 theRiverFlowsSeq = 9;

    thorntail_sleepy_init(self, objdata, setup);
    // @recomp: Change talk seqs depending on game state
    if (!main_get_bits(DINOMOD_BIT_921_SH_RiverUnblocked)) {
        // Usual "the sharpclaw drained our driver"
        objdata->talkSeqs = sElderTalkSeqs;
        objdata->talkSeqsCount = ARRAYCOUNT(sElderTalkSeqs);
    } else {
        // "The river flows to diamond bay"
        objdata->talkSeqs = &theRiverFlowsSeq;
        objdata->talkSeqsCount = 1;
        objdata->flags &= ~THORNTAILFLAG_RotateTalkSeqs;
    }
}

static _Bool player_has_spellstone(void) {
    return (main_get_bits(BIT_SpellStone_DIM_Activated) && !main_get_bits(BIT_877))
        || (main_get_bits(BIT_SpellStone_WC) && !main_get_bits(BIT_DB_Unlock_Act_Three))
        || (main_get_bits(BIT_SpellStone_DR)); // Note: Don't revert to non-spellstone state after DR
}

RECOMP_PATCH void thorntail_trader_act1_control(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup) {
    static u8 burrowThornsHintSeq = THORNTAILSEQ_10_ThornsBlockingBurrows;
    static u8 howsRandornSeq = THORNTAILSEQ_6_HowsRandorn;
    static u8 noLogsSeq = THORNTAILSEQ_4_NoLogsToday;

    Object *sidekick;
    Object *player;
    f32 time;

    sidekick = get_sidekick();
    player = get_player();

    // @recomp: Change talk seqs depending on game state
    if (!main_get_bits(DINOMOD_BIT_921_SH_RiverUnblocked)) {
        // Give (vague) hint about burrows
        objdata->talkSeqs = &burrowThornsHintSeq;
        objdata->talkSeqsCount = 1;
        objdata->flags &= ~THORNTAILFLAG_RotateTalkSeqs;
    } else if (player_has_spellstone()) {
        // Don't say there's no logs today
        objdata->talkSeqs = &howsRandornSeq;
        objdata->talkSeqsCount = 1;
        objdata->flags &= ~THORNTAILFLAG_RotateTalkSeqs;
    } else {
        // River unblocked, but player doesn't have a SpellStone. Tell them to find more!
        objdata->talkSeqs = &noLogsSeq;
        objdata->talkSeqsCount = 1;
        objdata->flags &= ~THORNTAILFLAG_RotateTalkSeqs;
    }

    thorntail_common_control(self, objdata, setup);

    // @recomp: Cancel distract after waking up from using distract
    if (objdata->state == THORNTAILSTATE_WakingUp && (objdata->flags & THORNTAILFLAG_ModAnimDone) && objdata->wakingFromDistract) {
        ((DLL_ISidekick*)sidekick->dll)->vtbl->func21(sidekick, 0, 0);
        objdata->wakingFromDistract = FALSE;
    }
    if ((objdata->state == THORNTAILSTATE_BlockingProgression) && (sidekick != NULL)) {
        if (vec3_distance_squared(&player->globalPosition, &self->globalPosition) < SQ(70.0f)) {
            // Allow distract command
            ((DLL_ISidekick*)sidekick->dll)->vtbl->func14(sidekick, 2);
        }
        if (((DLL_ISidekick*)sidekick->dll)->vtbl->func24(sidekick) != 0) {
            // Distract successfully used
            // @recomp: Don't try to wake up if it's nighttime, otherwise we will just awkwardly flip back to the sleep
            //          state partway through the waking up state.
            if (gDLL_7_Newday->vtbl->func8(&time) == 0) {
                main_set_bits(objdata->progressionBlockerGamebit, 1);
                objdata->wakingFromDistract = TRUE;
            }
        }
    }
}

