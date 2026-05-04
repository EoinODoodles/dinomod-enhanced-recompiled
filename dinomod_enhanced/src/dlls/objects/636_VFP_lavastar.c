#include "modding.h"

#include "PR/ultratypes.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/modgfx/182.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/rand.h"
#include "dll.h"

#include "recomp/dlls/objects/636_VFP_lavastar_recomp.h"

typedef struct {
    ObjSetup base;
    u8 _unk18[2];
    s16 unk1A;
    u8 _unk1C[2];
    s16 unk1E;
} VFP_lavastar_Setup;

typedef struct {
    f32 speed;
    s16 unk4;
    u32 soundHandle;
} VFP_lavastar_Data;

extern DLL_182 *sDLL_182;

RECOMP_PATCH void VFP_lavastar_control(Object* self) {
    VFP_lavastar_Setup* setup;
    VFP_lavastar_Data* objdata;

    objdata = (VFP_lavastar_Data*)self->data;
    setup = (VFP_lavastar_Setup*)self->setup;
    self->srt.transl.y += gUpdateRateF * objdata->speed;
    if ((setup->base.y + 1200.0f) < self->srt.transl.y) {
        objdata->speed = rand_next(5, 20) * 0.1f;
        self->srt.transl.y = setup->base.y;
    }
    if (rand_next(0, 3) == 0) {
        sDLL_182->vtbl->func0(self, 0, NULL, 4, -1, NULL);
    }
    // @recomp: Disable lavastar sound. It appears to be a blank sound and plays every frame. (originally by MusicalProgrammer)
    // if (objdata->soundHandle == 0) {
    //     gDLL_6_AMSFX->vtbl->play(self, SOUND_AAE, MAX_VOLUME, &objdata->soundHandle, NULL, 0, NULL);
    // }
}
