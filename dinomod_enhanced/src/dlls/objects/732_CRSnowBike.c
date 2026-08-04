#include "modding.h"

#include "dll.h"
#include "dlls/engine/4_race.h"
#include "dlls/engine/27.h"
#include "sys/gfx/modgfx.h"
#include "sys/rand.h"

#include "recomp/dlls/objects/732_CRSnowBike_recomp.h"

typedef struct {
    s8 unk0[0xE];
    s8 xJoy;            //Joystick X (simulated for CPU racers)
    s8 yJoy;            //Joystick Y (simulated for CPU racers)
    s8 unk10;
} CRSnowBike_SteerData;

typedef struct {
    Vec3f vGravity; //Gravity acceleration vector (relative to bike's own coordinate space)
    Vec3f velocity; //Bike's objectSpace velocity (Z points out the back of the bike, so forward velocity unit vector is {0,0,-1}) 
    f32 accelerationFactorPerFrame; // Per sixtieth of a second
    f32 gravityFactor;              // Scaling factor for gravity
    f32 accelerationFactor;         // Per second
    f32 unk24;                      // Unused
    f32 gravity;                    // Base unit for gravity
    f32 friction;                   // Applied while in contact with the ground
    f32 airResistance;              // Applied in proportion with square of speed
} CRSnowBike_MotionData;

typedef enum {
    STATE_0_Parked,
    STATE_1,
    STATE_2_Driving
} CRSnowBike_States;

typedef enum {
    CRSnowBike_FLAG_0_None = 0,
    CRSnowBike_FLAG_1_Finished = 1,
    CRSnowBike_FLAG_2_Driving_In_Void = 2,
    CRSnowBike_FLAG_4_Grounded = 4,
    CRSnowBike_FLAG_8_Race_Started = 8,
    CRSnowBike_FLAG_10_Was_In_Sequence = 0x10,
    CRSnowBike_FLAG_20_SharpClaw_Driver = 0x20
} CRSnowBike_Flags;

typedef enum {
    CRSnowBike_SOUNDFLAG_None = 0,
    CRSnowBike_SOUNDFLAG_Engine = 1,
    CRSnowBike_SOUNDFLAG_Hiss = 2,
    CRSnowBike_SOUNDFLAG_Jets = 4
} CRSnowBike_SoundFlags;

typedef struct {
    s16 started;
    s16 ended;
    s16 unk;
} CRSnowBike_Gamebits;

typedef enum {
    RACETRACK_0_CloudRunner_Fortress,
    RACETRACK_1_Golden_Plains
} CRSnowBike_Racetracks;

#define PLAYER_NOT_NEARBY 0
#define PLAYER_NOT_ALLOWED_DISMOUNT 0
typedef enum {
    SIDE_LEFT = 1,
    SIDE_RIGHT = 2
} CRSnowBike_Sides;

typedef enum {
    CRSnowBike_ANIMCMD_2_Lose_Race = 2,
    CRSnowBike_ANIMCMD_3_Free_Fuel_Gauge = 3
} CRSnowBike_AnimCommands;

typedef struct {
    ObjSetup base;
    u8 yaw;
    u8 isSharpClawBike;             // Whether the bike can only be driven by SharpClaw
    s16 gamebitUnlocked;            // The bike can only be mounted when this gamebit is set
    u8 racetrackIdx;                // Which race the bike is used in (CloudRunner Fortress vs. Golden Plains)
    u8 unk1D;
    s16 gamebitFinished;            // Bike disappears and stops updating when this gamebit is set
    u8 yJoySharpClaw;               // Strength factor for SharpClaws' simulated yJoy steering value
} CRSnowBike_Setup;

typedef struct {
    SRT srtCurves;                   //Stores CPU racers' current checkpoint/curve-interpolated position
    RaceStruct raceData;             //Race/checkpoints-related data
    s8 _unk3C[0x48 - 0x3C];
    u8 racetrackIdx;                 //See `CRSnowBike_Racetracks`
    u8 unk49;
    DLL27_Data collision;            //Terrain collider
    CRSnowBike_MotionData motion;    //Bike motion: objectSpace velocity (-Z forward), gravity, resistance, etc.
    CRSnowBike_SteerData steering;   //Driver controls
    DLL_IModgfx* modGfxDLLFlames;    //Effects DLL for bike's thruster flames
    DLL_IModgfx* modGfxDLLWaves;     //Effects DLL for bike's wave effects when turning sharply
    CRSnowBike_Gamebits* gamebitIDs; //Gamebits used when starting a race/etc.
    s8 _unk300[0x330 - 0x300];
    Vec3f wsCollisionCoords[5];      //worldSpace coords of dCollisionPoints' collisions
    s8 _unk36C[0x384 - 0x36C];
    f32 stallFactor;                 //Affects how quickly the bike loses speed after a damaging impact
    Vec3f attachPointCoords;
    f32 soundFactorRumble;
    f32 soundFactorJets;
    f32 forwardSpeed;
    Vec3f prevTranslate;        //The bike's worldSpace coordinates at the start of the tick
    Vec3f wsFrontOfBike;        //The front end of the bike's worldSpace coordinates
    u32 soundHandleEngine;
    u32 soundHandleJets;
    u32 soundHandleHiss;
    u32 soundHandleRumble;
    s32 fuelAmount;         //Fuel gauge level
    Vec3f maxVelocity;      //Velocity component limits, in bike's objectSpace 
    s32 _unk3D8;
    s16 yawOffset;
    s16 rollOffset;
    s16 yaw;
    s16 pitch;
    s16 roll;
    s16 pitchOffset;        //Tilt forward/back while airborne, based on joyY
    s16 _unk3E8;
    s16 fxTimer;
    s8 stallFrames;         //Slows the bike down for this many frames, after it's damaged
    u8 mountingFrom;        //Which side the player's approaching from when the bike's parked
    u8 numCollisionPoints;
    u8 flags;
    s8 state;
    u8 raceRanking;         //Race placement/ordinal ranking
    s8 framesInAir;         //How long the bike's been airborne, in frames
    s8 collisionFlags;      //Bitfield tracking collisions on different points on the bike
    u8 branchFlagCPU : 1;   //Randomised Boolean used for SharpClaw racers' checkpoint pathing
} CRSnowBike_Data;

//TODO: remove after decomp update
// #define CRSnowBike_func_0 dll_732_func_0

/**
  * Stop racing SharpClaws from getting stuck in walls in CRF (originally by MusicalProgrammer).
  * 
  * Previously the SharpClaw would drive at a fixed height when passing through a section of the racetrack which had its BLOCKS unloaded. 
  * Because two sections of the CRF track overlap (when viewed from above), this fixed height value could cause SharpClaw to be at the wrong 
  * elevation when the player's movements cause BLOCKS models to load in around the SharpClaw, snapping them onto the wrong part of the mesh.
  * They would continue trying to get to the next curve node, driving directly into the wall!
  *
  * This is fixed by storing the previous race checkpoint's Y value, for the SharpClaw to use while driving through an unloaded section of track.
  */
RECOMP_PATCH s32 CRSnowBike_sharpclaw_update_race_pathing(Object* self, CRSnowBike_Data* data, f32 arg2) {
    RaceCheckpointSetup* checkpointSetup;
    s32 sp30;
    CRSnowBike_Data* objData;
    
    objData = self->data;

    checkpointSetup = gDLL_4_Race->vtbl->func8(data->raceData.unk10, &sp30);
    if (checkpointSetup->unk20[1] == -1) {
        //@recomp: store checkpoint's y position
        objData->srtCurves.transl.y = checkpointSetup->pos.y;

        objData->branchFlagCPU = mathRnd(0, 1);
    }
    
    return gDLL_4_Race->vtbl->func5(&data->srtCurves, &data->raceData, arg2, 1, 0, objData->branchFlagCPU);
}
