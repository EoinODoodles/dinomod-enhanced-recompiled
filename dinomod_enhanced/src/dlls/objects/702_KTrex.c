#include "modding.h"

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
    // Whether the boss is currently charging forward. Not used for the full lap he does after getting zapped.
    KTFLAG_CHARGING = 0x20
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
/*108*/ u8 unk108;
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

extern f32 _data_6C[3];
extern f32 _data_78[3];

extern KTrex_Data *sKTData;

extern s32 dll_702_move_and_check_turn(ObjFSA_Data* fsa, KTrex_Data* ktdata);
extern void dll_702_push_state(s32 state);
extern s32 dll_702_is_in_active_laser_wall(Object* self);

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
    //          causing him to think he's at the end of the segment after just entering in, leading to and infinite loop of
    //          turning in place, softlocking the fight.
    if ((!reversed && _data_78[ktdata->anger] < ktdata->segmentPos) || (reversed && ktdata->segmentPos < _data_6C[ktdata->anger])) {
        // Reached end of segment, turn
        turn = TRUE;
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
        if (ktdata->segmentPos > _data_78[ktdata->anger]) {
            ktdata->segmentPos = _data_78[ktdata->anger];
        } else if (ktdata->segmentPos < _data_6C[ktdata->anger]) {
            ktdata->segmentPos = _data_6C[ktdata->anger];
        }
        if ((ktdata && ktdata) && ktdata){} // @fake
    }
    
    ktdata->pos.x = ktdata->segA_X[segment] + ((ktdata->segB_X[segment] - ktdata->segA_X[segment]) * ktdata->segmentPos);
    ktdata->pos.y = ktdata->segA_Y[segment] + ((ktdata->segB_Y[segment] - ktdata->segA_Y[segment]) * ktdata->segmentPos);
    ktdata->pos.z = ktdata->segA_Z[segment] + ((ktdata->segB_Z[segment] - ktdata->segA_Z[segment]) * ktdata->segmentPos);
    return turn;
}

/** Plays the defeat cutscene, and makes sure the necessary Object Groups are loaded in Walled City for the follow-up cutscene (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_702_logic_state_1(Object* self, ObjFSA_Data* fsa, f32 arg2) {
    if (fsa->enteredLogicState) {
        // @recomp: Replace existing logic
        main_set_bits(BIT_564, 1);
        gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_WALLED_CITY, 4, 1);
        gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_WALLED_CITY, 5, 1);
    }
    return 0;
}

RECOMP_PATCH s32 dll_702_logic_state_5(Object* self, ObjFSA_Data* fsa, f32 arg2) {
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
            return KT_LSTATE_3_TURN_CORNER + 1;
        }
    }
    if (sKTData->chargeCounter <= 0) {
        dll_702_push_state(KT_LSTATE_2_WALK);
        dll_702_push_state(KT_LSTATE_6_CHARGE_END);
        return KT_LSTATE_3_TURN_CORNER + 1;
    }
    if (dll_702_is_in_active_laser_wall(self) != 0) {
        return KT_LSTATE_7_ZAPPED + 1;
    }
    return 0;
}
