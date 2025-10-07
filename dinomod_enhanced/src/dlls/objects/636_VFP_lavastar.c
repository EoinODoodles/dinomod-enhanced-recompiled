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
    ObjCreateInfo base;
    u8 _unk18[2];
    s16 unk1A;
    u8 _unk1C[2];
    s16 unk1E;
} VFP_lavastar_CreateInfo;

typedef struct {
    f32 speed;
    s16 unk4;
    u32 soundHandle;
} VFP_lavastar_State;

extern DLL_182 *sDLL_182;

RECOMP_PATCH void VFP_lavastar_update(Object* self) {
    VFP_lavastar_CreateInfo* createInfo;
    VFP_lavastar_State* state;

    state = (VFP_lavastar_State*)self->state;
    createInfo = (VFP_lavastar_CreateInfo*)self->createInfo;
    self->srt.transl.y += delayFloat * state->speed;
    if ((createInfo->base.y + 1200.0f) < self->srt.transl.y) {
        state->speed = rand_next(5, 20) * 0.1f;
        self->srt.transl.y = createInfo->base.y;
    }
    if (rand_next(0, 3) == 0) {
        sDLL_182->vtbl->func0(self, 0, NULL, 4, -1, NULL);
    }
    // @recomp: Disable lavastar sound. It appears to be a blank sound and plays every frame. (originally by MusicalProgrammer)
    // if (state->soundHandle == 0) {
    //     gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_AAE, MAX_VOLUME, &state->soundHandle, NULL, 0, NULL);
    // }
}
