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

// Toggle WaterFallSpray via gamebit, for when Kyte pulls the lever in Cape Claw 2 (Originally by MusicalProgrammer)
RECOMP_PATCH void WaterFallSpray_control(Object *self) {
    Object *player;
    f32 dz;
    f32 dx;
    f32 dy;
    s16 i;
    WaterFallSpray_Setup *setup;
    SRT srt;

    setup = (WaterFallSpray_Setup *)self->setup;
    player = get_player();
    if (!player){
        return;
    }

    //@recomp: stop creating spray particles after objSetup-specific gamebit set (i.e. Kyte pulls lever)
    if (setup->gamebit != NO_GAMEBIT) {
        u32 gamebitSet = main_get_bits(setup->gamebit);

        if ((gamebitSet && !setup->invertGamebit) || (!gamebitSet && setup->invertGamebit)){
            return;
        }
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
