#include "game/gamebits.h"
#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/objects/345_WaterFallSpray_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 unk18;
/*1A*/ s8 roll;
/*1B*/ s8 pitch;
/*1C*/ s8 yaw;
/*1D*/ u8 amplitudeX;
/*1E*/ u8 amplitudeZ;
/*1F*/ u8 amplitudeY;
/*20*/ u8 distance;
/*21*/ u8 unk21;
/*22*/ s8 unk22;
/*23*/ u8 unk23;
/*24*/ u8 iterations;
} WaterFallSpray_Setup;

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

    //@recomp: stop creating spray particles after Kyte pulls lever
    //@TO-DO: store gamebitID on unused field in WaterFallSpray_Setup to make this check less hardcoded / more instance-specific? (unk18 seems intended for a gamebit anyway!)
    if (BIT_144 != NO_GAMEBIT && main_get_bits(BIT_144)){
        return;
    }

    if (self->unkDC <= 0) {
        dx = self->positionMirror.x - player->positionMirror.x;
        dy = self->positionMirror.y - player->positionMirror.y;
        dz = self->positionMirror.z - player->positionMirror.z;

        if (sqrtf(dx*dx + dy*dy + dz*dz) <= setup->distance * 0x10 || setup->distance == 0) {

            for (i = 0; i < setup->iterations; i++){
                srt.transl.x = rand_next(-setup->amplitudeX, setup->amplitudeX);
                srt.transl.y = rand_next(-setup->amplitudeY, setup->amplitudeY);
                srt.transl.z = rand_next(-setup->amplitudeZ, setup->amplitudeZ);

                if (setup->unk23 & 1) {
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_320, &srt, PARTFXFLAG_4, -1, NULL);
                }
                if (setup->unk23 & 2) {
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_321, &srt, PARTFXFLAG_4, -1, NULL);
                }
                if (setup->unk23 & 4) {
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_322, &srt, PARTFXFLAG_4, -1, NULL);
                }
                if (setup->unk23 & 8) {
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_351, &srt, PARTFXFLAG_4, -1, NULL);
                }
            }

        }
        self->unkDC = -setup->iterations;
    } else if (self->unkDC > 0) {
        self->unkDC -= gUpdateRate;
    }
}
