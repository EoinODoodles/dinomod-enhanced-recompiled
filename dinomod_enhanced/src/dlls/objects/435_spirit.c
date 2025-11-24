#include "modding.h"
#include "recomputils.h"

#include "common.h"

#include "recomp/dlls/_asm/435_recomp.h"

typedef struct {
    AnimObj_Data base;
    /** RECOMP EXTENDED */
    u32 frameTimer;
} Spirit_Data;

extern DLL_Unknown* _data_0;

/** Play the Krazoa Spirits' unused trail texture animation */
RECOMP_PATCH void dll_435_print(Object* self, Gfx **gdl, Mtx **mtxs, Vertex **vtxs, Triangle **pols, s8 visibility) {
    s16 id;
    s16 random;
    Camera *camera;
    //@recomp vars
    Spirit_Data *objData = self->data;
    TextureAnimator* animatedTexture;

    camera = get_camera();
    if (self->id == OBJ_ECSH_SpiritCup && camera) {
        self->srt.yaw = 0xFFFF - camera->srt.yaw;
    }
    
    id = self->id;

    //@recomp: play Krazoa Spirit's trail texture animation (on twos)
    if (
        id == OBJ_DFSH_Spirit ||
        id == OBJ_DBSH_Spirit ||
        id == OBJ_MMSH_Spirit ||
        id == OBJ_ECSH_Spirit ||
        id == OBJ_CCSH_Spirit ||
        id == OBJ_WGSH_Spirit ||
        id == OBJ_GPSH_Spirit ||
        id == OBJ_NWSH_Spirit
    ){
        objData->frameTimer += gUpdateRate;
        objData->frameTimer %= 28 * 2;

        animatedTexture = func_800348A0(self, 0, 0);
        if (animatedTexture){
            animatedTexture->unk0 = (objData->frameTimer / 2) << 8;
        }
    }

    if (visibility) {
        draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);
        if (self->opacity) {
            if (id == OBJ_DFSH_SpiritPriz || id == OBJ_CCSH_SpiritPriz) {
                if ((s16)rand_next(0, 1)) {
                    _data_0->vtbl->func[0].withSixArgs((s32)self, 0, 0, 4, -1, 0);
                }
            } else if (id == OBJ_MMSH_SpiritPriz || id == OBJ_WGSH_SpiritPriz) {
                if ((s16)rand_next(0, 1)) {
                    _data_0->vtbl->func[0].withSixArgs((s32)self, 1, 0, 4, -1, 0);
                }
            } else if (id == OBJ_ECSH_SpiritPriz || id == OBJ_GPSH_SpiritPriz) {
                if ((s16)rand_next(0, 1)) {
                    _data_0->vtbl->func[0].withSixArgs((s32)self, 2, 0, 4, -1, 0);
                }
            } else if (id == OBJ_DBSH_SpiritPriz || id == OBJ_NWSH_SpiritPriz){
                if ((s16)rand_next(0, 1)) {
                    _data_0->vtbl->func[0].withSixArgs((s32)self, 3, 0, 4, -1, 0);
                }
            }
        }
    }
}

RECOMP_PATCH u32 dll_435_get_data_size(Object *self, u32 a1) {
    return sizeof(Spirit_Data);
}
