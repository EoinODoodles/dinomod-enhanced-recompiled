#pragma once

#include "game/objects/object.h"

typedef struct {
    ObjSetup base;
    s16 gamebitActivate; //gamebitID to check before animating
    u8 enableFlag1;      //unknown purpose
    u8 blocksAnimatorID; //when animating Blocks model Shapes
    u8 mode;             //decides which behaviour to use: removing/adding HITS lines, removing/adding Blocks Shapes, fading in/out Blocks Shapes
    u8 hitsAnimatorID;   //when animating HITS lines
} HitAnimator_Setup;

typedef enum {
    HitAnimator_Mode_Invert = 1,  //Remove the target when gamebit set (instead of enabling it when gamebit set)
    HitAnimator_Mode_No_Fade = 2, //When targetting BLOCKS Shapes: show/hide immediately
    HitAnimator_Mode_BLOCKS = 4,  //Targetting BLOCKS Shapes
    HitAnimator_Mode_HITS = 8     //Targetting HITS Lines
} HitAnimator_Modes;

typedef struct {
    ObjSetup base;
    s16 gamebitID;          //gamebit controlling visibility (usually multi-bit, used as a bitfield)
    u8 animatorID1;         //the animatorID tag of the Block Shapes that will be shown/hidden
    u8 initialVisibility;   //Boolean
    u8 bitIndex;            //the index to check on the gamebit bitfield
    u8 animatorID2;         //up to 3 different animatorIDs can be specified
    u8 animatorID3;         //up to 3 different animatorIDs can be specified
} VisAnimator_Setup;

typedef struct {
    ObjSetup base;
    s8 loopStartFrame;   //Frame to loop back to after reaching endFrame (only when looping)
    s8 materialIndex;    //Index within object's local Blocks model's materials (the frame of the material's texture animation will be manipulated)
    s16 endFrame;        //End frame of texture animation
    s16 speed;           //Playback rate when advancing through the texture animation
    s16 gamebitFinished; //GamebitID to set when end of animation reached (also prevents looping)
    s16 gamebitPlay;     //GamebitID to query, starting animation
} TexFrameAnimator_Setup;

typedef struct {
    ObjSetup base;
    s16 textureIndex;       //in TABLES.bin subfile #14 ("scroll table")
    s16 gamebitActivate;    //gamebitID that activates animation (or -1 to skip bit check)
    s16 unused1C;
    s8 speedU;              //scroll speed (horizontal)
    s8 speedV;              //scroll speed (vertical)
} TexScroll_Setup;

typedef struct {
    ObjSetup base;
    s16 textureIndex;       //Primary material: index in TABLES.bin subfile #14 ("scroll table")
    s16 gamebitActivate;    //Unused, but instances of this object store a gamebitID here (e.g. 0x95)
    s8 uSpeedB;             //U scroll speed for secondary blend material 
    s8 vSpeedB;             //V scroll speed for secondary blend material 
    s8 uSpeedA;             //U scroll speed for primary material              
    s8 vSpeedA;             //V scroll speed for primary material              
    s16 blendTextureIndex;  //Secondary blend material: index in TABLES.bin subfile #14 (optional, -1 if unused)
                            //Blend material is used for multitextured scrolling water, etc.
} TexScroll2_Setup;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 flagPlay;
/*1A*/ s16 soundID; // Should use a SoundID enum value from dll 6
/*1C*/ u8 mode;
/*1D*/ u8 radius;
} SfxPlayer_Setup;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 unk18;
/*19*/ s8 unk19;
/*1A*/ s16 unk1A;
/*1C*/ s16 unk1C;
/*1E*/ s16 toggleGamebit;
/*20*/ s16 disableGamebit;
/*22*/ s8 unk22;
/*23*/ s8 unk23;
/*24*/ s8 unk24;
/*25*/ s8 unk25;
/*26*/ s8 unk26;
/*27*/ s8 unk27;
/*28*/ u8 unk28;
/*29*/ u8 unk29;
/*2A*/ s16 unk2A;
} FXEmit_Setup;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 gamebitPlayed;
/*1A*/ s16 gamebitPlay;  
/*1C*/ u8 yaw;
/*1D*/ u8 playbackOptions;
/*1E*/ s8 seqIndex;            //The index of the sequence in the Objects.bin entry's sequence list
/*1F*/ s8 modelInstIdx;        //Choose between 3D models, visible when debugging (usually a clapperboard)
/*20*/ s16 unk20;
/*22*/ u16 unk22;
/*24*/ u8 warpID;              //Optionally warp the player
} SeqObj_Setup;

#define TRIG_BITS_MODE(mode) (mode << 14)
#define TRIG_PARAMS_COMBINED(params) .param1 = (u8)((params) >> 8), .param2 = ((u8)params)

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 rotation;
/*19*/ u8 _unk19[0x20 - 0x19];
/*20*/ s16 gamebitEnabled;
} DR_EarthCallPad_Setup;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 unk18; // yaw?
/*19*/ u8 unk19; // pitch?
/*1A*/ u8 unk1A; // width? (x)
/*1B*/ u8 unk1B; // height? (y) divided by 2
/*1C*/ u8 unk1C; // length? (z)
/*1D*/ u8 effect;
/*1E*/ u8 _unk1E;
/*1F*/ u8 gamebitDisableValue; // disabled if the target gamebit is this value
/*20*/ s16 gamebit; // -1 if this effect box is always enabled
/*22*/ u8 target; // 0 = player, 1 = sidekick, 2 = objtype 6
} EffectBox_Setup;

typedef struct {
    ObjSetup base;
    s16 gamebitUnlocked;    //GamebitID determining whether the sidekick is unlocked (i.e. Kyte rescued)
    u8 sidekickIndex;       //Whether this should load Tricky/Kyte (see `SideLoad_Indices`)
} SideLoad_Setup;

typedef struct {
    ObjSetup base;
    s16 gamebitDug;         //Gamebit to set when Tricky digs up the spot
    s16 unused1A;      
    s16 unused1C;
    s16 magicCaveID;        //Values 3 onwards cause the dig spot to become be a Magic Cave entrance
    u8 digDepthMax;         //Dig distance (stored at 100x scale)
    u8 soundIndex;          //Index of the sound to play when digging has finished (0: small secret, 1: big secret)
    u8 unused22;
    u8 findCommandRadius;   //Range for Find command to show up in inventory
    u8 unused24;            
    u8 animatorID;          //Block shapes with this tag will be animated
    u8 falloffRadius;       //Vertex influence tapers off with this radius, and it also decides how far Tricky scoots backwards while digging
    u8 collectableDepth;    //How far down the collectable is buried beneath the dig spot
} GroundAnimator_Setup;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 gamebit;     //Spray stops when this gamebit is set (when specified)
/*1A*/ s8 roll;
/*1B*/ s8 pitch;
/*1C*/ s8 yaw;
/*1D*/ u8 amplitudeX;
/*1E*/ u8 amplitudeZ;
/*1F*/ u8 amplitudeY;
/*20*/ u8 distance;
/*21*/ u8 unk21;
/*22*/ u8 unk22;
/*23*/ u8 flags;
/*24*/ u8 iterations;
/*RECOMP*/
// Note: This addition is OK because this setup has a size of 0x28 in the map files.
/*25*/ u8 invertGamebit; //When set: spray plays when gamebit set (if a gamebit was specified)
} WaterFallSpray_Setup;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 yaw;
/*19*/ s8 spawnLogDisabled;
} DFdockpoint_Setup;
