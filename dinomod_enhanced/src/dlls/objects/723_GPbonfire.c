#include "modding.h"

#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "dll.h"

#include "recomp/dlls/objects/723_GPbonfire_recomp.h"

typedef struct {
/*00*/ u8 stateIndex;
/*01*/ u8 currentState;
/*02*/ u8 previousState;
/*04*/ s32 sequenceIndexKindling;   //the cutscene for placing the kindling
/*08*/ s32 sequenceIndexBurning;    //the cutscene for Kyte starting the fire
/*0C*/ CurvesFunc1BCReturnInner* curves;
/*10*/ s16 gameBitKindlingPlaced;   //flag to set when kindling is ready to burn
/*12*/ s16 gameBitBurning;          //flag to set when the bonfire's burning
/*14*/ s32 unused14;
/*18*/ u32 soundHandles[2];         //burning sfx
/*20*/ s16 timer;                   //counts down until the bonfire dwindles by one stage
/*22*/ s8 weedsDeposited;
/*23*/ u8 updateFireEffect;         //replaces the modgfx fire effect when its size changes
/*24*/ u8 callbackBool;             //Boolean associated with the cutscene callback function
} GPBonfireState;

typedef struct {
/*00*/ ObjCreateInfo base;
/*18*/ u8 interactionDistance;
/*1A*/ u16 kyteCurveID;
/*1C*/ s16 unused1C;
/*1E*/ s8 unused1E;
/*1F*/ u8 yaw;
} GPBonfireCreateInfo;

typedef struct {
s8 unk0[0x62 - 0];
u8 unk62;
s8 unk63[0x7A - 0x63];
s16 unk7A;
s8 unk7C[0x8D - 0x7C];
u8 unk8D;
u8 unk8E[0x98 - 0x8E];
u8 unk98;
} CallbackBCUnkArg2;

enum GPBonfireStates{
    STATE_0_INITIALISE = 0,
    STATE_1_WAIT_FOR_PLAYER_INTERACTION = 1,
    STATE_2_WAIT_FOR_KYTE = 2,
    STATE_3_START_BURNING = 3,
    STATE_4_ADDING_TUMBLEWEEDS = 4
};

/** Scale values for the GPbonfire object itself as the fire is built up */
/*0x0*/ static f32 bonfireScaleData[] = {
    0.1, 0.14, 0.196, 0.2744, 0.38415999
};
/** Scale values for the modgfx fire effect as the fire is built up */
/*0x14*/ static f32 modgfxScaleData[] = {
    6, 12, 18, 24, 30
};

/** How long (in 60ths of a second) until the bonfire dwindles by one stage */
#define BONFIRE_DWINDLING_TIMER 2000

extern void GPbonfire_func_A44(Object* self);
extern s32 GPbonfire_callbackBC(Object* self, s32 arg1, CallbackBCUnkArg2* arg2, s32 arg3);

//Allows GPbonfire to be lit and ChimneySweep immediately lifted (originally by jeebs2kx)
RECOMP_PATCH void GPbonfire_create(Object* self, GPBonfireCreateInfo* createInfo, s32 arg2) {
    GPBonfireState* state;

    state = self->state;
    self->unk0xbc = (void*)&GPbonfire_callbackBC;
    self->srt.yaw = createInfo->yaw << 8;
    state->stateIndex = STATE_0_INITIALISE;
    state->currentState = 0;
    state->soundHandles[0] = 0;
    state->soundHandles[1] = 0;
    obj_add_object_type(self, 0x30);
    state->gameBitKindlingPlaced = BIT_GP_Bonfire_Kindling_Placed;
    state->gameBitBurning = BIT_GP_ChimneySweep_Lifted; //@recomp: skip to bonfire end state
    state->sequenceIndexKindling = 0;
    state->sequenceIndexBurning = 1;
    state->unused14 = 0;
    state->weedsDeposited = 0;
}
