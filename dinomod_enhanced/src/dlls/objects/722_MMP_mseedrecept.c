#include "modding.h"

#include "common.h"
#include "game/gamebits.h"
#include "sys/gfx/animseq.h"
#include "sys/objtype.h"

#include "recomp/dlls/objects/722_MMP_mseedrecept_recomp.h"

typedef struct {
    ObjSetup base;
    s16 unk18;
    u16 kyteFlightGroup;
    s16 unk1C;
    s8 unk1E;
    u8 yaw;
} MoonSeedReceptacle_Setup;

typedef struct {
    u8 state;
    u8 flags;
    CurveSetup* curveSetup;
    s16 gamebitPlanted; //After using MoonSeed
    s16 gamebitGrown;   //After using Flame on planted seed (only at night according to design docs, but at any time of day in this implementation)
    s16 glowPhase;      //Angle used for sinusoidal glowing at night
    f32 rattleTimer;    //Used for shaking sounds/effects when planted and not yet grown
} MoonSeedReceptacle_Data;

typedef enum {
    MoonSeedReceptacle_STATE_0_Init,
    MoonSeedReceptacle_STATE_1_Bare_Soil,       //Waiting for the player to plant a MoonSeed
    MoonSeedReceptacle_STATE_2_Seed_Planted,    //Waiting for Flame command
    MoonSeedReceptacle_STATE_3_Seed_Flamed,     //Kyte is using Flame on the Seed
    MoonSeedReceptacle_STATE_4_Grown            //Finished
} MoonSeedReceptacle_States;

typedef enum {
    MoonSeedReceptacle_FLAG_1_Sequence_Played = 1,
    MoonSeedReceptacle_FLAG_2_Glowing = 2,
    MoonSeedReceptacle_FLAG_4_Rattling = 4
} MoonSeedReceptacle_Flags;

typedef enum {
    MMP_MoonSeedReceptacle_ID_1 = 0x41A5B, //Between the SharpClaw outpost and CloudRunner Fortress, leading further into MMP.
    MMP_MoonSeedReceptacle_ID_2 = 0x41A59, //Near the ravine, leading up to meteoroid impact site.
    MMP_MoonSeedReceptacle_ID_3 = 0x41A5C, //Near the exit of the meteoroid impact site, leading towards the Krazoa Shrine.
    MMP_MoonSeedReceptacle_ID_4 = 0x41A5D  //Along the path with the first Lunaimar encounters, leading up to the dark tunnel that winds back towards the SharpClaw outpost. 
} MoonSeedReceptacle_uIDs;

extern int MoonSeedReceptacle_anim_callback(Object* self, Object *animObj, AnimObj_Data* animObjData, s8 arg3);

/** 
    Changes the gamebitIDs used to track whether MoonSeeds have been planted, allowing game progress
    to continue despite MoonSeed item collection not having been implemented (originally by jeebs2kx)
*/
void MoonSeedReceptacle_setup(Object* self, MoonSeedReceptacle_Setup* setup, s32 reset) {
    MoonSeedReceptacle_Data* objData;

    objData = self->data;
    self->animCallback = MoonSeedReceptacle_anim_callback;
    self->srt.yaw = setup->yaw << 8;
    objData->state = MoonSeedReceptacle_STATE_0_Init;
    
    obj_add_object_type(self, OBJTYPE_KyteTarget);

    switch (setup->base.uID) {
        //Soil spot just beyond SharpClaw outpost 
        case MMP_MoonSeedReceptacle_ID_1: 
            objData->gamebitPlanted = BIT_MMP_MoonSeed_1_Planted;
            objData->gamebitGrown = BIT_MMP_MoonSeed_1_Grown;
            break;
        //Soil spot near ravine, leading up to meteoroid impact site
        case MMP_MoonSeedReceptacle_ID_2: 
            objData->gamebitPlanted = BIT_MMP_MoonSeed_1_Planted; //@recomp: change gamebitID
            objData->gamebitGrown = BIT_MMP_MoonSeed_2_Grown;
            break;
        //Soil spot leading out of meteoroid impact site, towards Shrine
        case MMP_MoonSeedReceptacle_ID_3: 
            objData->gamebitPlanted = BIT_MMP_MoonSeed_1_Planted; //@recomp: change gamebitID
            objData->gamebitGrown = BIT_MMP_MoonSeed_3_Grown;
            break;
        //Soil spot leading up to dark tunnel
        case MMP_MoonSeedReceptacle_ID_4:
            objData->gamebitPlanted = BIT_MMP_MoonSeed_1_Planted; //@recomp: change gamebitID
            objData->gamebitGrown = BIT_MMP_MoonSeed_4_Grown;
            break;
        default:
            STUBBED_PRINTF("Error romdef ident no known!\n");
    }

    objData->flags = 0;
}
