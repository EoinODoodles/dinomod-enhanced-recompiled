#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

/* RECOMP - edited version of struct */
typedef struct {
/*00*/ ObjSetup base;
/*19*/ s8 yaw;
/*19*/ s8 modelIndex;
/*1A*/ u8 _unk1A[0x1C - 0x1A];
/*1C*/ s16 gamebitSlab;     //@recomp: repurpose unused field
/*1E*/ s16 gamebitLit;
/*20*/ s16 gamebitRise;
/*22*/ s16 gamebitSwitch;   //@recomp: repurpose unused field
} WCBeacon_Setup;
