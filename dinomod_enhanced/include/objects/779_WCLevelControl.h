#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

//TEMPORARY DEFINES
enum WC_ObjectGroups {
    WC_ObjGroup0_Sun_Beacon_Tunnel,
    WC_ObjGroup1_Moon_Beacon_Tunnel,
    WC_ObjGroup2_Sun_Pushblock_Puzzle,
    WC_ObjGroup3_Moon_Pushblock_Puzzle,
    WC_ObjGroup4_Boss_Lobby,
    WC_ObjGroup5_Central_Temple,
    WC_ObjGroup6_Sun_Temple_Exterior,
    WC_ObjGroup7_Moon_Temple_Exterior,
    WC_ObjGroup8_Sun_Temple_Interior,
    WC_ObjGroup9_Moon_Temple_Interior
};

#define BIT_WC_Is_Daytime 0x7F1
#define BIT_WC_Is_Nighttime 0x7F3

#define BIT_WC_Sun_Pressure_Switch_Active 0x7ED
#define BIT_WC_Sun_Beacon_Raised 0x7EF
#define BIT_WC_Sun_Beacon_Lit 0x7F9
#define BIT_WC_SlabDoor_Sun_Symbol_Lit 0x7F7

#define BIT_WC_Moon_Pressure_Switch_Active 0x7EE
#define BIT_WC_Moon_Beacon_Raised 0x7F0
#define BIT_WC_Moon_Beacon_Lit 0x7FA
#define BIT_WC_SlabDoor_Moon_Symbol_Lit 0x802

#define BIT_WC_SlabDoor_Opened 0x813
#define BIT_WC_Boss_Door_Opened 0x819
#define BIT_WC_King_EarthWalker_Cage_Opened 0x7F5
#define BIT_WC_King_EarthWalker_Rescued 0x1DF

#define BIT_WC_Moon_Passageway_Door_Opens 0x814
#define BIT_WC_Sun_Passageway_Door_Opens 0x815

#define BIT_WC_Sun_Pushblock_Puzzle_Reset 0x808
#define BIT_WC_Moon_Pushblock_Puzzle_Reset 0x809
#define BIT_WC_Sun_Pushblock_Puzzle_Progress 0x810
#define BIT_WC_Moon_Pushblock_Puzzle_Progress 0x811
#define BIT_WC_Sun_Aperture_Opened 0x812
#define BIT_WC_Moon_Aperture_Opened 0x813

#define BIT_WC_Sun_Temple_Opened 0x828
#define BIT_WC_Moon_Temple_Opened 0x829

#define BIT_WC_Moon_Temple_Illusory_Wall_Switch_Pressed 0x265
#define BIT_WC_Moon_Temple_Hazards_Deactivated 0x338

#define BIT_WC_Sun_Temple_Illusory_Wall_Switch_Pressed 0x205
#define SOUND_9FB_Illusory_Wall_Revealed 0x9FB

#define BIT_WC_Sun_Temple_Maze_Timed_Challenge_Switch_Pressed 0x2B1
#define BIT_WC_Sun_Temple_Maze_Timed_Challenge_Door_Opened 0x274
#define BIT_WC_Played_Seq_179_Sun_Temple_Maze_Timed_Challenge_Intro 0x204
#define BIT_WC_Sun_Temple_Maze_Illusory_Wall_1_Shown 0x226
#define BIT_WC_Sun_Temple_Maze_Illusory_Wall_2_Hidden 0x2A6
#define BIT_WC_Sun_Temple_Maze_Illusory_Wall_3_Hidden 0x206
#define BIT_WC_Sun_Temple_Maze_Illusory_Wall_4_Hidden 0x25F
#define BIT_WC_Sun_Temple_Maze_Solved 0x2A5

#define BIT_WC_Sun_Temple_Magic_Bridge_Visible 0x2D4
#define BIT_WC_Moon_Temple_Magic_Bridge_Visible 0x2D5

#define BIT_WC_Transporter_Chamber_Rises 0x335
#define BIT_WC_Transporter_Chamber_Opened 0x235
//END OF TEMPORARY DEFINES

enum WC_CustomObjectGroups {
    WC_OBJGROUP_Approach_Cave_Entrance = 15,
    WC_OBJGROUP_Approach_Cave_Exit,
    WC_OBJGROUP_Jungle_Door_Area,
    WC_OBJGROUP_Sun_Passageway_Door,
    WC_OBJGROUP_Moon_Passageway_Door,
    WC_OBJGROUP_Sun_Passageway,
    WC_OBJGROUP_Moon_Passageway,
    WC_OBJGROUP_Outskirts
};
