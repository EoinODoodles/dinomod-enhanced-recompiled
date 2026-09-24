#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 yaw;
/*19*/ s8 spawnLogDisabled;
/*1A*/ s16 range;
/*1C*/ s16 _unk1C;
/*1E*/ s16 _unk1E; //Unused, but may be a gamebit? Either 0 or -1 in Rare's instances.
/*20*/ s16 _unk20; //Unused, but may be a gamebit? Either 0 or -1 in Rare's instances.
/*21*/ s8 dismountOffsetX; //@recomp (repurposed padding): position offset from self, for direction to dismount towards
/*21*/ s8 dismountOffsetZ; //@recomp (repurposed padding): position offset from self, for direction to dismount towards
} DFdockpoint_Setup;
