#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "configs.h"

#include "common.h"
#include "dlls/objects/common/sidekick.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objanim.h"
#include "sys/objtype.h"
#include "dll.h"

#include "recomp/dlls/objects/265_SHvines_recomp.h"

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 roll;  //@recomp: repurposing unused field
    u8 scale; //@recomp: repurposing unused field
    s16 flameDistance;
    s16 gamebitBurnt;
} SHvines_Setup;

typedef struct {
    s32 vertexCount;
} SHvines_Data;

RECOMP_PATCH void SHvines_setup(Object* self, SHvines_Setup* objSetup, s32 arg2) {
    SHvines_Data* objData;
    ModelInstance* modelInstance;
    Model* model;

    objData = self->data;
    obj_add_object_type(self, OBJTYPE_TrickyTarget);

    if (main_get_bits(objSetup->gamebitBurnt)) {
        self->srt.flags |= OBJFLAG_INVISIBLE;
        obj_free_tick(self);
        func_800267A4(self);
    }
    
    self->srt.yaw = objSetup->yaw << 8;

    //@recomp: add roll param
    self->srt.roll = objSetup->roll << 8;

    //@recomp: add scale param
    if (objSetup->scale) {
        self->srt.scale = (f32)objSetup->scale/50.0f;

        //If scale is increased significantly, have player ignore vines' objHits (so HITS can block instead - more controllable)
        if (self->srt.scale > 2.0f) {
            func_800267A4(self);
        }
    }
    
    create_temp_dll(DLL_ID_53_MOVELIB);

    modelInstance = self->modelInsts[self->modelInstIdx];
    model = modelInstance->model;
    objData->vertexCount = model->vertexCount;
}

RECOMP_PATCH void SHvines_control(Object* self) {
    SHvines_Setup* objSetup;
    Object* sidekick;

    objSetup = (SHvines_Setup*)self->setup;
    sidekick = get_sidekick();
    
    //@recomp: only show Flame command etc. before the vines are burnt
    if (self->opacity == OBJECT_OPACITY_MAX) {
        //Allow the Flame command when nearby
        if (sidekick != NULL) {
            if (vec3_distance_squared(&self->globalPosition, &get_player()->globalPosition) <= SQ(objSetup->flameDistance)) {
                ((DLL_ISidekick*)sidekick->dll)->vtbl->enable_command(sidekick, Sidekick_Command_INDEX_4_Flame);
            }
        }

        //@recomp: start fadeout if the vine's gamebit is set externally
        if ((objSetup->gamebitBurnt != NO_GAMEBIT) && main_get_bits(objSetup->gamebitBurnt)) {
            self->opacity = OBJECT_OPACITY_MAX - 3; //start fade out
        }
    }

    //Fade out
    if (self->opacity < OBJECT_OPACITY_MAX) {
        if (self->opacity < gUpdateRate) {
            obj_free_tick(self);
            self->srt.flags |= OBJFLAG_INVISIBLE;
            func_800267A4(self);
            return;
        }
        
        self->opacity -= gUpdateRate;
    }
}
