#include "modding.h"

#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"

#include "recomp/dlls/objects/389_BlastedObject_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 yaw8;
/*1A*/ s16 extraDamageStages; // aka health - 1. if zero, no model is drawn and it's assumed the block shapes are the visible surface
/*1C*/ s16 shapeAnimatorID; // animator ID of block shapes to hide/remove collision of when blasted
/*1E*/ s16 blastedGamebit; // bit to set when blasted
/*20*/ s16 hitCountGamebit; // for persisting the hit count
} BlastedObject_Setup;

typedef struct {
    s32 blockShapesUpdated;
    s8 unk4;
    u8 hitCount;
} BlastedObject_Data;

extern Object* sHitByList[];

extern s32 BlastedObject_hideBlockShapes(Object *self, s32 animatorID);

RECOMP_PATCH void BlastedObject_obj_Setup(Object* self, BlastedObject_Setup* setup, s32 reset) {
    BlastedObject_Data* objdata = self->data;
    
    objdata->blockShapesUpdated = FALSE;
    objSetPriority(self, OBJPRIORITY_BLASTEDOBJ);
    self->objhitInfo->unk58 |= 1;
    objdata->unk4 = (s8) setup->extraDamageStages;
    if (setup->hitCountGamebit != -1) {
        if ((objdata->hitCount = mainGetBits(setup->hitCountGamebit))) {
            objSetModel(self, objdata->hitCount);
        }
    }
    // @bug: Always re-enables hit count 0 bit
    // @recomp: Restore the correct bits, also don't modify these if we don't have damage stages or else
    //          it will conflict with other nearby blasted objects (in particular in CRF).
    if (setup->extraDamageStages > 0) {
        for (s32 i = 0; i <= setup->extraDamageStages; i++) {
            mainSetBits(BIT_2DE + i, i == objdata->hitCount);
        }
    }
    self->srt.yaw = setup->yaw8 << 8;
    if (mainGetBits(setup->blastedGamebit) != 0) {
        objdata->blockShapesUpdated = BlastedObject_hideBlockShapes(self, setup->shapeAnimatorID);
    }
    // @recomp: Clear sHitByList (this is not a perfect fix)
    for (s32 i = 0; i < 4; i++) {
        sHitByList[i] = NULL;
    }
}
