#include "game/gamebits.h"
#include "modding.h"
#include "recomputils.h"
#include "common_objsetups.h"

#include "common.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/objects/345_WaterFallSpray_recomp.h"

typedef enum {
    WaterFallSpray_FLAG_Big_Mist_Cloud = 0x1,
    WaterFallSpray_FLAG_Falling_Droplets = 0x2,
    WaterFallSpray_FLAG_Small_Mist_Cloud = 0x4,
    WaterFallSpray_FLAG_Mist_Jet = 0x8
} WaterFallSpray_Flags;

enum AMSFXWaterFallsFlags {
    // The follow flags cut the volume of the high or low waterfall sfx in half for each flag set.
    AMSFX_WATERFALLS_LOWER_HIGH = 0x1,
    AMSFX_WATERFALLS_LOWER_LOW = 0x2,
    AMSFX_WATERFALLS_LOWER_HIGH2 = 0x4,
    AMSFX_WATERFALLS_LOWER_LOW2 = 0x8,
    // Clear the list of waterfall sprays and re-search the map for an updated list.
    AMSFX_WATERFALLS_REFRESH = 0x10
};

typedef struct {
    u8 enabled;
} WaterFallSpray_Data; //@recomp: custom struct

/**
  * Handles toggling the WaterFallSpray on/off via gamebit 
  *
  * WaterFallSprays' sounds are toggled too when state changes, fixing bugs like 
  * the waterfall in Cape Claw continuing to emit sound even after Kyte pulls the nearby lever. 
  */
static void WaterFallSpray_check_if_enabled(Object *self, WaterFallSpray_Setup* objSetup) {
    WaterFallSpray_Data* objData = self->data;
    if (!objSetup || !objData) {
        return;
    }

    //Check if the WaterFallSpray has no gamebit assigned (always on)
    if (objSetup <= NO_GAMEBIT + 1) {
        if (!objData->enabled) {
            objData->enabled = TRUE;
        }
        return;
    }

    //If it does use a gamebit, check whether it's set
    u32 gamebitSet = main_get_bits(objSetup->gamebit);
    u8 prevEnabled = objData->enabled;

    //Handle WaterFallSprays that switch on when their gamebit is set
    if (objSetup->invertGamebit) {
        if (!objData->enabled && gamebitSet) {
            objData->enabled = TRUE;
        } else if (objData->enabled && !gamebitSet) {
            objData->enabled = FALSE;
        }
    //Handle WaterFallSprays that switch off when their gamebit is set
    } else {
        if (objData->enabled && gamebitSet) {
            objData->enabled = FALSE;
        } else if (!objData->enabled && !gamebitSet) {
            objData->enabled = TRUE;
        }
    }

    //Signal AMSFX to refresh its WaterFallSprays list when state changes
    if (objData->enabled != prevEnabled) {
        gDLL_6_AMSFX->vtbl->water_falls_set_flags(AMSFX_WATERFALLS_REFRESH);
    }
}

RECOMP_PATCH void WaterFallSpray_setup(Object *self, WaterFallSpray_Setup *setup, s32 reset) {
    WaterFallSpray_Data* objData = self->data;

    self->srt.roll = setup->roll << 8;
    self->srt.pitch = setup->pitch << 8;
    self->srt.yaw = setup->yaw << 8;
    self->unkDC = 0;
    self->stateFlags |= OBJSTATE_UPDATE_DISABLED;

    //@recomp: handle toggling via gamebits
    WaterFallSpray_check_if_enabled(self, setup);
}

// Toggle WaterFallSpray via gamebit, for when Kyte pulls the lever in Cape Claw 2 (Originally by MusicalProgrammer)
RECOMP_PATCH void WaterFallSpray_control(Object *self) {
    Object *player;
    f32 dz;
    f32 dx;
    f32 dy;
    s16 i;
    WaterFallSpray_Setup *setup;
    SRT srt;
    /* RECOMP */
    WaterFallSpray_Data* objData = self->data;

    setup = (WaterFallSpray_Setup *)self->setup;
    player = get_player();
    if (!player){
        return;
    }

    //@recomp: stop creating spray particles after objSetup-specific gamebit set (i.e. Kyte pulls lever)
    WaterFallSpray_check_if_enabled(self, setup);
    if (objData->enabled == FALSE) {
        return;
    }

    if (self->unkDC <= 0) {
        dx = self->globalPosition.x - player->globalPosition.x;
        dy = self->globalPosition.y - player->globalPosition.y;
        dz = self->globalPosition.z - player->globalPosition.z;

        if (sqrtf(SQ(dx) + SQ(dy) + SQ(dz)) <= setup->distance * 0x10 || setup->distance == 0) {

            for (i = 0; i < setup->iterations; i++){
                srt.transl.x = rand_next(-setup->amplitudeX, setup->amplitudeX);
                srt.transl.y = rand_next(-setup->amplitudeY, setup->amplitudeY);
                srt.transl.z = rand_next(-setup->amplitudeZ, setup->amplitudeZ);

                if (setup->flags & WaterFallSpray_FLAG_Big_Mist_Cloud) {
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_320, &srt, PARTFXFLAG_4, -1, NULL);
                }
                if (setup->flags & WaterFallSpray_FLAG_Falling_Droplets) {
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_321, &srt, PARTFXFLAG_4, -1, NULL);
                }
                if (setup->flags & WaterFallSpray_FLAG_Small_Mist_Cloud) {
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_322, &srt, PARTFXFLAG_4, -1, NULL);
                }
                if (setup->flags & WaterFallSpray_FLAG_Mist_Jet) {
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_351, &srt, PARTFXFLAG_4, -1, NULL);
                }
            }

        }
        self->unkDC = -setup->iterations;
    } else if (self->unkDC > 0) {
        self->unkDC -= gUpdateRate;
    }
}

RECOMP_PATCH u32 WaterFallSpray_get_data_size(Object *self, u32 a1){
    return sizeof(WaterFallSpray_Data);
}
