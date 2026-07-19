#include "modding.h"
#include "recomputils.h"

#include "game/objects/object_id.h"
#include "game/objects/object.h"
#include "sys/map.h"
#include "sys/objlib.h"
#include "sys/objprint.h"
#include "sys/print.h"
#include "dlls/objects/290_magicplant.h"

#include "recomp/dlls/objects/290_MagicPlant_recomp.h"

static f32 dMagicDustY[] = {
    -40, -35, -30, -25
};

#define ATTACH_JOINT_ID 5

RECOMP_PATCH void MagicPlant_print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    MtxF* jointMtx;
    f32 x;
    f32 y;
    f32 z;
    MtxF mtx;
    SRT srt;
    Object* pad;
    MagicPlant_Setup* objSetup;
    MagicPlant_Data* objData;
    ModelInstance *modelInstance;
    s32 dustIdx;

    objData = self->data;
    objSetup = (MagicPlant_Setup*)self->setup;
    if (visibility == 0) {
        return;
    }

    objprintDrawModel(self, gdl, mtxs, vtxs, pols, 1.0f);
    modelInstance = self->modelInsts[self->modelInstIdx];

    //Position the MagicDust gem at the centre of the plant's petals
    if ((objData->magic != NULL) && (modelInstance->unk34 & 8) && (objData->magic->unkC4 != NULL)) {

        {
            //@recomp: bail if the gem is deleted
            if (objData->magic->stateFlags & OBJSTATE_DESTROYED){
                objData->magic = NULL;
                return;
            }
        }

        //Get the matrix for the end joint of the main joint chain
        jointMtx = objGetAttachPointBoneMatrix(self, ATTACH_JOINT_ID);
        if (jointMtx == NULL) {
            return;
        }

        //Get coords for the end joint of the main joint chain
        objGetAttachPointLocalSpace(self, ATTACH_JOINT_ID, &x, &y, &z);

        dustIdx = objSetup->dustIdx;
        srt.transl.x = x;
        srt.transl.y = dMagicDustY[dustIdx & 3];
        srt.transl.z = z;
        srt.yaw = 0;
        srt.pitch = 0;
        srt.roll = 0;
        srt.scale = objData->magic->srt.scale / self->srt.scale;
        mathYprXyzMtx(&mtx, &srt);
        mathMtxCat4x3F(&mtx, jointMtx, &mtx);

        objData->magic->srt.transl.x = mtx.m[3][0] + gWorldX;
        objData->magic->srt.transl.y = mtx.m[3][1];
        objData->magic->srt.transl.z = mtx.m[3][2] + gWorldZ;

        objprintSetModelMatrixOverride(&mtx);
        objprintDrawModel(objData->magic, gdl, mtxs, vtxs, pols, 1.0f);
        objprintSetModelMatrixOverride(NULL);
    }
}
