#pragma once

#include "PR/ultratypes.h"

// The same as the `HitsLine` / `ModLine` structs, but with `settingsA` / `settingsB` changed to unsigned variables 
// as a modding convenience (avoiding implicit conversion warnings when combining flags).
typedef struct{
/*00*/    s16 Ax;
/*02*/    s16 Bx;
/*04*/    s16 Ay;
/*06*/    s16 By;
/*08*/    s16 Az;
/*0a*/    s16 Bz;
union { //height can be treated as a single s16 value, using the uppermost bit of settingsA
    struct {
    /*0c*/    s8 heightA;
    /*0d*/    s8 heightB;
    };
    /*0c*/    s16 heightUnified;
};
/*0e*/    u8 settingsA;
/*0f*/    u8 settingsB;
/*10*/    s16 animatorID;
/*12*/    s16 pad;
} TrackLine; 

typedef enum {
    TrackLine_SETTINGA_Unified_Height = 0x80 //The line's height is read as a single s16 value, instead of separate s8 heights above pointA/pointB
} TrackLineSettingsA;

typedef enum {
    TrackLine_SETTINGB_Deactivated = 0x40,   //Line currently has no effect (can be toggled via HitAnimator)
    TrackLine_SETTINGB_Nonsolid = 0x80       //Player can pass through (when this flag isn't set, the line even affects the camera!)
} TrackLineSettingsB;

typedef enum {
    HITS_1 = 1,
    HITS_Precipice = 2,  //Cling to ledge when walking over line (can drop off)
    HITS_Wall_Climb = 3, //Other settings determine if player can drop off bottom/clamber over top
    HITS_Jump = 4,       //Always jump when passing line (no dangling off precipice) - smaller jump at low approach speeds
    HITS_Jump_or_Precipice = 5, //Jump/dangle off ledge, depending on approach speed
    HITS_Clamber_Up = 6, //Step up when approached from the ground, or ledge grab when approached mid-jump
    HITS_7 = 7,
    HITS_8 = 8,
    HITS_9 = 9,
    HITS_Ladder = 10,
    HITS_11 = 11,
    HITS_12 = 12,
    HITS_13 = 13,
    HITS_Crawl = 14, //Requires a crawlspace curve network too, or the player will warp to the world origin (NOTE: seems to stop working with `TrackLine_SETTINGB_Nonsolid` applied)
    HITS_Hop = 15,   //A little skip/hop up (used rarely, like around Walled City's Shrine transporter) (NOTE: seems to stop working with `TrackLine_SETTINGB_Nonsolid` applied)
    HITS_Swim_Clamber_Ashore = 16,
    HITS_Precipice_No_Letting_Go = 17 //Like HITS_Precipice, but you can't drop off
} PlayerHitsTypes; //Player HITS actions (lower 6 bits of TrackLine settingsB)

typedef enum {
    Player_Ignores_Line = 1,       //Ignored by player, even if it's a solid line?
    Wall_Climb_No_Letting_Go = 2,  //Can't let go at the top or bottom of a climbable wall (need to keep moving laterally to another section of wall)
    Wall_Climb_Stop_at_Bottom = 4, //Can't let go at the bottom of a climbable wall (NOTE: overrides `Wall_Climb_Stop_at_Top`)
    Wall_Climb_Stop_at_Top = 8,    //Can't clamber over the top of a climbable wall
} PlayerHitsConfigs; //Player HITS action modifiers (Lower bits of TrackLine settingsA)

#define HITS_A(x, y, z) .Ax = x, .Ay = y, .Az = z
#define HITS_B(x, y, z) .Bx = x, .By = y, .Bz = z
