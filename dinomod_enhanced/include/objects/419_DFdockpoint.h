#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 yaw;
/*19*/ s8 spawnLogDisabled;
/*1A*/ s16 range;
/*1C*/ u8 options;         //@recomp (repurposed padding)
/*1D*/ u8 _unk1D;
/*1E*/ s16 _unk1E; //Unused, but may be a gamebit? Either 0 or -1 in Rare's instances.
/*20*/ s16 _unk20; //Unused, but may be a gamebit? Either 0 or -1 in Rare's instances.
/*22*/ s8 dismountOffsetX; //@recomp (repurposed padding): position offset from self, for direction to dismount towards
/*23*/ s8 dismountOffsetZ; //@recomp (repurposed padding): position offset from self, for direction to dismount towards
} DFdockpoint_Setup;

/* Custom options */
typedef enum {
    //Don't give the log the DFdockpoint's mapID (useful for logs spawned
    //at the boundary between two levels, like DF's dockpoint just beside MMP)
    DFdockpoint_OPTION_1_Create_Log_Without_MapID = 1 
} DFdockpoint_Options;
