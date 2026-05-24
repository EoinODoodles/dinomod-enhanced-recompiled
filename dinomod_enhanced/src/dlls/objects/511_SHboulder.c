#include "custom_gamebits.h"
#include "modding.h"
#include "recomputils.h"

#include "PR/os.h"
#include "common.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/objhits.h"
#include "sys/objects.h"

#include "objects/511_SHboulder.h"

#include "recomp/dlls/objects/511_SHboulder_recomp.h"

typedef struct {
    u8 fadeOut;
    u16 timer;
} SHboulder_Data;

/** Adds transforms into the setup, and removes boulder if its gamebit is already set */
RECOMP_PATCH void SHboulder_setup(Object *self, SHboulder_Setup *objSetup, s32 arg2) {
    SHboulder_Data* objData = self->data;//@recomp
    if (objSetup->gamebitGone != NO_GAMEBIT && main_get_bits(objSetup->gamebitGone)){
        objData->fadeOut = TRUE;
        self->opacity = 0;
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

/* Creates an explosion of rock debris, for when the river's unblocked */
static void SHboulder_create_debris(Object* self, SHboulder_Setup* objSetup) {
    s32 i;
    SRT transform;
    f32 params[2];

    //Explosion
    transform.scale = 0.2f;
    transform.transl.y = 40;
    gDLL_17_partfx->vtbl->spawn(self, 0x14, &transform, 0, -1, &params);
    
    //Rock particles
    transform.scale = 30;
    transform.transl.z = 10;
    for (i = 0; i < 50; i++){
        params[0] = rand_next(-30, 30) * 0.02f * 0.7f;
        params[1] = -rand_next(18, 22) * 0.03f * 0.7f;
        transform.transl.x = rand_next(-30, 30);
        transform.transl.y = rand_next(-3, 40);
        gDLL_17_partfx->vtbl->spawn(self, 0x3F2, &transform, 60, -1, &params);
    }
}

#define SECONDS_IN_HOUR (60 * 60)
#define SECONDS_IN_MINUTE (60)

/** Sets a specific time of day/night so the sun/moon is framed in a nice way during the boulder's sequence */
static void SHboulder_set_seq_time(void) {
    f32 time;

    if (gDLL_7_Newday->vtbl->func8(&time)) {
        gDLL_7_Newday->vtbl->func9((50 * SECONDS_IN_MINUTE));
    } else {
        gDLL_7_Newday->vtbl->func9((13 * SECONDS_IN_HOUR) + (40 * SECONDS_IN_MINUTE));
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
    objSetup = (SHboulder_Setup*)self->setup;

    //@recomp: check for gamebit
    if (!objData->fadeOut && 
        objSetup && 
        (objSetup->gamebitGone != NO_GAMEBIT) && 
        main_get_bits(objSetup->gamebitGone)
    ){
        objData->fadeOut = TRUE;
        if (objSetup->debris) {
            SHboulder_set_seq_time();
            SHboulder_create_debris(self, objSetup);
            self->opacity = 0;
        }
    }

    //@recomp: destroy the special sequence boulder after it's finished creating particles
    if (objSetup->debris && objData->fadeOut) {
        objData->timer += gUpdateRate;
        if (objData->timer > 333) {
            obj_destroy_object(self);
        }
    }

    //@recomp: do nothing if the boulder can't be destroyed
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

    //Check for damage
    if (func_80025F40(self, &hitBy, &hitSphereID, &hitDamage) == Damage_Type_Explosion) {
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
