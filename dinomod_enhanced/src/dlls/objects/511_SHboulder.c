#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "sys/main.h"
#include "sys/objhits.h"
#include "sys/objects.h"

#include "objects/511_SHboulder.h"

#include "recomp/dlls/objects/511_SHboulder_recomp.h"

typedef struct {
    u8 fadeOut;
} SHboulder_Data;

/** Adds transforms into the setup, and removes boulder if its gamebit is already set */
RECOMP_PATCH void SHboulder_setup(Object *self, SHboulder_Setup *objSetup, s32 arg2) {
    if (objSetup->gamebitGone != NO_GAMEBIT && main_get_bits(objSetup->gamebitGone)){
        obj_destroy_object(self);
    }

    self->srt.yaw = objSetup->yaw << 8;
    self->srt.pitch = objSetup->pitch << 8;
    self->srt.roll = objSetup->roll << 8;
    if (objSetup->scale){
        self->srt.scale = objSetup->scale * 0.01f;
    }

    //@recomp: Scale objHits wrt. the object's scale
    f32 scaleMultiplier = 1.3f * self->srt.scale / self->def->scale;
    if (self->objhitInfo != NULL) {
        self->objhitInfo->unk52 *= scaleMultiplier;
        self->objhitInfo->unk56 *= scaleMultiplier;
        self->objhitInfo->unk54 *= scaleMultiplier;
    }
}

/** 
  * Fix fading out the boulder, set a gamebit when damaged, or destroy self when gamebit set externally.
  */
RECOMP_PATCH void SHboulder_control(Object* self) {
    s32 hitSphereID;
    s32 hitDamage;
    u8 boulderStruck;
    s32 opacity;
    Object* hitBy;
    SHboulder_Data* objData;
    SHboulder_Setup* objSetup; //@recomp

    objData = self->data;

    //@recomp: do nothing if the boulder can't be destroyed
    objSetup = (SHboulder_Setup*)self->setup;
    if (objSetup->invincible) {
        return;
    }

    //Handle fading out
    boulderStruck = objData->fadeOut;
    if (boulderStruck) {
        opacity = self->opacity - gUpdateRate * 4; //@recomp: fix bug in opacity fade
        if (opacity < 0) {
            opacity = 0;
            obj_destroy_object(self);
        }
        self->opacity = opacity;
        return;
    }

    //@recomp: check for gamebit
    if (!objData->fadeOut && 
        objSetup && 
        (objSetup->gamebitGone != NO_GAMEBIT) && 
        main_get_bits(objSetup->gamebitGone)
    ){
        objData->fadeOut = TRUE;
    }

    //Check for damage
    if (func_80025F40(self, &hitBy, &hitSphereID, &hitDamage) == Damage_Type_Barrel_Explosion) {
        objData->fadeOut = TRUE;

        //@recomp: set gamebit on destruction
        if (objSetup && objSetup->gamebitGone != NO_GAMEBIT){
            main_set_bits(objSetup->gamebitGone, 1);
        }
    }
}

/** 
  * Fixes a crash that happened when SHboulder loaded
  */
RECOMP_PATCH u32 SHboulder_get_data_size(Object *self, u32 a1){
    return sizeof(SHboulder_Data); //@recomp: return actual size of struct, instead of 0
}
