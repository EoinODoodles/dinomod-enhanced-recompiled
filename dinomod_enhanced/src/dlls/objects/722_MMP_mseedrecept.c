#include "dlls/objects/214_animobj.h"
#include "modding.h"

#include "common.h"
#include "game/gamebits.h"

#include "recomp/dlls/objects/722_MMP_mseedrecept_recomp.h"
#include "sys/objtype.h"

typedef struct {
    u8 unk0;
    u8 unk1;
    CurveSetup* curveSetup;
    s16 gamebitPlanted; //after using MoonSeed
    s16 gamebitGrown; //after using Flame on planted seed (at night only, according to design docs)
    s16 unkC;
    s16 unkE;
    f32 unk10;
} MoonSeedReceptacle_Data;

typedef struct {
    ObjSetup base;
    s16 unk18;
    u16 kyteFlightGroup;
    s16 unk1C;
    s8 unk1E;
    u8 yaw;
} MoonSeedReceptacle_Setup;

extern int moonSeedReceptacle_anim_callback(Object* self, Object *animObj, AnimObj_Data* animObjData, s8 arg3);

/** 
    Changes the gameBitIDs used to track whether MoonSeeds have been planted, allowing game progress
    to continue despite MoonSeed item collection not having been implemented (originally by jeebs2kx)
*/
RECOMP_PATCH void moonSeedReceptacle_setup(Object* self, MoonSeedReceptacle_Setup* setup, s32 arg2) {
    MoonSeedReceptacle_Data* objData;

    objData = self->data;
    self->animCallback = moonSeedReceptacle_anim_callback;
    self->srt.yaw = setup->yaw << 8;
    objData->unk0 = 0;
    
    obj_add_object_type(self, 0x30);

    switch (setup->base.uID) {
        //Soil spot just beyond SharpClaw outpost 
        case 0x41A5B: 
            objData->gamebitPlanted = BIT_MMP_MoonSeed_1_Planted;
            objData->gamebitGrown = BIT_MMP_MoonSeed_1_Grown;
            break;
        //Soil spot near ravine, leading up to meteoroid impact site
        case 0x41A59: 
            objData->gamebitPlanted = BIT_MMP_MoonSeed_1_Planted; //@recomp: change flagID
            objData->gamebitGrown = BIT_MMP_MoonSeed_2_Grown;
            break;
        //Soil spot leading out of meteoroid impact site, towards Shrine
        case 0x41A5C: 
            objData->gamebitPlanted = BIT_MMP_MoonSeed_1_Planted; //@recomp: change flagID
            objData->gamebitGrown = BIT_MMP_MoonSeed_3_Grown;
            break;
        //Soil spot leading up to dark tunnel
        case 0x41A5D:
            objData->gamebitPlanted = BIT_MMP_MoonSeed_1_Planted; //@recomp: change flagID
            objData->gamebitGrown = BIT_MMP_MoonSeed_4_Grown;
            break;
    }

    objData->unk1 = 0;
}
