#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

typedef struct {
/*00*/    ObjSetup base;
/*18*/    s16 gamebitOpenA;           //animCallback advances door to "Opening" state when this gamebit (and gamebitOpenB if specified) is set, or to "Closing" state when unset (obj must be in a sequence!)
/*1A*/    s16 gamebitRestoreState;    //Restores state when the door loads (either "Open" or "Closed" state) - objSeq will use a preemptTime to skip the door to its open position when restoring "Open" state.
/*1C*/    s16 preemptTime;            //The sequence time to jump to when the object loads in its "Open" state (state restored via `gamebitRestoreState`).
/*1E*/    s8 objSeqIdx;               //The door opening cutscene's objSeqIdx
/*1F*/    u8 yaw;
/*20*/    u8 preemptEnabledActors;    //Configures which actors to include when the door's sequence is played using a preemptTime
/*21*/    u8 scale;                   //Scale factor for the door (0x40 is 1x scale)
/*22*/    s16 gamebitOpenB;           //When specified, animCallback advances door to "Opening" state when this gamebit and gamebitOpenA are set (obj must be in a sequence!)
/*24*/    s16 gamebitCameraBack;      //When specified, animCallback flips part (`flipBitsCameraBack`) of this gamebit's value if the camera is behind the door when the door finishes opening/closing (obj must be in a sequence!)
/*26*/    s16 gamebitCameraFront;     //When specified, animCallback flips part (`flipBitsCameraFront`) of this gamebit's value if the camera is in front of the door when the door finishes opening/closing (obj must be in a sequence!)
/*27*/    u8 flipBitsCameraBack;      //The section of `gamebitCameraBack`'s value to flip when `gamebitCameraBack`'s value is updated
/*28*/    u8 flipBitsCameraFront;     //The section of `gamebitCameraFront`'s value to flip when `gamebitCameraFront`'s value is updated
/* RECOMP */
/*29*/    u8 options;     //Custom settings (see `SeqDoor_CustomOptions`)
/*2A*/    u8 range;       //Range for custom "don't move while player nearby" option
} SeqDoor_Setup;

typedef enum {
    SeqDoor_OPTION_1_Delay_Play_Until_Gamebit_Set = 1,
    SeqDoor_OPTION_2_Unload_If_Already_Open = 2,
    SeqDoor_OPTION_4_Unload_At_End_of_Sequence = 4,
    SeqDoor_OPTION_8_Sync_With_State_Gamebit = 8,
    SeqDoor_OPTION_10_Wait_While_Player_Nearby = 0x10,   //Don't close while the player is within a specified radius
    SeqDoor_OPTION_20_Wait_While_Sidekick_Nearby = 0x20, //Don't close while the sidekick is within a specified radius
    SeqDoor_OPTION_40_3D_Nearby_Check = 0x40             //Use a 3D distance check instead of a lateral 2D one (for player/sidekick nearby options)
} SeqDoor_CustomOptions;
