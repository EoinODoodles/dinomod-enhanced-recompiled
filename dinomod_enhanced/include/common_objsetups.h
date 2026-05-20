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
/*18*/ s16 gamebitPlay;        //The sequence will play when this gamebit is set
/*1A*/ s16 gamebitFinished;    //This gamebit will be set when the sequence has played
/*1C*/ u8 rotate;
/*1D*/ u8 playbackOptions;
/*1E*/ s8 seqIndex;            //The index of the sequence in the Object.bin entry's sequence list
/*1F*/ s8 modelInstIdx;        //Choose between 3D models, visible when debugging (usually a clapperboard)
/*20*/ s16 unk20;
/*22*/ u16 unk22;
/*24*/ u8 warpID;              //Optionally warp the player
} SeqObj_Setup;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 rotation;
/*19*/ u8 _unk19[0x20 - 0x19];
/*20*/ s16 gamebitEnabled;
} DR_EarthCallPad_Setup;
