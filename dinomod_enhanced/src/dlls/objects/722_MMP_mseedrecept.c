#include "dlls/objects/214_animobj.h"
#include "modding.h"

#include "common.h"
#include "game/gamebits.h"

#include "recomp/dlls/objects/722_MMP_mseedrecept_recomp.h"
#include "sys/objtype.h"

typedef struct {
/*00*/	s16 objId;
/*02*/	u8 quarterSize;
/*03*/	u8 act;
/*04*/	u8 nextCurveGroup;
/*05*/	u8 prevCurveGroup;
/*06*/	u8 branch1CurveGroup;
/*07*/	u8 branch2CurveGroup;
/*08*/	f32 x;
/*0c*/	f32 y;
/*10*/	f32 z;
/*14*/	s32 uID;
} CurveSetup; //curves use the base objSetup fields differently, so creating a unique base struct

typedef struct {
/*00*/ CurveSetup base;
/*18*/ s8 curveType; //2) KTrex, 3) RedEye, 1A) camera?, 1B) camera?, 1D) ThornTail, 1F) crawlSpace, 22) Kyte, 24) Tricky
/*1C*/ s32 unk1C;
/*20*/ s32 nextCurveUID;
/*24*/ s32 prevCurveUID;
/*28*/ s32 branch1UID;
/*2C*/ s32 branch2UID;
/*30*/ s8 yaw;
/*31*/ s8 pitch;
/*32*/ s16 usedBit; //gameBit
/*34*/ s16 unk34; //gameBit?
/*36*/ s16 unk36;
/*38*/ s16 unk38; //gameBit?
} KyteCurve_Setup;

typedef struct {
    u8 unk0;
    u8 unk1;
    KyteCurve_Setup* curveSetup;
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

extern s32 moonSeedReceptacle_anim_callback(Object* self, s32 animObj, AnimObj_Data* animObjData, s32 arg3);

/** 
    Changes the gameBitIDs used to track whether MoonSeeds have been planted, allowing game progress
    to continue despite MoonSeed item collection not having been implemented (originally by jeebs2kx)
*/
RECOMP_PATCH void moonSeedReceptacle_setup(Object* self, MoonSeedReceptacle_Setup* setup, s32 arg2) {
    MoonSeedReceptacle_Data* objData;

    objData = self->data;
    self->unk0xbc = (void*)&moonSeedReceptacle_anim_callback;
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
