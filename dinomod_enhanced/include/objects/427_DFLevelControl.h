#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

//TEMPORARY DEFINES
typedef enum {
    DF_ObjGroup0_Foodbag_SharpClaw_Cave, //The eastern cave in the lower falls, near the exit to MMP
    DF_ObjGroup1_Entrance_Magic_Plant_Basin, //The secluded area up a side pathway along the cascading route into Discovery Falls
    DF_ObjGroup2_Lower_Falls, //The main lower tier of Discovery Falls
    DF_ObjGroup3_Shrine_Exterior, //The area outside the shrine
    DF_ObjGroup4_Toxic_Cave, //The cave with noxious gas, in the north-west of the Falls' middle tier
    DF_ObjGroup5_Mole_Cave, //The two-tiered cave with the SharpClaw and the hungry mole
    DF_ObjGroup6_Middle_Falls, //The middle tier of Discovery Falls, with the pulley and cradle
    DF_ObjGroup7, //Empty
    DF_ObjGroup8_Whirlpool_Cave, //The south-east cave, near the upper falls and shrine
    DF_ObjGroup9_Whirlpool_Cave_HitAnimator, //Removes the wall crack?
    DF_ObjGroup10_Mole_Cave_Podium, //One of the shrine switches
    DF_ObjGroup11_Shrine_Door, //The shrine's door pieces, and related SeqObjs
    DF_ObjGroup12_Upper_Falls_Stalactite_Cave, //The upper cave leading down towards the whirlpool cave 
    DF_ObjGroup13_Upper_Falls_Demolition_Cave, //The cave network just outside the whirlpool cave, with SharpClaw and explosive barrels
    DF_ObjGroup14_Middle_and_Upper_Falls, //Various objects near the upper falls' turbine, including the middle falls' projectile switches and fish
    DF_ObjGroup15,  //Empty
    DF_ObjGroup16, //Empty
    DF_ObjGroup17_Shrine_Exterior_Whirlpool_Cave_Waterfall //Texscroll for the Whirlpool Cave's exterior waterfall (once blown up)
} DF_ObjectGroups;

#define BIT_DF_Kyte_Secured_Rope_Near_BWC 0x10B
#define BIT_DF_Kyte_Secured_Rope_Upper_Falls 0x28F
//END OF TEMPORARY DEFINES

enum DF_CustomObjectGroups {
    DF_ObjGroup_Rope_BWC_Detached = 20,         //The original DFropenode setup near BWC, but moved into an objGroup of its own (synced with `DF_ObjGroup2_Lower_Falls` via `DFlevelcontrol`)
    DF_ObjGroup_Rope_BWC_Attached,              //A duplicate of the rope setup near BWC, but already attached (used for restoring state without seq preempt messing up rope's spring dynamics)
    DF_ObjGroup_Rope_Upper_Falls_Detached,      //The original upper falls DFropenode setup, but moved into an objGroup of its own (synced with `DF_ObjGroup14_Middle_and_Upper_Falls` via `DFlevelcontrol`)
    DF_ObjGroup_Rope_Upper_Falls_Attached,      //A duplicate of the upper falls' rope setup, but already attached (used for restoring state without seq preempt messing up rope's spring dynamics)
    DF_ObjGroup_Turbine_Sequence_HitAnimators   //Enables widescreen fixes just while the turbine sequence is active
};
