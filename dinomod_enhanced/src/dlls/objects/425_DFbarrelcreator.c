#include "modding.h"
#include "recomputils.h"
#include "common_objsetups.h"

#include "common.h"
#include "sys/main.h"
#include "sys/objtype.h"

#include "recomp/dlls/objects/425_DFbarrelcreator_recomp.h"

typedef struct {
    ObjSetup base;
    s16 _unused18;
    u8 yaw;
    s16 _unused1C;
    s8 wasRespawned;  //@recomp: track barrels recreated by DFbarrelcreator
    s8 hitPoints;     //@recomp: repurposed as health
} DFBarrel_Setup; //0x20

typedef struct {
    ObjSetup base;
    u8 searchDistance;  //Creates a barrel if none are found inside this radius (stored divided by 4)
    u8 yaw : 4;          //@recomp: Yaw for the barrels created
    u8 barrelHealth : 2; //@recomp: Optional health value for the barrel that's created
    u8 delay : 2;        //@recomp: Optional waiting time between barrels
    s16 gamebitStop;    //Stops creating barrels if this gamebit is set
} DFBarrelCreator_Setup;

typedef struct {
    u16 timer;
    u8 notFirstBarrel;
    Object* barrel;
} DFBarrelCreator_Data; //@recomp

RECOMP_PATCH void DFbarrelcreator_setup(Object *self, DFBarrelCreator_Setup *setup, s32 reset) { 
    //@recomp: make sure search distance isn't too small
    if (setup->searchDistance < 50) {
        setup->searchDistance = 50;
    }

    //@recomp: store yaw for barrels
    self->srt.yaw = setup->yaw << 12;
}

RECOMP_PATCH void DFbarrelcreator_control(Object* self) {
    DFBarrelCreator_Setup* objSetup;
    DFBarrel_Setup* barrel;
    f32 distance;
    /* RECOMP */
    DFBarrelCreator_Data* objData;

    objSetup = (DFBarrelCreator_Setup*)self->setup;
    objData = self->data;
    
    //@recomp: track whether the created barrel's been destroyed yet
    if (objData->barrel && (objData->barrel->stateFlags & OBJSTATE_DESTROYED)) {
        objData->barrel = NULL;
    }
    if (objData->barrel) {
        return;
    }

    //Don't create a barrel if the specified gamebit is set
    if ((objSetup->gamebitStop != NO_GAMEBIT) && main_get_bits(objSetup->gamebitStop)) {
        return;
    }

    //@recomp: don't create a barrel yet if this DFbarrelcreator specifies a delay between barrels
    if ((objData->timer > 0)) {
        if (objData->timer > gUpdateRate) {
            objData->timer -= gUpdateRate;
        } else {
            objData->timer = 0;
        }
        return;
    }

    //Don't create a barrel if there's already one nearby
    distance = objSetup->searchDistance * 4;
    if (obj_get_nearest_type_to(OBJTYPE_Barrel, self, &distance)) {
        return;
    }

    //Create a barrel
    barrel = (DFBarrel_Setup*)obj_alloc_setup(sizeof(DFBarrel_Setup), OBJ_DFbarrel);
    barrel->base.loadDistance = 100;
    barrel->base.fadeDistance = 80;
    barrel->base.loadFlags = 4;
    barrel->base.fadeFlags = 4;
    barrel->base.x = self->srt.transl.x;
    barrel->base.y = self->srt.transl.y - 30.0f;
    barrel->base.z = self->srt.transl.z;
    barrel->yaw = self->srt.yaw >> 8;               //@recomp: use BarrelCreator's yaw
    barrel->hitPoints = objSetup->barrelHealth;     //@recomp: optional hitPoints for barrel
    barrel->wasRespawned = objData->notFirstBarrel; //@recomp: track that the barrel was recreated by DFbarrelcreator

    //@recomp: store a reference to the barrel
    objData->barrel = obj_create((ObjSetup*)barrel, 5, self->mapID, -1, NULL);

    objData->timer = objSetup->delay * 30; //@recomp: optionally wait between barrels   
    objData->notFirstBarrel = TRUE;
}

RECOMP_PATCH u32 DFbarrelcreator_get_data_size(Object *self, u32 offsetAddr) {
    return sizeof(DFBarrelCreator_Data);
}
