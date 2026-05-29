#include "SHbarrelcreator.h"
#include "custom_object_ids.h"
#include "custom_objsetups.h"

#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"

void SHbarrelcreator_ctor(void *dll) { }
void SHbarrelcreator_dtor(void *dll) { }

static void SHbarrelcreator_setup(Object* self, SHBarrelcreator_Setup* setup, s32 reset) {
    self->srt.yaw = setup->yaw << 8;
    
    self->stateFlags |= (OBJSTATE_UPDATE_DISABLED | OBJSTATE_PRINT_DISABLED);
}

static void SHbarrelcreator_control(Object* self) {
    SHBarrelcreator_Setup* objSetup = (SHBarrelcreator_Setup*)self->setup;
    SHBarrel_Setup* barrel;
    f32 distance;

    //Don't create a barrel if the specified gamebit is set
    if ((objSetup->gamebitStop != NO_GAMEBIT) && main_get_bits(objSetup->gamebitStop)) {
        return;
    }

    //Don't create a barrel if there's already one nearby
    distance = objSetup->searchDistance * 4;
    if (obj_get_nearest_type_to(OBJTYPE_Barrel, self, &distance)) {
        return;
    }

    //Create a barrel
    barrel = (SHBarrel_Setup*)obj_alloc_setup(sizeof(SHBarrel_Setup), OBJ_SHbarrel);
    barrel->base.loadDistance = 100;
    barrel->base.fadeDistance = 80;
    barrel->base.loadFlags = OBJSETUP_LOAD_MAIN;
    barrel->base.fadeFlags = OBJSETUP_FADE_CAMERA;
    barrel->base.x = self->srt.transl.x;
    barrel->base.y = self->srt.transl.y - 30.0f;
    barrel->base.z = self->srt.transl.z;
    barrel->yaw = self->srt.yaw >> 8;
    obj_create((ObjSetup*)barrel, OBJINIT_STANDALONE | OBJINIT_FLAG4, self->mapID, -1, NULL);
}

static void SHbarrelcreator_update(Object* self) {}

static void SHbarrelcreator_print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) { }

static void SHbarrelcreator_free(Object* self, s32 onlySelf) { }

static u32 SHbarrelcreator_get_model_flags(Object *self) {
    return MODFLAGS_NONE;
}

static u32 SHbarrelcreator_get_data_size(Object *self, u32 offsetAddr) {
    return 0;
}

DLL_IObject_Vtbl DLL_SHbarrelcreator_vtbl = {
    .setup = (void*)SHbarrelcreator_setup,
    .control = SHbarrelcreator_control,
    .update = SHbarrelcreator_update,
    .print = SHbarrelcreator_print,
    .free = SHbarrelcreator_free,
    .get_model_flags = SHbarrelcreator_get_model_flags,
    .get_data_size = SHbarrelcreator_get_data_size
};
