#pragma once

#include "PR/ultratypes.h"
#include "dlls/engine/27.h"
#include "game/objects/object.h"

typedef struct {
    DLL27_Data collision;       //Terrain/HITS data
    u16 characterDistance;      //Distance to character Tumbleweed's fleeing from (player/sidekick)
    u16 interactRadius;         //Distance at which player can interact with tumbleweed
    f32 baseScale;              //100% scale for Tumbleweed
    f32 timer;                  //Self-deletion timer, also used for scale-up speed when growing
    f32 twigSqueakTimer;        //Random interval between twigs' squeaks
    u8 state;                   //State machine phase
    u8 carryingGold;            //Bitfield flags: carrying a Shiny Nugget when bit0 is set
    u8 flags;                   //Flags for creating leaf/dust particles, or starting deletion timer
    s8 pad273;
    s16 rollSpeed;              //Rotation speed
    s16 pitchSpeed;             //Rotation speed
    s16 yawSpeed;               //Rotation speed
    s16 pad27A;
    Object* goldenNugget;       //Reference to Shiny Nugget
    Object* player;             //Reference to player
    s16 goldDroppedGamebit;     //GamebitID to set when Shiny Nugget dropped
    s16 pad286;
    f32 homeX;                  //World coord that fleeing Tumbleweeds return to
    f32 homeZ;                  //World coord that fleeing Tumbleweeds return to
    Vec3f* gravitateTarget;     //World coord for Tumbleweed to gravitate towards
    s16 carryMessageArgLo;      //Lower halfword of player message argument when Tumbleweed carried
    s16 carryMessageArgHi;      //Upper halfword of player message argument when Tumbleweed carried
    u8 carryFlags;              //Flags for Tumbleweed twigs when being carried (lifted/dropped/etc.)
    u8 beingCarried;            //Boolean for Tumbleweed twigs
    // RECOMP ADDITIONS //
    u8 destroySilently;         //Used to switch off sounds/particles when deleting Tumbleweeds for SC_beacon
} Tumbleweed_Data_Extended;

void tumbleweed_set_silent_delete(Object* self, int enable);
void tumbleweed_stop_being_carried(Object* self);
void tumbleweed_create_disintegrate_effects(Object* self, int createLeaves, int createDust, int playSound);
