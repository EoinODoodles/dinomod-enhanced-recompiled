#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "game/objects/interaction_arrow.h"
#include "sys/main.h"

#include "recomp/dlls/objects/318_fish_recomp.h"

//TEMPORARY DEFINES
#define fish_obj_Control fish_control
#define fish_isJumpPossible fish_is_jump_possible
#define fish_randomTurnSwim fish_random_turn_swim
#define fish_handleApproachingWall fish_handle_approaching_wall
#define fish_jumpHandleFlight fish_jump_handle_flight
#define fish_jumpHandleAnticipation fish_jump_handle_anticipation
#define fish_caughtByNet fish_caught_by_net
//END OF TEMPORARY DEFINES

typedef struct {
/*00*/    ObjSetup base;
/*18*/    u8 yaw;
/*19*/    u8 scale;
/*1A*/    u8 peekDuration;
/*1B*/    u8 bubbleRate;
/*1C*/    u16 swimDurationBeforePeek;     //How long to stay in State 2, before transitioning into peek state (#3)
/*1E*/    u16 respawnDuration;
/*20*/    u8 maxLateralSpeed;             //Stored at 100x actual value
/*21*/    u8 jumpDistanceLateral;
/*22*/    u8 jumpDuration;
/*23*/    u8 jumpAnticipateDuration;
/*24*/    u8 jumpRecoveryDuration;
/*26*/    u16 swimDurationAfterPeek;      //How long to stay in State 4, before transitioning into jump state
/*28*/    u8 surfaceOffsetY;
} fish_Setup;

typedef struct {
/*00*/    f32 animSpeed;
/*04*/    f32 timer;
/*08*/    f32 bubbleTimer;
/*0C*/    f32 bubbleFXParams[2];
/*14*/    f32 bubbleRate;
/*18*/    u8 state;
/*19*/    u8 randomTurnsPaused;
/*1A*/    s16 turn;
/*1C*/    s16 yawSpeed;
/*20*/    f32 maxLateralSpeed;
/*24*/    f32 maxSpeedApproachingWall;
/*28*/    f32 lateralSpeed;
/*2C*/    f32 surfaceY;
/*30*/    s16 objHitsConfig;
/*34*/    f32 unk34;                      //related to lateral speed during jump anticipation
/*38*/    f32 jumpAnticipationDistance;
/*3C*/    f32 unk3C;                      //related to height during jump anticipation
/*40*/    f32 unk40;                      //related to height during jump anticipation
/*44*/    f32 jumpDistanceLateral;
/*48*/    f32 jumpDuration;
/*4C*/    f32 jumpHeightFactor;
/*50*/    f32 jumpSpeedLateral;
/*54*/    f32 timerPrevJump;
/*58*/    f32 jumpedFromX;
/*5C*/    f32 jumpedFromY;
/*60*/    f32 jumpedFromZ;
/*64*/    f32 jumpFlightDuration;
/*68*/    f32 splashTime;
/*6C*/    f32 unk6C;                      //related to roll while anticipating jump
/*70*/    f32 unk70;                      //related to roll while following jump arc
/*74*/    f32 unk74;                      //related to roll while anticipating jump
/*78*/    f32 unk78;                      //related to roll while following jump arc
/*7C*/    u8 initialised;
} fish_Data;

typedef enum {
    Fish_STATE_0_Hidden,
    Fish_STATE_1_Fade_In,
    Fish_STATE_2_Swim_before_Peek,
    Fish_STATE_3_Peek_Up,
    Fish_STATE_4_Swim_after_Peek,
    Fish_STATE_5_Jump,
    Fish_STATE_6_Jump_Flight,
    Fish_STATE_7_Jump_Recovery
} fish_States;

typedef enum {
    Fish_MODANIM_0_Swim_LOOP,
    Fish_MODANIM_1_Jump,
    Fish_MODANIM_2_Tilted_Up,
    Fish_MODANIM_3_Tilted_Down
} fish_ModAnim;

#define FADE_IN_DURATION 60.0f

extern void fish_initialise(Object* self);
extern s32 fish_isJumpPossible(Object* self);
extern void fish_randomTurnSwim(Object* self);
extern void fish_swim(Object* self, s16 turnImpulse, u8 delerateAmount);
extern s32 fish_handleApproachingWall(Object* self, Vec3f* vFuture, Vec3s16* eulerRotation);
extern s32 fish_jumpHandleFlight(Object* self);
extern s32 fish_jumpHandleAnticipation(Object* self);
extern void fish_caughtByNet(Object* self);

/* Check if the fish's local Block is loaded, to reduce the likelihood of them drifting away into the void */
RECOMP_PATCH void fish_obj_Control(Object* self) {
    s32 damageType;
    fish_Setup* setup;
    f32 bubblesPerSecond;
    fish_Data* objdata;
    Object* player;
    f32 timerPow4;
    f32 timerSq;

    setup = (fish_Setup*)self->setup;
    objdata = self->data;

    //@recomp: don't do anything if the fish's local Block is unloaded (to avoid swimming off into the void)
    if (mapGetBlockByIndex(mapWorldCoordsToBlockIndex(self->globalPosition.x, self->globalPosition.y, self->globalPosition.z)) == NULL) {
        return;
    }

    player = objGetPlayer();

    if (objdata->initialised == FALSE) {
        fish_initialise(self);
    }

    //Allow targetting based on player-specific gamebits (whether Krystal/Sabre own a fishing net)
    if (player->id == OBJ_Krystal) {
        if (mainGetBits(BIT_Krystal_Fishing_Net) == FALSE) {
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
        } else {
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        }
    } else {
        if (mainGetBits(BIT_Sabre_Fishing_Net) == FALSE) {
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
        } else {
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        }
    }

    //Handle fishing net collision
    damageType = func_80025F40(self, NULL, NULL, NULL);
    if (damageType && (damageType == Damage_Type_Fishing_Net)) {
        fish_caughtByNet(self);
        return;
    }

    objdata->timer += gUpdateRateF;

    switch (objdata->state) {
    case Fish_STATE_0_Hidden:
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
        if (objdata->timer >= setup->respawnDuration) {
            objdata->state = Fish_STATE_1_Fade_In;
            objdata->timer -= setup->respawnDuration;
        }
        break;
    case Fish_STATE_1_Fade_In:
        if (objdata->timer <= FADE_IN_DURATION) {
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
            self->opacity = (s32) ((objdata->timer / FADE_IN_DURATION) * OBJECT_OPACITY_MAX);
            fish_swim(self, 0, 0);
        } else {
            self->opacity = OBJECT_OPACITY_MAX;
            self->objhitInfo->unk58 = objdata->objHitsConfig;
            objdata->state = Fish_STATE_2_Swim_before_Peek;
            objdata->timer -= FADE_IN_DURATION;
            fish_swim(self, 0, 0);
        }
        break;
    case Fish_STATE_2_Swim_before_Peek:
        if (objdata->timer <= setup->swimDurationBeforePeek) {
            fish_randomTurnSwim(self);
        } else {
            self->srt.transl.y = setup->base.y;
            objdata->state = Fish_STATE_3_Peek_Up;
            objdata->timer -= setup->swimDurationBeforePeek;
            fish_swim(self, 0, 0);
            objAnimSet(self, Fish_MODANIM_2_Tilted_Up, 0.0f, 0);
            objAnim_func_80024D74(self, 30);
        }
        break;
    case Fish_STATE_3_Peek_Up:
        if (objdata->timer <= setup->peekDuration) {
            self->srt.transl.y = setup->base.y + (((objdata->surfaceY - setup->base.y) * objdata->timer) / setup->peekDuration);
            fish_randomTurnSwim(self);
        } else {
            self->srt.transl.y = objdata->surfaceY;
            objdata->state = Fish_STATE_4_Swim_after_Peek;
            objdata->timer -= setup->peekDuration;
            fish_swim(self, 0, 0);
            objAnimSet(self, Fish_MODANIM_0_Swim_LOOP, 0.0f, 0);
            objAnim_func_80024D74(self, 30);
        }
        break;
    case Fish_STATE_4_Swim_after_Peek:
        if (objdata->timer >= setup->swimDurationAfterPeek) {
            if (fish_isJumpPossible(self)) {
                objdata->timer = gUpdateRateF;
                objdata->state = Fish_STATE_5_Jump;
                objdata->jumpedFromX = self->srt.transl.x;
                objdata->jumpedFromY = self->srt.transl.y;
                objdata->jumpedFromZ = self->srt.transl.z;
                objdata->timerPrevJump = 0.0f;
                
                objAnimSet(self, Fish_MODANIM_1_Jump, 0.0f, 0);
                objAnim_func_80024D74(self, 30);
                fish_jumpHandleAnticipation(self);
            } else {
                fish_swim(self, 0, 0);
            }
        } else {
            fish_randomTurnSwim(self);
        }

        //Create bubble particles when the player's nearby
        {
            timerSq = SQ(objdata->timer);
            timerPow4 = SQ(timerSq);
            objdata->bubbleTimer += gUpdateRateF;

            bubblesPerSecond = SQ(timerPow4) * objdata->bubbleRate;
            if (bubblesPerSecond > setup->bubbleRate) {
                bubblesPerSecond = setup->bubbleRate;
            }

            if (objdata->bubbleTimer >= 60.0f / bubblesPerSecond) {
                objdata->bubbleTimer -= 60.0f / bubblesPerSecond;
                if ((player != NULL) && (vec3DistanceSquared(&self->globalPosition, &player->globalPosition) < SQ(100))) {
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_26, NULL, 0, -1, objdata->bubbleFXParams);
                }
            }
        }
        break;
    case Fish_STATE_5_Jump:
        //Anticipate downwards before bursting up through the water
        if (fish_jumpHandleAnticipation(self) == FALSE) {
            //Store position and advance state when leaving the water
            objdata->jumpedFromX = self->srt.transl.x;
            objdata->jumpedFromY = self->srt.transl.y;
            objdata->jumpedFromZ = self->srt.transl.z;
            fish_jumpHandleFlight(self);
        }
        break;
    case Fish_STATE_6_Jump_Flight:
        if (fish_jumpHandleFlight(self) == FALSE) {
            fish_swim(self, 0, 0);
        }
        break;
    case Fish_STATE_7_Jump_Recovery:
        //Settling back into regular swimming, after the end of a jump
        if (objdata->timer <= setup->jumpRecoveryDuration) {
            self->srt.transl.y = objdata->surfaceY - (((objdata->surfaceY - setup->base.y) * objdata->timer) /setup->jumpRecoveryDuration);
            fish_randomTurnSwim(self);
        } else {
            self->srt.transl.y = setup->base.y;
            objdata->state = Fish_STATE_2_Swim_before_Peek;
            objdata->timer -= setup->jumpRecoveryDuration;
            fish_swim(self, 0, 0);
            objAnimSet(self, Fish_MODANIM_0_Swim_LOOP, 0.0f, 0);
            objAnim_func_80024D74(self, 30);
        }
        break;
    }

    objAnimAdvance(self, objdata->animSpeed, gUpdateRateF, NULL);
}
