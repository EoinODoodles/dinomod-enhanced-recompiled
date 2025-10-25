#include "PR/ultratypes.h"
#include "functions.h"
#include "modding.h"

#include "game/objects/object.h"
#include "game/gamebits.h"
#include "recomputils.h"
#include "sys/dll.h"
#include "sys/gfx/gx.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "game/gamebits.h"
#include "dll.h"

#include "recomp/dlls/objects/581_SB_KyteCage_recomp.h"

void func_80034FF0(s32);

typedef struct {
    Object* kyte;
    union {
        s8 createLightning;
        u8 createLightningU;
    };
    s8 unk5;
    s8 unk6;
    s8 unk7;
} KyteCageState;

// Removes a flag check which could prevent the "Scales Escapes with Kyte" sequence from playing 
// just before you teleport away to SwapStone Circle (originally by MusicalProgrammer)
RECOMP_PATCH void kyteCage_print(Object* self, Gfx** gfx, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    KyteCageState* state;
    Object* kyte;
    u32 dYaw;
    ModelInstance* model;
    s8 boneIdx;
    s8 matrixIndex;
    MtxF* boneMatrix;
    MtxF resultMatrix;
    SRT galleonTransform;
    SRT boneTransform;
    void** dll;
    ObjDef* objDef;

    if (!visibility 
        // || main_get_bits(BIT_WM_Played_Randorn_First_Meeting) @recomp: remove check
    ) {
        return;
    }

    draw_object(self, gfx, mtxs, vtxs, pols, 1.0f);

    //Using the end of the cage's joint chain to update Kyte's transformation based on the cage's animation?
    state = self->data;
    if (state != NULL) {
        if (state->kyte != NULL) {
            kyte = state->kyte;
            if (kyte->unkB0 & 0x40) {
                state->kyte = NULL;
            }
            objDef = self->def;
            model = self->modelInsts[self->modelInstIdx];
            
            if (objDef->numAttachPoints != 0) {
                if (model->unk34 & 8) { 
                    boneIdx = objDef->pAttachPoints[1].bones[self->modelInstIdx];
                    boneMatrix = (MtxF *) &(((f32 **)model->matrices)[(model->unk34 & 1)][boneIdx << 4]);
                    
                    boneTransform.transl.x = 200.0f;
                    boneTransform.transl.y = 0.0f;
                    boneTransform.transl.z = -200.0f;
                    boneTransform.scale = 0.8f;
                    boneTransform.yaw = kyte->srt.yaw - self->srt.yaw;
                    boneTransform.pitch = 0;
                    boneTransform.roll = 0;
                    matrix_from_srt(&resultMatrix, &boneTransform);
                    matrix_concat_4x3(&resultMatrix, boneMatrix, &resultMatrix);
                    func_80034FF0((s32)&resultMatrix);
                    draw_object(kyte, gfx, mtxs, vtxs, pols, 1.0f);
                    func_80034FF0(NULL);
                }
                kyte->unkDC = 2;
            }

            //Create a lightning strike from above (at a random angle)
            if (state->createLightningU == 1) {
                galleonTransform.roll = 0;
                galleonTransform.pitch = 0;
                galleonTransform.yaw = 0;
                galleonTransform.transl.x = 0.0f;
                galleonTransform.transl.y = 0.0f;
                galleonTransform.transl.z = 0.0f;
                galleonTransform.scale = 1.0f;

                boneIdx = self->def->pAttachPoints->bones[0];
                boneMatrix = (MtxF *) &(((f32 **)model->matrices)[(model->unk34 & 1)][boneIdx << 4]);
                boneTransform.yaw = 0;
                boneTransform.roll = 0;
                boneTransform.pitch = 0;
                boneTransform.scale = 1.0f;
                boneTransform.transl.x = self->def->pAttachPoints->pos.x;
                boneTransform.transl.y = self->def->pAttachPoints->pos.y;
                boneTransform.transl.z = self->def->pAttachPoints->pos.z;
                vec3_transform(boneMatrix, boneTransform.transl.x, boneTransform.transl.y, boneTransform.transl.z, &boneTransform.transl.x, &boneTransform.transl.y, &boneTransform.transl.z);
                if (self->parent) {
                    galleonTransform.yaw = self->parent->srt.yaw;
                    rotate_vec3((SRT* ) &galleonTransform, (f32* ) &boneTransform.transl);
                    boneTransform.transl.x += self->parent->srt.transl.x;
                    boneTransform.transl.y += self->parent->srt.transl.y;
                    boneTransform.transl.z += self->parent->srt.transl.z;
                } else {
                    boneTransform.transl.x += gWorldX;
                    boneTransform.transl.z += gWorldZ;
                }
                state->createLightning = 0;
                dll = dll_load_deferred(0x200D, 1);
                ((DLL_Unknown*)dll)->vtbl->func[0].withSevenArgs((s32)self, 0, (s32)&boneTransform, 1, -1, 0xD, 0);
                dll_unload(dll);

            //Create a lightning trails across deck (at a random angle)
            } else if (state->createLightningU == 2) {
                galleonTransform.roll = 0;
                galleonTransform.pitch = 0;
                galleonTransform.yaw = 0;
                galleonTransform.transl.x = 0.0f;
                galleonTransform.transl.y = 0.0f;
                galleonTransform.transl.z = 0.0f;
                galleonTransform.scale = 1.0f;

                boneIdx = self->def->pAttachPoints->bones[0];
                boneMatrix = (MtxF *) &(((f32 **)model->matrices)[(model->unk34 & 1)][boneIdx << 4]);
                boneTransform.yaw = 0;
                boneTransform.roll = 0;
                boneTransform.pitch = 0;
                boneTransform.scale = 1.0f;
                boneTransform.transl.x = self->def->pAttachPoints->pos.x;
                boneTransform.transl.y = self->def->pAttachPoints->pos.y;
                boneTransform.transl.z = self->def->pAttachPoints->pos.z;

                vec3_transform(boneMatrix, boneTransform.transl.x, boneTransform.transl.y, boneTransform.transl.z, &boneTransform.transl.x, &boneTransform.transl.y, &boneTransform.transl.z);
                if (self->parent) {
                    galleonTransform.yaw = self->parent->srt.yaw;
                    rotate_vec3((SRT* ) &galleonTransform, (f32* ) &boneTransform.transl);
                    boneTransform.transl.x += self->parent->srt.transl.x;
                    boneTransform.transl.y += self->parent->srt.transl.y;
                    boneTransform.transl.z += self->parent->srt.transl.z;
                } else {
                    boneTransform.transl.x += gWorldX;
                    boneTransform.transl.z += gWorldZ;
                }
                state->createLightning = 0;
                dll = dll_load_deferred(0x200F, 1);
                ((DLL_Unknown*)dll)->vtbl->func[0].withSevenArgs((s32)self, 0, (s32)&boneTransform, 1, -1, 0xF, 0);
                dll_unload(dll);
            }
        }
    }
}
