#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "sys/math.h"
#include "sys/print.h"

#include "recomp/dlls/objects/435_KrazoaSpirit_recomp.h"

typedef struct {
    AnimObj_Data base;
    /** RECOMP EXTENDED */
    TextureAnimator* animTexture;
    u32 frameTimer;
} Spirit_Data;

extern DLL_Unknown* data_modGfx;

/** Getting the texture animator so it can be used in the control function */
RECOMP_PATCH void Spirit_setup(Object* self, AnimObj_Setup* objSetup, s32 arg2) {
    Spirit_Data* objData;
    s32 temp_v0;
    s16 id;

    objData = self->data;
    
    objData->base.eventGamebit = objSetup->unk1A;
    objData->base.unk7A = -1;
    objData->base.unk24 = 1.0f / ((u8)objSetup->unk24 + 1.0f);
    objData->base.unk28 = -1;
    
    temp_v0 = self->unkDC;
    
    if ((temp_v0 == 0) && (objSetup->sequenceIdBitfield != 1)) {
        gDLL_3_Animation->vtbl->func6(&objData->base, objSetup);
        self->unkDC = objSetup->sequenceIdBitfield + 1;
    } else if (temp_v0 && (temp_v0 != objSetup->sequenceIdBitfield + 1)) {
        gDLL_3_Animation->vtbl->func8(&objData->base);
        if (objSetup->sequenceIdBitfield != -1) {
            gDLL_3_Animation->vtbl->func6(&objData->base, objSetup);
        }
        self->unkDC = objSetup->sequenceIdBitfield + 1;
    }
    
    id = self->id;
    self->opacityWithFade = 0;
    self->opacity = 0;
    
    if (id == OBJ_DBSH_SpiritPriz || 
        id == OBJ_DFSH_SpiritPriz || 
        id == OBJ_MMSH_SpiritPriz || 
        id == OBJ_ECSH_SpiritPriz || 
        id == OBJ_GPSH_SpiritPriz || 
        id == OBJ_CCSH_SpiritPriz || 
        id == OBJ_WGSH_SpiritPriz || 
        id == OBJ_NWSH_SpiritPriz) {   
        data_modGfx = dll_load_deferred(0x102D, 1);
    }

    //@recomp: get texture animator
    objData->animTexture = func_800348A0(self, 0, 0);
}

/** Play the Krazoa Spirits' unused trail texture animation */
RECOMP_PATCH void Spirit_print(Object* self, Gfx **gdl, Mtx **mtxs, Vertex **vtxs, Triangle **pols, s8 visibility) {
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
        objData->frameTimer %= 28 * 2; //28 frames

        if (objData->animTexture){
            objData->animTexture->frame = (objData->frameTimer / 2) << 8;
        }
    }

    if (visibility) {
        draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);
        if (self->opacity) {
            if (id == OBJ_DFSH_SpiritPriz || id == OBJ_CCSH_SpiritPriz) {
                if ((s16)rand_next(0, 1)) {
                    data_modGfx->vtbl->func[0].withSixArgs((s32)self, 0, 0, 4, -1, 0);
                }
            } else if (id == OBJ_MMSH_SpiritPriz || id == OBJ_WGSH_SpiritPriz) {
                if ((s16)rand_next(0, 1)) {
                    data_modGfx->vtbl->func[0].withSixArgs((s32)self, 1, 0, 4, -1, 0);
                }
            } else if (id == OBJ_ECSH_SpiritPriz || id == OBJ_GPSH_SpiritPriz) {
                if ((s16)rand_next(0, 1)) {
                    data_modGfx->vtbl->func[0].withSixArgs((s32)self, 2, 0, 4, -1, 0);
                }
            } else if (id == OBJ_DBSH_SpiritPriz || id == OBJ_NWSH_SpiritPriz){
                if ((s16)rand_next(0, 1)) {
                    data_modGfx->vtbl->func[0].withSixArgs((s32)self, 3, 0, 4, -1, 0);
                }
            }
        }
    }
}

RECOMP_PATCH u32 Spirit_get_data_size(Object *self, u32 a1) {
    return sizeof(Spirit_Data);
}
