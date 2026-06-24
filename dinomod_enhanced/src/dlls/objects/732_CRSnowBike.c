#include "modding.h"

#include "dll.h"
#include "dlls/engine/4_race.h"
#include "dlls/engine/27.h"
#include "sys/gfx/modgfx.h"
#include "sys/rand.h"

#include "recomp/dlls/_asm/732_recomp.h"

typedef struct {
    s8 unk0[0xE];
    s8 xJoy;
    s8 yJoy;
    s8 unk10;
} DLL732_Unk_2E0; //Controller/joystick-related

typedef struct {
    Vec3f unk0;
    Vec3f unkC;
    f32 unk18;
    f32 unk1C;
    f32 unk20;
    f32 unk24;
    f32 unk28;
    f32 unk2C;
    f32 unk30;
} DLL732_Data2AC;

typedef struct {
    ObjSetup base;
    u8 yaw;
    u8 unk19;
    s16 gamebitUnlocked;
    u8 racetrackIdx;
    u8 unk1D;
    s16 gamebitA;
    u8 unk20;
} CRSnowBike_Setup;

typedef struct {
    SRT unk0;
    RaceStruct raceData;
    s8 _unk3C[0x48 - 0x3C];
    u8 racetrackIdx;
    u8 unk49;
    DLL27_Data collision;
    DLL732_Data2AC unk2AC;
    DLL732_Unk_2E0 unk2E0;
    DLL_IModgfx* unk2F4;
    DLL_IModgfx* unk2F8;
    s16* gamebitIDs;
    s8 _unk300[0x330 - 0x300];
    Vec3f unk330[6];
    s8 _unk378[0x384 - 0x378];
    f32 unk384;
    Vec3f unk388;
    f32 unk394;
    f32 unk398;
    f32 unk39C;
    Vec3f unk3A0;
    Vec3f unk3AC;
    u32 soundHandle1;
    u32 soundHandle2;
    u32 soundHandle3;
    u32 soundHandle4;
    s32 fuelAmount;
    Vec3f unk3CC;
    s8 _unk3D8[0x3DC - 0x3D8];
    s16 yawOffset;
    s16 rollOffset;
    s16 yaw;
    s16 pitch;
    s16 roll;
    s16 unk3E6;
    s8 _unk3E8[0x3EA - 0x3E8];
    s16 unk3EA;
    s8 unk3EC;
    u8 unk3ED;
    u8 unk3EE;
    u8 flags;
    s8 state;
    u8 unk3F1;
    s8 unk3F2;
    s8 unk3F3;
    u8 unk3F4_0 : 1;
} CRSnowBike_Data;

//TODO: remove after decomp update
#define CRSnowBike_func_0 dll_732_func_0

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
RECOMP_PATCH s32 CRSnowBike_func_0(Object* self, CRSnowBike_Data* data, f32 arg2) {
    RaceCheckpointSetup* checkpointSetup;
    s32 sp30;
    CRSnowBike_Data* objData;
    
    objData = self->data;

    checkpointSetup = gDLL_4_Race->vtbl->func8(data->raceData.unk10, &sp30);
    if (checkpointSetup->unk20[1] == -1) {
        //@recomp: store checkpoint's y position
        objData->unk0.transl.y = checkpointSetup->pos.y;

        objData->unk3F4_0 = rand_next(0, 1);
    }
    
    return gDLL_4_Race->vtbl->func5(&data->unk0, &data->raceData, arg2, 1, 0, objData->unk3F4_0);
}
