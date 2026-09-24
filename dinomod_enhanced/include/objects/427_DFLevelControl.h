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

#define BIT_DF_Play_Seq_0167_Seq_Arrival_So_This_Is_Discovery_Falls 0x15
#define BIT_DF_Played_Seq_0167_Arrival_So_This_Is_Discovery_Falls 0x16

#define BIT_DF_Play_Seq_0168_Krystal_Meets_HighTop_Archaeologist 0x17
#define BIT_DF_Played_Seq_0168_Krystal_Meets_HighTop_Archaeologist 0x18

#define BIT_DF_Play_Seq_02AD_Krystal_Meets_Foodbag_SharpClaw 0x4BB
#define BIT_DF_Played_Seq_02AD_Krystal_Meets_Foodbag_SharpClaw 0x4BC

#define BIT_DF_Play_Seq_0016_Kyte_Secures_Rope_Near_BWC 0x10A
#define BIT_DF_Played_Seq_0016_Kyte_Secures_Rope_Near_BWC 0x10B

#define BIT_DF_Play_Seq_000F_Kyte_Secures_Rope_Upper_Falls 0x28E
#define BIT_DF_Played_Seq_000F_Kyte_Secures_Rope_Upper_Falls 0x28F

#define BIT_DF_Play_Seq_002F_Kyte_Activates_Turbine 0x28D
#define BIT_DF_Played_Seq_002F_Kyte_Activates_Turbine BIT_DF_Cradle_Powered

#define BIT_DF_Cradle_Moving_Down 0x1C //Also set when hitting nearby Projectile Switches: intended to reverse cradle direction (but buggy)

#define BIT_DF_Toxic_Cave_Destroy_Gas_Vent 0x20
#define BIT_DF_Toxic_Cave_Destroyed_Gas_Vent 0x21
#define BIT_DF_Toxic_Cave_Destroy_Wall 0x37
#define BIT_DF_Toxic_Cave_Destroyed_Wall 0x38

#define BIT_DF_Play_Seq_003E_Mole_Cave_SharpClaw_Hides 0x31
#define BIT_DF_Played_Seq_003E_Mole_Cave_SharpClaw_Hides 0x32

#define BIT_DF_Mole_Cave_Destroy_Door 0x293
#define BIT_DF_Mole_Cave_Destroyed_Door 0x294

#define BIT_DF_Play_Seq_002C_Mole_Cave_Hatch_Opens 0x100
#define BIT_DF_Played_Seq_002C_Mole_Cave_Hatch_Opens 0x101

#define BIT_DF_Mole_Dug_Wall_1 BIT_CapyTunnel1
#define BIT_DF_Mole_Dug_Wall_2 BIT_CapyTunnel2
#define BIT_DF_Mole_Dug_Wall_3 BIT_CapyTunnel3

#define BIT_DF_Play_Seq_002E_Demolition_Cave_SharpClaw_Antics 0x3F
#define BIT_DF_Played_Seq_002E_Demolition_Cave_SharpClaw_Antics 0x5E

#define BIT_DF_Demolition_Cave_Destroy_Whirlpool_Wall_1 0x11E
#define BIT_DF_Demolition_Cave_Destroyed_Whirlpool_Wall_1 0x11F
#define BIT_DF_Demolition_Cave_Destroy_Whirlpool_Wall_2 0x1
#define BIT_DF_Demolition_Cave_Destroyed_Whirlpool_Wall_2 0x2
#define BIT_DF_Demolition_Cave_Destroy_Whirlpool_Wall_3 0x3
#define BIT_DF_Demolition_Cave_Destroyed_Whirlpool_Wall_3 0x4
#define BIT_DF_Demolition_Cave_Destroy_Whirlpool_Wall_4 0x2A0
#define BIT_DF_Demolition_Cave_Destroyed_Whirlpool_Wall_4 0x342
#define BIT_DF_Demolition_Cave_Destroy_Crate_Wall 0x11C
#define BIT_DF_Demolition_Cave_Destroyed_Crate_Wall 0x11D
#define BIT_DF_Demolition_Cave_Destroy_Plant_Wall_1 0x2a1
#define BIT_DF_Demolition_Cave_Destroyed_Plant_Wall_1 0x2a2
#define BIT_DF_Demolition_Cave_Destroy_Plant_Wall_2 0x2a3
#define BIT_DF_Demolition_Cave_Destroyed_Plant_Wall_2 0x2a4 
#define BIT_DF_Demolition_Cave_Destroy_SharpClaw_Wall 0x345
#define BIT_DF_Demolition_Cave_Destroyed_SharpClaw_Wall 0x346
#define BIT_DF_Demolition_Cave_Destroy_Far_Wall 0x343
#define BIT_DF_Demolition_Cave_Destroyed_Far_Wall 0344

// #define BIT_DF_Whirlpool_Cave_Wall_Demolished 0x105
#define BIT_DF_Whirlpool_Cave_Wall_Demolition_Finished 0x106

#define BIT_DF_Play_Seq_0031_Activate_Shrine_Switch_1 0x10E //Foodbag Cave
#define BIT_DF_Played_Seq_0031_Activate_Shrine_Switch_1 BIT_DF_Shrine_Door_Light_Activated_One //Foodbag Cave

#define BIT_DF_Play_Seq_0032_Activate_Shrine_Switch_2 0x10F //Toxic Cave
#define BIT_DF_Played_Seq_0032_Activate_Shrine_Switch_2 BIT_DF_Shrine_Door_Light_Activated_Two //Toxic Cave

#define BIT_DF_Play_Seq_0033_Activate_Shrine_Switch_3 0x110 //Mole Cave
#define BIT_DF_Played_Seq_0033_Activate_Shrine_Switch_3 BIT_DF_Shrine_Door_Light_Activated_Three //Mole Cave

#define BIT_DF_Play_Seq_0034_Activate_Shrine_Switch_4 0x1F7 //Whirlpool Cave
#define BIT_DF_Played_Seq_0034_Activate_Shrine_Switch_4 BIT_DF_Shrine_Door_Light_Activated_Four //Whirlpool Cave

#define BIT_DF_Seq_0035_Shrine_Door_Opens 0x4A1
#define BIT_DF_Seq_0035_Shrine_Door_Opened 0x4A2

#define BIT_DF_Shrine_Door_Opens 0x10C
#define BIT_DF_Shrine_Door_Opened 0x10D

#define BIT_DF_Defeated_Shrine_Entrance_SharpClaw 0x91F

#define BIT_DF_Play_Seq_003C_Krystal_Returns_From_The_Shrine 0x1A
#define BIT_DF_Played_Seq_003C_Krystal_Returns_From_The_Shrine 0x1B

#define BIT_DF_Play_Seq_0067_Kyte_Wants_To_Visit_CloudRunner_Fortress 0x29D
#define BIT_DF_Played_Seq_0067_Kyte_Wants_To_Visit_CloudRunner_Fortress 0x29F

#define BIT_DF_8DE 0x8DE
//END OF TEMPORARY DEFINES

enum DF_CustomObjectGroups {
    DF_ObjGroup_Rope_BWC_Detached = 20,         //The original DFropenode setup near BWC, but moved into an objGroup of its own (synced with `DF_ObjGroup2_Lower_Falls` via `DFlevelcontrol`)
    DF_ObjGroup_Rope_BWC_Attached,              //A duplicate of the rope setup near BWC, but already attached (used for restoring state without seq preempt messing up rope's spring dynamics)
    DF_ObjGroup_Rope_Upper_Falls_Detached,      //The original upper falls DFropenode setup, but moved into an objGroup of its own (synced with `DF_ObjGroup14_Middle_and_Upper_Falls` via `DFlevelcontrol`)
    DF_ObjGroup_Rope_Upper_Falls_Attached,      //A duplicate of the upper falls' rope setup, but already attached (used for restoring state without seq preempt messing up rope's spring dynamics)
    DF_ObjGroup_Turbine_Sequence_HitAnimators   //Enables widescreen fixes just while the turbine sequence is active
};
