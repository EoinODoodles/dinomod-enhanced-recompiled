#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

/* RECOMP - edited version of struct */
typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 yaw;
/*19*/ u8 modelIdx;
/*1A*/ s16 gameBitPressed;         //Gamebit to set when switch is pressed down
/*1C*/ u8 yOffsetAnimation;        //How far down the switch should move when pressed
/*1D*/ u8 yThreshold;              //Threshold for other objects pressing switch
/*1E*/ u8 distanceGuardCommand;    //Player distance at which Guard sidekick command is selectable
/*20*/ s16 gamebitFinished;        //@recomp: unused gamebit's purpose changed (all instances previously had this set to NO_GAMEBIT) 
/*22*/ s16 gamebitPlayerOnSwitch;  //@recomp: repurposed passing, gamebit to set while the player is on the switch (not the sidekick)
} WCPressureSwitch_Setup;
