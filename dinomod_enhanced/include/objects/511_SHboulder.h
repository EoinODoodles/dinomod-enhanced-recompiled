#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 pitch;
    s8 roll;
    u8 scale;               //100 = 1.0f scale
    s16 gamebitGone;        //Gamebit to set when destroyed (also destroys the boulder when gamebit set)
    u8 invincible;          //Boulder can't be destroyed by explosions (for the custom one blocking Willow Grove)
} SHboulder_Setup;          //@recomp: custom setup struct

