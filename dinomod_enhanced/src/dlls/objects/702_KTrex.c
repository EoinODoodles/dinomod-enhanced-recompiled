#include "modding.h"
#include "recompconfig.h"

#include "dlls/engine/18_objfsa.h"
#include "dlls/engine/33.h"
#include "dlls/objects/214_animobj.h"
#include "sys/gfx/model.h"
#include "sys/gfx/modgfx.h"
#include "sys/main.h"
#include "sys/objanim.h"
#include "sys/objhits.h"
#include "sys/objmsg.h"
#include "sys/objtype.h"
#include "segment_334F0.h"
#include "dll.h"

/**
 * Summary of recomp changes
 * ******************************
 * Vanilla (patches):
 * - Fix "move and check turn" logic to avoid softlock where boss keeps turning in place forever.
 * - Handle final ktdata pos update when transitioning into a turn. Ensures the boss is in the exact
 *   correct position before the turn 100% of the time.
 * - Play defeat cutscene and updated WC object groups after winning the fight.
 * - Fix case where boss could "see" the player when they are behind or directly underneath the boss.
 * - Correctly handle chargeCounter > 1, avoiding a badly desynced state.
 * - Lower floor switches immediately after zapping boss. Matches SFA behavior and possibly helps avoid
 *   a buggy end cutscene. Just a nice change to have, doesn't affect gameplay.
 * - Remove vulnerable flag immediately after logic state 8. Prevents the "fight progress" from being
 *   getting desynced leading to an eventual softlock.
 * - Ensure KT_RexDoorTrex closes even if the intro cutscene is skipped.
 *
 * Enhanced:
 * - Reduce vulnerable time as the fight progresses.
 * - Increase move speed during full charge (just a little).
 * - Skip "charge end" state after a normal charge (matches SFA).
 * - Use a different animation for a full charge end that plays a roar sound normally missing due to a bug.
 * - Switch to unused "intense" fight music while boss is charging (or preparing a charge).
 * - Don't let boss do a 180 if the player is in view.
 *
 * Enhanced (Hard Mode):
 * - Adjustments to speed, vulnerable times, and reverse/charge chances.
 * - Silent 180 turns.
 * - Sometimes charges for an extra segment.
 * - Always charges around the corner during the last phase if the player is in the next segment.
 */

enum KTRecompMode {
    KT_RECOMP_VANILLA,
    KT_RECOMP_ENHANCED,
    KT_RECOMP_HARD
};

static int dinomod_kt_enhanced(void) {
    return recomp_get_config_u32("kt_mode") != KT_RECOMP_VANILLA;
}

static int dinomod_kt_hard(void) {
    return recomp_get_config_u32("kt_mode") == KT_RECOMP_HARD;
}

#include "recomp/dlls/objects/702_KTrex_recomp.h"

enum KTLogicStates {
    KT_LSTATE_0 = 0,
    KT_LSTATE_1_DEFEATED = 1,
    KT_LSTATE_2_WALK = 2,
    KT_LSTATE_3_TURN_CORNER = 3,
    KT_LSTATE_4_ROAR = 4,
    KT_LSTATE_5_CHARGE = 5,
    KT_LSTATE_6_CHARGE_END = 6,
    KT_LSTATE_7_ZAPPED = 7,
    KT_LSTATE_8_ON_GROUND = 8,
    KT_LSTATE_9_STAND_UP = 9,
    KT_LSTATE_10_FULL_CHARGE = 10,
    KT_LSTATE_11_REVERSE = 11
};

enum KTAnimStates {
    KT_ASTATE_0 = 0,
    KT_ASTATE_1_MOVING_STRAIGHT = 1,
    KT_ASTATE_2_TURN_90 = 2,
    KT_ASTATE_3_TURN_180 = 3,
    KT_ASTATE_4_ROAR = 4,
    KT_ASTATE_5_CHARGE_END = 5,
    KT_ASTATE_6_ZAPPED = 6,
    KT_ASTATE_7_ON_GROUND = 7,
    KT_ASTATE_8_STAND_UP = 8
};

enum KTFlags {
    // Whether the boss is moving clockwise (0) or counter-clockwise (1) around the arena.
    KTFLAG_REVERSED = 0x1,
    // Which arena segment the boss is in.
    // Bits 1-2.
    KTFLAG_SEGMENT = 0x6,
    // Whether the boss was just damaged by the player.
    KTFLAG_DAMAGED = 0x8,
    // Whether the boss can be damaged.
    KTFLAG_VULNERABLE = 0x10,
    // Whether the random chance reverse/charge was already rolled for the current logic state.
    KTFLAG_ROLLED_CHANCE = 0x20
};

#define KTFLAG_GET_SEGMENT(ktflags) ((ktflags >> 1) & 3)
#define KTFLAG_SET_SEGMENT(segment) (segment << 1)

// Note: This is not the only object data. DLL 33 data is prepended to this in memory.
typedef struct {
/*000*/ GenericStack *stateStack;
/*004*/ f32 timer;
        // Where in the current segment the boss in, ranging from 0-1.
        // When reversed (moving counter-clockwise), this value progresses from 1 to 0 instead of 0 to 1.
/*008*/ f32 segmentPos;
/*00C*/ s32 standingUpSegment; // The last segment the boss stood up in after being knocked down.
        // The following fields are curve XYZ coords. These determine the positions used
        // for the A and B sides of each arena segment, for both clockwise and counter-clockwise movement.
/*010*/ f32 segStartCW_X[4];
/*020*/ f32 segStartCW_Y[4];
/*030*/ f32 segStartCW_Z[4];
/*040*/ f32 segEndCW_X[4];
/*050*/ f32 segEndCW_Y[4];
/*060*/ f32 segEndCW_Z[4];
/*070*/ f32 segEndCCW_X[4]; // Reversed positions start here (see above comments)
/*080*/ f32 segEndCCW_Y[4];
/*090*/ f32 segEndCCW_Z[4];
/*0A0*/ f32 segStartCCW_X[4];
/*0B0*/ f32 segStartCCW_Y[4];
/*0C0*/ f32 segStartCCW_Z[4];
        // The current A and B positions of each segment.
        // These are flipped when the boss is reversed (moving counter-clockwise).
/*0D0*/ f32 *segA_X;
/*0D4*/ f32 *segA_Y;
/*0D8*/ f32 *segA_Z;
/*0DC*/ f32 *segB_X;
/*0E0*/ f32 *segB_Y;
/*0E4*/ f32 *segB_Z;
        // World position that the boss should be in.
        // Moves to this position on the next tick.
        // Calculated by combining the current segment pos with the target curve pos.
/*0E8*/ Vec3f pos;
        // Where in the boss's current segment the player is, ranging from 0-1.
        // Note: This is *not* the pos of the player in their current segment. If the player is not in
        // the same segment as the boss, this value is not well defined.
/*0F4*/ f32 playerSegmentPos;
/*0F8*/ s16 turnStartYaw; // The yaw of the boss at the start of a turn.
/*0FA*/ u16 flags;
        // = 0 when walking around normally.
        // = 1 when charging at sabre.
        // = 2 when charging around the arena after getting zapped.
/*0FC*/ u8 anger;
/*0FD*/ u8 roarType;
        // Which arena segment the boss is in (bitfield).
        // This is used to check if the boss is in the same segment as the player.
/*0FE*/ u8 selfSegmentBitfield;
        // Which arena segment the player is in (bitfield).
        // Determined by triggers that set/unset gamebits as the player walks around the arena.
/*0FF*/ u8 playerSegmentBitfield;
/*100*/ u8 laserWallBitfield; // Bitfield of which laser walls are active.
/*101*/ u8 fightProgress; // Increases as the player progresses the fight by damaging the boss.
/*102*/ u8 health;
/*103*/ s8 chargeCounter; // How many segments to charge down. Decrements after each turn while charging.
/*104*/ s32 fxFlags;
/*108*/ u8 breathingSfxIndex;
/*10C*/ SRT unk10C;
/*124*/ SRT unk124;
/*13C*/ SRT unk13C;
/*154*/ SRT unk154;
/*16C*/ Vec3f unk16C;
} KTrex_Data;

typedef struct {
/*00*/ DLL33_ObjSetup base;
/*38*/ f32 speeds[3]; // Movement speed, per "anger" level.
/*44*/ u16 roarTime[3]; // How long a roar lasts.
/*4A*/ u16 vulnerableTime[4]; // How long the boss is vulnerable for.
/*52*/ u8 reverseChance[4]; // Chance to reverse direction.
/*56*/ u8 chargeChance[4]; // Chance to charge without seeing Sabre.
} KTrex_ObjSetup;

enum KTModAnims {
    KTANIM_Idle = 0,
    KTANIM_Walk1 = 1, // anger level 0
    KTANIM_Walk2 = 2, // anger level 1
    KTANIM_Walk3 = 3, // anger level 2
    KTANIM_Roar_Small = 4,
    KTANIM_Roar_VeryBig = 5, // unused
    KTANIM_Roar_Big = 6,
    KTANIM_Walk_Slow = 7, // creepy, unused
    KTANIM_Turn_90_CW_Normal = 8,
    KTANIM_Roar_ChargeEnd1 = 9, // referenced but unused due to logic
    KTANIM_Roar_10 = 10, // unused
    KTANIM_FallDown = 11,
    KTANIM_FlailOnGround = 12,
    KTANIM_GetBackUp = 13,
    KTANIM_Turn_90_CCW_Normal = 14,
    KTANIM_Turn_180 = 15,
    KTANIM_Turn_90_CW_Fast = 16,
    KTANIM_Turn_90_CCW_Fast = 17,
    KTANIM_Roar_ChargeEnd2 = 18
};

enum KTModAnimEvents {
    KTANIM_EVT_0_Speak = 0,
    KTANIM_EVT_1_Footfall_Left = 1,
    KTANIM_EVT_2_Footfall_Right = 2,
    KTANIM_EVT_7 = 7,
    KTANIM_EVT_9_Spit_Partfx_Enable = 9,
    KTANIM_EVT_10_Spit_Partfx_Disable = 10
};

enum KTFxFlags {
    KTFX_Footfall_Right1 = 0x1,
    KTFX_Footfall_Left1 = 0x2,
    KTFX_Footfall_Right2 = 0x4,
    KTFX_Footfall_Left2 = 0x8,
    KTFX_Footfall_Right3 = 0x10,
    KTFX_Footfall_Left3 = 0x20,
    KTFX_Sound_68E = 0x40,
    KTFX_Sound_BigRoar = 0x80,
    KTFX_Sound_IntroRoar = 0x100,
    KTFX_Sound_ChargeEndRoar = 0x200,
    KTFX_Spit_Partfx = 0x800,
    KTFX_Spit_Partfx_Disable = 0x1000,
    KTFX_Sound_FlailRoar = 0x2000,
    KTFX_Sound_Breathing1 = 0x4000,
    KTFX_Sound_Breathing2 = 0x8000,
    KTFX_Sound_WallSlam = 0x10000,
    KTFX_Sound_Explosion = 0x20000,
    KTFX_Sound_GroundScrape = 0x40000,
    KTFX_Sound_PainRoar = 0x80000,
    KTFX_EnablePartFx = 0x100000,
};

extern s16 sChargeEndModAnims[3];
extern s16 sTurn90ModAnims[3][2];
extern f32 sTurn90AnimTickDeltas[3];
extern f32 _data_6C[3];
extern f32 _data_78[3];

extern DLL33_Data* sDLL33Data;
extern KTrex_Data *sKTData;

extern void dll_702_anim_event_to_fx(s32 modAnimEvent, s32 fxFlags);
extern s32 dll_702_move_and_check_turn(ObjFSA_Data* fsa, KTrex_Data* ktdata);
extern void dll_702_push_state(s32 state);
extern s32 dll_702_pop_state(void);
extern s32 dll_702_is_in_active_laser_wall(Object* self);

static s32 dinomodInitHack = FALSE;

// Util function of logic state 2's condition for whether Klanadack can see the player
static int dinomod_kt_can_see_player(void) {
    s32 reversed = sKTData->flags & KTFLAG_REVERSED;

    return ((sKTData->selfSegmentBitfield & sKTData->playerSegmentBitfield) && 
        ((!reversed && (sKTData->segmentPos + 0.25f) <= sKTData->playerSegmentPos) || (reversed && sKTData->playerSegmentPos <= (sKTData->segmentPos - 0.25f))));
}

static int dinomod_kt_is_player_in_next_segment(void) {
    static s8 segmentToBitfield[4] = {0x02, 0x08, 0x01, 0x04};

    s32 reversed = sKTData->flags & KTFLAG_REVERSED;
    s32 segment = KTFLAG_GET_SEGMENT(sKTData->flags);

    if (reversed) {
        segment -= 1;
        if (segment < 0) {
            segment = 3;
        }
    } else {
        segment += 1;
        if (segment > 3) {
            segment = 0;
        }
    }

    return sKTData->playerSegmentBitfield & segmentToBitfield[segment];
}

RECOMP_HOOK_DLL(dll_702_setup) void dll_702_setup_hook(Object* self, KTrex_ObjSetup* setup, s32 arg2) {
    dinomodInitHack = FALSE;

    if (dinomod_kt_enhanced()) {
        // @recomp: (enhanced) Difficulty adjustments
        if (!dinomod_kt_hard()) {
            setup->speeds[2] = 4.5f; // speed up full charge a little (4 -> 4.5)
            setup->vulnerableTime[1] = 500; // 6 -> 5 swipes
            setup->vulnerableTime[2] = 400; // 6 -> 4 swipes
            setup->vulnerableTime[3] = 300; // 6 -> 3 swipes
        } else  {
            setup->speeds[0] = 4.5f; // speed up normal speed (4 -> 4.5)
            setup->speeds[1] = 5.5f; // speed up normal charge (4 -> 5.5)
            setup->speeds[2] = 6.5f; // speed up full charge (4 -> 6.5)
            setup->vulnerableTime[0] = 400; // 6 -> 4 swipes
            setup->vulnerableTime[1] = 300; // 6 -> 3 swipes
            setup->vulnerableTime[2] = 200; // 6 -> 2 swipes
            setup->vulnerableTime[3] = 100; // 6 -> 1 swipes
            setup->reverseChance[0] = 10; // 0% -> 10%
            setup->reverseChance[1] = 20; // 5% -> 20%
            setup->reverseChance[2] = 30; // 10% -> 30%
            setup->reverseChance[3] = 40; // 20% -> 40%
            setup->chargeChance[0] = 10; // 0% -> 10%
            setup->chargeChance[1] = 20; // 5% -> 20%
            setup->chargeChance[2] = 30; // 10% -> 30%
            setup->chargeChance[3] = 40; // 20% -> 40%
        }
    }
}

RECOMP_HOOK_DLL(dll_702_free) void dll_702_free_hook(Object *self) {
    // @recomp: Free handles to music we used
    gDLL_5_AMSEQ2->vtbl->func1(self, 0xD8, 0, 0, 0);
    gDLL_5_AMSEQ2->vtbl->func1(self, 0xD9, 0, 0, 0);
    gDLL_5_AMSEQ2->vtbl->func1(self, 0xD5, 0, 0, 0);
}

RECOMP_PATCH s32 dll_702_move_and_check_turn(ObjFSA_Data* fsa, KTrex_Data* ktdata) {
    f32 posDelta;
    s32 reversed;
    s32 segment;
    s32 turn;

    turn = FALSE;
    reversed = ktdata->flags & KTFLAG_REVERSED;
    segment = KTFLAG_GET_SEGMENT(ktdata->flags);
    if (reversed) {
        posDelta = -fsa->speed;
    } else {
        posDelta = fsa->speed;
    }

    ktdata->segmentPos += posDelta * gUpdateRateF;

    // @recomp: Only check one side of the segment, depending on whether Klanadack is moving CW or CCW.
    //          Checking both (as vanilla does) is problematic if a turn does not move him far enough into the next segment,
    //          causing him to think he's at the end of the segment after just entering in, leading to an infinite loop of
    //          turning in place, softlocking the fight.
    if ((!reversed && _data_78[ktdata->anger] < ktdata->segmentPos) || (reversed && ktdata->segmentPos < _data_6C[ktdata->anger])) {
        // Reached end of segment, turn
        turn = TRUE;
        
        // @recomp: Switch up the order of this logic a little so that ktdata `pos` is correct and not set to the end of the next segment
        if (ktdata->segmentPos > _data_78[ktdata->anger]) {
            ktdata->segmentPos = _data_78[ktdata->anger];
        } else if (ktdata->segmentPos < _data_6C[ktdata->anger]) {
            ktdata->segmentPos = _data_6C[ktdata->anger];
        }

        ktdata->pos.x = ktdata->segA_X[segment] + ((ktdata->segB_X[segment] - ktdata->segA_X[segment]) * ktdata->segmentPos);
        ktdata->pos.y = ktdata->segA_Y[segment] + ((ktdata->segB_Y[segment] - ktdata->segA_Y[segment]) * ktdata->segmentPos);
        ktdata->pos.z = ktdata->segA_Z[segment] + ((ktdata->segB_Z[segment] - ktdata->segA_Z[segment]) * ktdata->segmentPos);

        if (reversed) {
            segment -= 1;
            if (segment < 0) {
                segment = 3;
            }
        } else {
            segment += 1;
            if (segment > 3) {
                segment = 0;
            }
        }
        ktdata->flags &= ~KTFLAG_SEGMENT;
        ktdata->flags |= KTFLAG_SET_SEGMENT(segment);
    } else {
        ktdata->pos.x = ktdata->segA_X[segment] + ((ktdata->segB_X[segment] - ktdata->segA_X[segment]) * ktdata->segmentPos);
        ktdata->pos.y = ktdata->segA_Y[segment] + ((ktdata->segB_Y[segment] - ktdata->segA_Y[segment]) * ktdata->segmentPos);
        ktdata->pos.z = ktdata->segA_Z[segment] + ((ktdata->segB_Z[segment] - ktdata->segA_Z[segment]) * ktdata->segmentPos);
    }
    
    return turn;
}

RECOMP_PATCH s32 dll_702_anim_state_2(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    MtxF tempMtx;
    f32 tempY;
    SRT tempSRT;
    u16 reversed;

    reversed = sKTData->flags & KTFLAG_REVERSED;
    if (fsa->enteredAnimState) {
        func_80023D30(self, sTurn90ModAnims[sKTData->anger & 0xFFFF][reversed], 0.0f, 0);
        fsa->animTickDelta = sTurn90AnimTickDeltas[sKTData->anger];
        sKTData->turnStartYaw = self->srt.yaw;

        // @recomp: Ensure position is updated from last tick of dll_702_move_and_check_turn.
        //          This position update doesn't normally happen which results in the boss being slightly
        //          further from the corner than intended, which can make him off center in the following
        //          segment after the turn. Really only noticeable at higher movement speeds.
        self->srt.transl.x = sKTData->pos.x;
        self->srt.transl.z = sKTData->pos.z;

        // @recomp: (hard) Increase turn speed on hard mode
        if (dinomod_kt_hard()) {
            fsa->animTickDelta *= (sKTData->anger == 0 ? 1.25f : 1.5f);
        }
    }
    dll_702_anim_event_to_fx(KTANIM_EVT_2_Footfall_Right, KTFX_Footfall_Right1); // on event 2 -> right footfall
    dll_702_anim_event_to_fx(KTANIM_EVT_1_Footfall_Left, KTFX_Footfall_Left1); // on event 1 -> left footfall
    dll_702_anim_event_to_fx(KTANIM_EVT_0_Speak, KTFX_Sound_68E); // on event 1 -> little noise he makes on the corner
    dll_702_anim_event_to_fx(KTANIM_EVT_7, KTFX_Sound_WallSlam); // on event 7 -> wall slam fx
    fsa->unk340 |= 1;
    gDLL_18_objfsa->vtbl->func7(self, fsa, gUpdateRateF, 3);
    tempSRT.yaw = sKTData->turnStartYaw;
    tempSRT.pitch = 0;
    tempSRT.roll = 0;
    tempSRT.transl.x = 0.0f;
    tempSRT.transl.y = 0.0f;
    tempSRT.transl.z = 0.0f;
    tempSRT.scale = 1.0f;
    matrix_from_srt(&tempMtx, &tempSRT);
    vec3_transform(&tempMtx, fsa->unk27C, 0.0f, -fsa->unk278, &self->speed.x, &tempY, &self->speed.z);
    if (reversed) {
        self->srt.yaw = (f32) sKTData->turnStartYaw + (16384.0f * self->animProgress);
    } else {
        self->srt.yaw = (f32) sKTData->turnStartYaw - (16384.0f * self->animProgress);
    }
    return 0;
}

RECOMP_PATCH s32 dll_702_anim_state_3(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    u16 reversed;

    reversed = sKTData->flags & KTFLAG_REVERSED;
    if (fsa->enteredAnimState) {
        func_80023D30(self, KTANIM_Turn_180, 0.0f, 0);
        fsa->animTickDelta = 0.005f;
        fsa->unk278 = 0.0f;
        fsa->unk27C = 0.0f;
        sKTData->turnStartYaw = self->srt.yaw;

        // @recomp: (hard) Increase turn speed on hard mode
        if (dinomod_kt_hard()) {
            fsa->animTickDelta *= 2.5f;
        }
    }
    if (reversed) {
        self->srt.yaw = (f32) sKTData->turnStartYaw + (32768.0f * self->animProgress);
    } else {
        self->srt.yaw = (f32) sKTData->turnStartYaw - (32768.0f * self->animProgress);
    }
    return 0;
}

RECOMP_PATCH s32 dll_702_anim_state_5(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    if (fsa->enteredAnimState) {
        if (!dinomod_kt_enhanced()) {
            func_80023D30(self, sChargeEndModAnims[sKTData->anger], 0.0f, 0);
        } else {
            // @recomp: (enhanced) Use unused roar that actually has a speak event
            func_80023D30(self, KTANIM_Roar_ChargeEnd1, 0.0f, 0);
        }
        fsa->animTickDelta = 0.005f;
        fsa->unk278 = 0.0f;
        fsa->unk27C = 0.0f;
    }
    // @bug: this event never plays with the above mod anim. the boss ends up doing a roar anim
    //       but with no sound here. the sound is a shorter quieter roar.
    dll_702_anim_event_to_fx(KTANIM_EVT_0_Speak, KTFX_Sound_ChargeEndRoar);
    return 0;
}

/** Plays the defeat cutscene, and makes sure the necessary Object Groups are loaded in Walled City for the follow-up cutscene (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_702_logic_state_1(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    if (fsa->enteredLogicState) {
        // @recomp: Replace existing logic
        main_set_bits(BIT_564, 1);
        gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_WALLED_CITY, 4, 1);
        gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_WALLED_CITY, 5, 1);
    }
    return 0;
}

RECOMP_PATCH s32 dll_702_logic_state_2(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    KTrex_ObjSetup* objsetup;
    s32 chanceIdx;
    s32 reversed;

    objsetup = (KTrex_ObjSetup*)self->setup;
    if (fsa->enteredLogicState) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, KT_ASTATE_1_MOVING_STRAIGHT);
        sKTData->anger = 0;
        sKTData->flags &= ~KTFLAG_ROLLED_CHANCE;
        fsa->speed = objsetup->speeds[sKTData->anger] / 1000.0f;
        // @recomp: (enhanced) Reset music when returning to normal walk
        if (dinomod_kt_enhanced() && dinomodInitHack) {
            gDLL_5_AMSEQ2->vtbl->func0(self, 0xD8, 0, 0, 0);
        }
    }

    // @recomp: Set fight music and close doors even if intro cutscene is skipped (this is a little hacky)
    if (!dinomodInitHack && sKTData->segmentPos >= 0.5f) {
        dinomodInitHack = TRUE;
        gDLL_5_AMSEQ2->vtbl->func0(self, 0xD8, 0, 0, 0);
        main_set_bits(BIT_570, 0); // close KT_RexDoorTrex (not perfect but it works)
    }

    if (dll_702_move_and_check_turn(fsa, sKTData) != 0) {
        dll_702_push_state(KT_LSTATE_2_WALK);
        return KT_LSTATE_3_TURN_CORNER + 1;
    }

    reversed = sKTData->flags & KTFLAG_REVERSED;
    if (sKTData->anger == 0) {
        if ((sKTData->fightProgress >= 2) && !(sKTData->flags & KTFLAG_ROLLED_CHANCE) && 
                ((!reversed && sKTData->segmentPos >= 0.7f) || (reversed && sKTData->segmentPos <= 0.3f))) {
            chanceIdx = sKTData->fightProgress >> 1;
            // @recomp: (hard) Always charge around the corner if the player is in the next segment on hard mode in the last phase
            if (rand_next(0, 100) <= objsetup->chargeChance[chanceIdx] || 
                    (dinomod_kt_hard() && sKTData->fightProgress >= 6 && dinomod_kt_is_player_in_next_segment())) {
                sKTData->chargeCounter = 2;
                dll_702_push_state(KT_LSTATE_5_CHARGE);
                sKTData->roarType = 1;
                if (dinomod_kt_enhanced()) {
                    // @recomp: (enhanced) Kick in more intense version of music for charge
                    gDLL_5_AMSEQ2->vtbl->func0(self, 0xD9, 0, 0, 0);
                }
                if (dinomod_kt_enhanced() && rand_next(0, 100) <= 50) {
                    // @recomp: (hard) Charge an extra segment randomly on hard mode
                    sKTData->chargeCounter += 1;
                }
                return KT_LSTATE_4_ROAR + 1;
            }
            if (rand_next(0, 100) <= objsetup->reverseChance[chanceIdx]) {
                // @recomp: (enhanced) Don't turn around in front of the player!
                if (!dinomod_kt_enhanced() || !dinomod_kt_can_see_player()) {
                    sKTData->roarType = 0;
                    dll_702_push_state(KT_LSTATE_11_REVERSE);
                    return KT_LSTATE_4_ROAR + 1;
                }
            }
            sKTData->flags |= KTFLAG_ROLLED_CHANCE;
        }
    }
    // @recomp: Don't see player if they are behind (but far from the corner curve) or right below the boss (the +/- 0.25f part).
    //          The player segment position is determined by how far the player is from the start curve (not how close to the end),
    //          so if the player is behind Klanadack in a corner this logic will think the player is in front. Adding a little
    //          deadzone to the check ensures that he doesn't awkwardly start a charge while the player is below/behind him.
    if ((sKTData->selfSegmentBitfield & sKTData->playerSegmentBitfield) && 
            ((!reversed && (sKTData->segmentPos + 0.25f) <= sKTData->playerSegmentPos) || (reversed && sKTData->playerSegmentPos <= (sKTData->segmentPos - 0.25f)))) {
        // Player is ahead of the boss, in the same segment. Charge!
        sKTData->chargeCounter = 1;
        dll_702_push_state(KT_LSTATE_5_CHARGE);
        sKTData->roarType = 1;
        if (dinomod_kt_enhanced()) {
            // @recomp: (enhanced) Kick in more intense version of music for charge
            gDLL_5_AMSEQ2->vtbl->func0(self, 0xD9, 0, 0, 0);
        }
        if (dinomod_kt_hard() && ((!reversed && sKTData->segmentPos >= 0.5f) || (reversed && sKTData->segmentPos <= 0.5f))) {
            sKTData->chargeCounter += 1;
        }
        return KT_LSTATE_4_ROAR + 1;
    }
    return 0;
}

RECOMP_PATCH s32 dll_702_logic_state_5(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    KTrex_ObjSetup* objsetup;

    objsetup = (KTrex_ObjSetup*)self->setup;
    if (fsa->enteredLogicState) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, KT_ASTATE_1_MOVING_STRAIGHT);
        sKTData->anger = 1;
        fsa->speed = objsetup->speeds[sKTData->anger] / 1000.0f;
    }
    if (dll_702_move_and_check_turn(fsa, sKTData) != 0) {
        sKTData->chargeCounter -= 1;
        // @recomp: If we're at a turn and not ready to end the charge, actually enter the turning state
        //          and return back to this charge state instead of staying in this state. Otherwise, the
        //          segment Klanadack is in is desync'd with his rotation. Also, due to segmentPos only
        //          being reset in the turn state, skipping that state means move_and_check_turn will increment
        //          the segment twice, causing Klanadack to warp around the arena.
        //
        //          The (assumed) intent here, is that setting chargeCounter > 1 would cause Klanadack to preserve
        //          his charge across multiple segments. This is used by the random chance charge that should have 
        //          him charge to the end of the current segment, turn, then charge down the next segment before 
        //          ending the charge.
        if (sKTData->chargeCounter > 0) {
            dll_702_push_state(KT_LSTATE_5_CHARGE);
        } else {
            dll_702_push_state(KT_LSTATE_2_WALK);
            // @recomp: (enhanced) Skip charge end state for normal charges. Makes the fight flow better (SFA has this change)
            if (!dinomod_kt_enhanced()) {
                dll_702_push_state(KT_LSTATE_6_CHARGE_END);
            }
        }
        return KT_LSTATE_3_TURN_CORNER + 1;
    }
    if (dll_702_is_in_active_laser_wall(self) != 0) {
        return KT_LSTATE_7_ZAPPED + 1;
    }
    return 0;
}

RECOMP_PATCH s32 dll_702_logic_state_6(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    if (fsa->enteredLogicState) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, KT_ASTATE_5_CHARGE_END);
        if (dinomod_kt_enhanced()) {
            // @recomp: (enhanced) Reset music after charge
            gDLL_5_AMSEQ2->vtbl->func0(self, 0xD8, 0, 0, 0);
        }
    } else if (fsa->unk33A != 0) {
        return dll_702_pop_state() + 1;
    }

    return 0;
}

RECOMP_PATCH s32 dll_702_logic_state_7(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    if (fsa->enteredLogicState) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, KT_ASTATE_6_ZAPPED);
        self->unkAF &= ~0x8;
        // @recomp: Increment fight progress early (and always) to disable floor switches (we decrement 
        //          this in logic state 10 if the boss was not damaged to undo this). Mimics SFA behavior.
        //          I think this also helps avoid the camera bugging out in the end cutscene?
        sKTData->fightProgress += 1;
        main_set_bits(BIT_572_KT_FightProgress, sKTData->fightProgress);
    } else if (fsa->unk33A != 0) {
        return KT_LSTATE_8_ON_GROUND + 1;
    }

    return 0;
}

RECOMP_PATCH s32 dll_702_logic_state_8(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    KTrex_ObjSetup* objsetup;

    objsetup = (KTrex_ObjSetup*)self->setup;
    if (fsa->enteredLogicState) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, KT_ASTATE_7_ON_GROUND);
        sKTData->timer = (f32) objsetup->vulnerableTime[sKTData->fightProgress >> 1];
        sKTData->flags |= KTFLAG_VULNERABLE;
        sKTData->flags &= ~KTFLAG_DAMAGED;
        self->unkAF &= ~8;
    } else {
        if ((sKTData->flags & KTFLAG_DAMAGED) || (sKTData->timer -= gUpdateRateF) <= 0.0f) {
            // @recomp: Disable vulnerable state. This state isn't normally cleared, allowing the player to set
            //          the KTFLAG_DAMAGED flag outside of this logic state. Best case, the audio sfx plays for
            //          damaging him and nothing else. Worst case, this is done during logic state 9 which results
            //          in the fight progress being incremented without his health being decremented. This desync
            //          eventually leads to out of bounds array access with higher than intended fight progress.
            sKTData->flags &= ~KTFLAG_VULNERABLE;
            if (sKTData->flags & KTFLAG_DAMAGED) {
                sKTData->health -= 1;
            }
            if (sKTData->health <= 0) {
                if (dinomod_kt_enhanced()) {
                    // @recomp: (enhanced) Reset music to start music for defeat cutscene
                    gDLL_5_AMSEQ2->vtbl->func0(self, 0xD5, 0, 0, 0);
                }
                return KT_LSTATE_1_DEFEATED + 1;
            }
            if (dinomod_kt_enhanced()) {
                // @recomp: (enhanced) Reset music after taking damage (as he calmly stands up)
                gDLL_5_AMSEQ2->vtbl->func0(self, 0xD8, 0, 0, 0);
            }
            self->unkAF |= 8;
            return KT_LSTATE_9_STAND_UP + 1;
        }
    }

    return 0;
}

RECOMP_PATCH s32 dll_702_logic_state_9(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    u16 ktflags;
    
    if (fsa->enteredLogicState) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, KT_ASTATE_8_STAND_UP);
    } else if (fsa->unk33A != 0) {
        // @recomp: Don't increment fight progress here, we do this in logic state 7 now
        // if (sKTData->flags & KTFLAG_DAMAGED) {
        //     sKTData->fightProgress += 1;
        //     main_set_bits(BIT_572_KT_FightProgress, sKTData->fightProgress);
        // }
        ktflags = sKTData->flags;
        sKTData->standingUpSegment = KTFLAG_GET_SEGMENT(ktflags);
        sKTData->timer = 300.0f;
        gDLL_2_Camera->vtbl->func8(2, 0);
        if (dinomod_kt_enhanced()) {
            // @recomp: (enhanced) Kick in more intense version of music for charge
            gDLL_5_AMSEQ2->vtbl->func0(self, 0xD9, 0, 0, 0);
        }
        // @recomp: (hard) Compensate for increased move speed
        if (dinomod_kt_hard()) {
            sKTData->timer /= 2.0f;
        }
        return KT_LSTATE_10_FULL_CHARGE + 1;
    }
    return 0;
}

RECOMP_PATCH s32 dll_702_logic_state_10(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    KTrex_ObjSetup* objsetup;
    s32 segment;
    s32 reversed;

    objsetup = (KTrex_ObjSetup*)self->setup;
    segment = KTFLAG_GET_SEGMENT(sKTData->flags);
    reversed = sKTData->flags & KTFLAG_REVERSED;
    if (fsa->enteredLogicState) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, KT_ASTATE_1_MOVING_STRAIGHT);
        sKTData->anger = 2;
        fsa->speed = objsetup->speeds[sKTData->anger] / 1000.0f;
    }
    if (dll_702_move_and_check_turn(fsa, sKTData) != 0) {
        dll_702_push_state(KT_LSTATE_10_FULL_CHARGE);
        return KT_LSTATE_3_TURN_CORNER + 1;
    }
    sKTData->timer -= gUpdateRateF;
    if (sKTData->timer <= 0.0f) {
        sKTData->timer = 0.0f;
    }
    if ((sKTData->timer <= 0.0f) && (segment == sKTData->standingUpSegment) && 
            ((!reversed && sKTData->segmentPos >= 0.75f) || (reversed && sKTData->segmentPos <= 0.25f))) {
        if (sKTData->flags & KTFLAG_DAMAGED) {
            sKTData->fightProgress += 1;
            if (dinomod_kt_enhanced() && dinomod_kt_can_see_player()) {
                // @recomp: (enhanced) If player is in view, don't turn around
                dll_702_push_state(KT_LSTATE_2_WALK);
            } else {
                sKTData->roarType = 0;
                dll_702_push_state(KT_LSTATE_11_REVERSE);

                // @recomp: (hard) Skip roar and do a silent turn on hard mode
                if (!dinomod_kt_hard()) {
                    dll_702_push_state(KT_LSTATE_4_ROAR);
                }
            }
        } else {
            // @recomp: Decrement fight progress if not damaged (an above patch makes this always get incremented)
            sKTData->fightProgress -= 1;
            dll_702_push_state(KT_LSTATE_2_WALK);
        }
        gDLL_2_Camera->vtbl->func8(3, 0);
        main_set_bits(BIT_572_KT_FightProgress, sKTData->fightProgress);
        return KT_LSTATE_6_CHARGE_END + 1;
    }
    return 0;
}
