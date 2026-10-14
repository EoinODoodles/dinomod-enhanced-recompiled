#include "cheats.h"
#include "math_util.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "skeleton.h"

#include "PR/os.h"
#include "common.h"
#include "dlls/objects/234_ScorpionRobot.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "macros.h"
#include "sys/gfx/model_asm.h"
#include "sys/gfx/model.h"
#include "sys/math.h"
#include "sys/print.h"
#include "sys/rarezip.h"

#define ALIGN8(a)  (((u32) (a) & ~0x7) + 0x8)
#define ALIGN16(a)  ((u32) (a) & ~0xF)
#define PAD16(a) while (a & 7){a++;}

#define ANIM_SLOT_COUNT 128
#define ANIM_SLOT_RC(animationRef, idx) ((((s32*)animationRef) + (idx << 1)))[0]
#define ANIM_SLOT_ANIM(animationRef, idx) ((((s32*)animationRef) + (idx << 1)))[1]

#define MODEL_SLOT_ID(modelRef, idx) ((((s32*)modelRef) + (idx << 1)))[0]
#define MODEL_SLOT_MODEL(modelRef, idx) ((((s32*)modelRef) + (idx << 1)))[1]

extern s32* gFile_MODELS_TAB;
extern ModelSlot* gLoadedModels;
extern s32* gFreeModelSlots;
extern s32 gNumLoadedModels;
extern s32 gNumModelsTabEntries;
extern s32 gNumFreeModelSlots;
extern s16* gAuxBuffer;
extern u32* D_800B17BC;
extern AnimSlot* gLoadedAnims;
extern s32* gBuffer_ANIM_TAB;
extern s32 gNumLoadedAnims;
extern s16 SHORT_ARRAY_800b17d0[48]; // TODO: check length, it's at most 48

//@recomp: increase array length (SHORT_ARRAY_800b17d0)
static s16 rsExtendedTiltList[100];

/* Use extended tiltList for seqJoints */
RECOMP_PATCH void mod_func_800199A8(MtxF* arg0, ModelInstance* modelInst, AnimState* animState, f32 animProgress, u32 arg4) {
    MtxF* jointMtxs;
    Model* model;
    s32 var_s0;
    s32 var_s1;
    s32 var_v1;
    s32 animFlags;
    s32 temp_v0;
    AnimState animStateN;

    animFlags = 0;
    model = modelInst->model;
    jointMtxs = modelInst->matrices[modelInst->unk34 & 1];
    animState->curAnimationFrame[0] = animState->totalAnimationFrames[0] * animProgress;
    
    if (model->unk71 & 8) {
        animStateN.anims[0] = animState->anims[0];
        animStateN.anims[1] = animState->anims[1];
        animStateN.anims2[0] = animState->anims2[0];
        animStateN.anims2[1] = animState->anims2[1];

        var_s0 = 0;
        for (; var_s0 < 2; var_s0++) {
            if (animState->unk58[0] != 0) {
                var_s1 = var_s0;
            } else {
                var_s1 = 0;
            }
            animStateN.animIndexes[var_s0] = animState->animIndexes[var_s1];
            animStateN.unk60[var_s0] = animState->unk60[var_s1];
            animStateN.totalAnimationFrames[var_s0] = animState->totalAnimationFrames[var_s1];
            animStateN.curAnimationFrame[var_s0] = animState->curAnimationFrame[var_s1];
            animStateN.unk34[var_s0] = animState->unk34[var_s1];
        }

        animStateN.unk58[0] = animState->unk58[0];
        mod_func_8001A1D4(model, &animStateN, 2);
        temp_v0 = animState->unk62[1];
        if (temp_v0 & 1) {
            animFlags = 0x10;
        }

        if (temp_v0 & 4) {
            animFlags |= 0x20;
        }

        //@recomp: use extended tilt list
        func_8001B4F0(&jointMtxs, arg0, &animStateN, model->joints, model->jointCount, rsExtendedTiltList, arg4, animFlags | 0x40);
        return;
    }

    for (var_s0 = 0; var_s0 < 2; var_s0++) {
        if (var_s0 != 0) {
            var_v1 = animState->unk5C[0];
        } else {
            var_v1 = animState->unk58[1];
        }
        if (var_v1 != 0) {
            if (animState->unk58[0] != 0) {
                var_s1 = 4 << var_s0;
            } else {
                var_s1 = 0;
            }
            animStateN.unk60[0] = animState->unk60[var_s0];
            animStateN.totalAnimationFrames[0] = animState->totalAnimationFrames[var_s0];
            animStateN.curAnimationFrame[0] = animState->curAnimationFrame[var_s0];
            animStateN.unk34[0] = animState->unk34[var_s0];
            animStateN.unk60[1] = animState->unk60[var_s0];
            animStateN.totalAnimationFrames[1] = animState->totalAnimationFrames[var_s0];
            animStateN.curAnimationFrame[1] = animState->curAnimationFrame[var_s0];
            animStateN.unk34[1] = animState->unk3C[var_s0];
            if (model->unk71 & 0x40) {
                animStateN.animIndexes[0] = 0;
                animStateN.animIndexes[1] = 1;
                animStateN.anims[0] = animState->anims[animState->animIndexes[var_s0]];
                animStateN.anims[1] = animState->anims2[animState->unk48[var_s0]];
            } else {
                animStateN.animIndexes[0] = animState->animIndexes[var_s0];
                animStateN.animIndexes[1] = animState->unk48[var_s0];
            }
            animStateN.unk58[0] = var_v1;
            mod_func_8001A1D4(model, &animStateN, 2);

            //@recomp: use extended tilt list
            func_8001B4F0(&jointMtxs, arg0, &animStateN, model->joints, (s32) model->jointCount, rsExtendedTiltList, (s32) arg4, var_s1);
            if (var_s1 != 0) {
                animFlags |= 1 << var_s0;
            }
        }
    }
    if (((animState->unk58[1] == 0) && (animState->unk5C[0] == 0)) || (animFlags != 0)) {
        var_s1 = 1;
        if (animState->unk58[0] != 0) {
            var_s1 = 2;
        }
        animStateN.anims[0] = animState->anims[0];
        animStateN.anims[1] = animState->anims[1];
        animStateN.anims2[0] = animState->anims2[0];
        animStateN.anims2[1] = animState->anims2[1];

        for (var_s0 = 0; var_s0 < var_s1; var_s0++) {
            animStateN.animIndexes[var_s0] = animState->animIndexes[var_s0];
            animStateN.unk60[var_s0] = animState->unk60[var_s0];
            animStateN.totalAnimationFrames[var_s0] = animState->totalAnimationFrames[var_s0];
            animStateN.curAnimationFrame[var_s0] = animState->curAnimationFrame[var_s0];
            animStateN.unk34[var_s0] = animState->unk34[var_s0];
        }
        
        animStateN.unk58[0] = animState->unk58[0];
        mod_func_8001A1D4(model, &animStateN, var_s1);
        temp_v0 = animState->unk62[1];
        if (temp_v0 & 1) {
            animFlags |= 0x10;
        }
        if (temp_v0 & 4) {
            animFlags |= 0x20;
        }

        //@recomp: use extended tilt list
        func_8001B4F0(&jointMtxs, arg0, &animStateN, model->joints, (s32) model->jointCount, rsExtendedTiltList, (s32) arg4, animFlags);
    }
}

/* Use extended tiltList for seqJoints */
RECOMP_PATCH void mod_func_80019FC0(MtxF* arg0, ModelInstance* modelInst, AnimState* animState, f32 arg3, u32 arg4, u8 animIdx0, u8 arg6, u8 animIdx1, u8 flags, s16 arg9) {
    MtxF* spA4;
    Model* temp_s1;
    AnimState sp38;

    temp_s1 = modelInst->model;
    spA4 = modelInst->matrices[modelInst->unk34 & 1];
    if (flags & 0x10) {
        animState->curAnimationFrame[0] = animState->totalAnimationFrames[0] * arg3;
    }
    sp38.unk60[0] = animState->unk60[animIdx0];
    sp38.totalAnimationFrames[0] = animState->totalAnimationFrames[animIdx0];
    sp38.curAnimationFrame[0] = animState->curAnimationFrame[animIdx0];
    sp38.unk34[0] = animState->unk34[animIdx0];
    sp38.unk60[1] = animState->unk60[arg6];
    sp38.totalAnimationFrames[1] = animState->totalAnimationFrames[arg6];
    sp38.curAnimationFrame[1] = animState->curAnimationFrame[arg6];
    sp38.unk34[1] = animState->unk34[animIdx1];
    if (temp_s1->unk71 & 0x40) {
        sp38.animIndexes[0] = 0;
        sp38.animIndexes[1] = 1;
        sp38.anims[0] = animState->anims[animState->animIndexes[animIdx0]];
        if (animIdx1 < 2) {
            sp38.anims[1] = animState->anims[animState->animIndexes[animIdx1]];
        } else {
            sp38.anims[1] = animState->anims2[animState->animIndexes[animIdx1]];
        }
    } else {
        sp38.animIndexes[0] = animState->animIndexes[animIdx0];
        sp38.animIndexes[1] = animState->animIndexes[animIdx1];
    }
    if (arg9 == 0) {
        arg9 = 1;
    }
    sp38.unk58[0] = arg9;
    mod_func_8001A1D4(temp_s1, &sp38, 2);
    flags &= 0xF;
    if (!(flags & 0xC)) {
        if (animState->unk62[1] & 1) {
            flags |= 0x10;
        }
        if (animState->unk62[1] & 4) {
            flags |= 0x20;
        }
    }

    //@recomp: use extended tilt list
    func_8001B4F0(&spA4, arg0, &sp38, temp_s1->joints, (s32) temp_s1->jointCount, rsExtendedTiltList, (s32) arg4, flags);
}

// Use an extended seqJoint/tiltList array, to avoid potential overflow during the last shot of the Rolling Demo when too many characters are using seqJoints.
RECOMP_PATCH void mod_func_8001A640(Object* object, ModelInstance* modelInst, Model* model) {
    s8* amap;
    AnimState* animState0;
    ObjDef* def;
    s32 jointOffset;
    s16* seqJoint;
    s32 seqJointDefPos;
    s32 i;
    s32 tiltListIdx;
    u8 jointID;

    tiltListIdx = 0;
    if (model->unk71 & 0x40) {
        animState0 = modelInst->animState0;
        amap = (s8*) animState0->anims[animState0->animIndexes[0]];
    } else {
        amap = (s8*) &model->amap[modelInst->animState0->animIndexes[0] * ALIGN8(model->jointCount - 1)];
    }

    def = object->def;
    seqJointDefPos = 0;

    tiltListIdx = 0;
    for (i = 0; i < def->numSequenceBones; i++) {
        jointID = def->pSequenceBones[seqJointDefPos + 1 + object->modelInstIdx];
        if (jointID != 0xFF) {
            seqJoint = object->unk6C[i];
            jointOffset = amap[jointID] << 6;

            // pitch
            if (seqJoint[0] != 0) {
                rsExtendedTiltList[tiltListIdx++] = jointOffset;
                rsExtendedTiltList[tiltListIdx++] = seqJoint[0];
            }

            // yaw
            if (seqJoint[1] != 0) {
                rsExtendedTiltList[tiltListIdx++] = jointOffset + 2;
                rsExtendedTiltList[tiltListIdx++] = seqJoint[1];
            }

            // roll
            if (seqJoint[2] != 0) {
                rsExtendedTiltList[tiltListIdx++] = jointOffset + 4;
                rsExtendedTiltList[tiltListIdx++] = seqJoint[2];
            }

            // scaleX
            if (seqJoint[3] != 0) {
                rsExtendedTiltList[tiltListIdx++] = jointOffset + 0xC;
                rsExtendedTiltList[tiltListIdx++] = seqJoint[3];
            }

            // scaleY
            if (seqJoint[4] != 0) {
                rsExtendedTiltList[tiltListIdx++] = jointOffset + 0xE;
                rsExtendedTiltList[tiltListIdx++] = seqJoint[4];
            }

            // scaleZ
            if (seqJoint[5] != 0) {
                rsExtendedTiltList[tiltListIdx++] = jointOffset + 0x10;
                rsExtendedTiltList[tiltListIdx++] = seqJoint[5];
            }

            // translateX
            if (seqJoint[6] != 0) {
                rsExtendedTiltList[tiltListIdx++] = jointOffset + 0x18;
                rsExtendedTiltList[tiltListIdx++] = seqJoint[6];
            }

            // translateY
            if (seqJoint[7] != 0) {
                rsExtendedTiltList[tiltListIdx++] = jointOffset + 0x1A;
                rsExtendedTiltList[tiltListIdx++] = seqJoint[7];
            }

            // translateZ
            if (seqJoint[8] != 0) {
                rsExtendedTiltList[tiltListIdx++] = jointOffset + 0x1C;
                rsExtendedTiltList[tiltListIdx++] = seqJoint[8];
            }
        }

        //Move to next seqJointDef
        seqJointDefPos += 1 + def->numModels;
    }

    //End tilt-list?
    rsExtendedTiltList[tiltListIdx] = 0x1000;

    //@recomp: compare against extended array, and use debug print
    if (tiltListIdx > ARRAYCOUNT_S(rsExtendedTiltList)) {
        diPrintf("Warning! Tiltlist overflow!!\n");
    }
}

/* Intercept model matrices just after animation, so edits can optionally be applied. */
RECOMP_PATCH void mod_func_80019730(ModelInstance* modelInst, Model* model, Object* obj, MtxF* mtxs) {
    AnimState* animState0;
    s32 sp60;
    s32 pad;
    AnimState* animState1;
    Vec3f sp4C;
    s16 sp44[3];

    mod_func_8001A640(obj, modelInst, model);
    modelInst->unk34 ^= 1;
    sp60 = modelInst->unk34 & 1;
    animState0 = modelInst->animState0;
    if (animState0->unk62[1] & 4) {
        mod_func_8001A3FC(modelInst, 0, 0, obj->animProgress, obj->srt.scale, &sp4C, sp44);
        D_800903DC = sp44[0];
        D_800903DE = sp44[1];
        D_800903E0 = sp44[2];
    }

    if (modelInst->model->unk71 & 8) {
        mod_func_800199A8(mtxs, modelInst, modelInst->animState0, obj->animProgress, 0x7F);
    } else if (modelInst->animState0->unk62[1] & 8) {
        animState1 = modelInst->animState1;
        mod_func_80019FC0(mtxs, modelInst, animState0, obj->animProgress, 0x7F, 0, 0, 2, 0x14, animState0->unk58[1]);
        mod_func_80019FC0(mtxs, modelInst, animState1, obj->animProgressLayered, 0x7F, 0, 0, 2, 0x18, animState1->unk58[1]);
        mod_func_80019FC0(mtxs, modelInst, animState0, obj->animProgress, 0x7F, 0, 0, 0, 7, animState1->unk58[0]);
        mod_func_80019FC0(mtxs, modelInst, animState0, obj->animProgress, 0x7F, 0, 1, 1, 1, animState0->unk58[0]);
    } else {
        mod_func_800199A8(mtxs, modelInst, modelInst->animState0, obj->animProgress, 0x7F);
        if ((modelInst->animState1 != NULL) && (obj->curModAnimIdLayered >= 0)) {
            mod_func_800199A8(mtxs, modelInst, modelInst->animState1, obj->animProgressLayered, -1U);
        }
    }

    //@recomp: edit matrices after animation is applied
    skeletonEditJointMatrices(modelInst, model, obj, mtxs);

    camAddMatrixToPool(modelInst->matrices[sp60], model->jointCount);
}
