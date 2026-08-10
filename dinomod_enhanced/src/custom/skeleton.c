#include "cheats.h"
#include "math_util.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

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

#define MAX_JOINTS 200                          //TODO: not sure whether the game has an exact max joint count, but it can't be beyond 255 anyway
static u8 rsJointChildCounts[MAX_JOINTS];       //How many children each jointID has
static u8 rsJointMappedChildCounts[MAX_JOINTS]; //How many of each joint's child jointIDs have been mapped
static u8* rsJointChildIDs[MAX_JOINTS];         //Pointers to each joint's array of child jointIDs
static u8 rsJointChildData[MAX_JOINTS*4];       //A buffer for mapping the parent->child relationships in a model's joint hierarchy

typedef enum {
    ScaleModelJoint_MODE_0_Default, //Scales specified joints and their descendants
    ScaleModelJoint_MODE_1_Scale_Specified_and_Translate_Children,   //Only edits specified joints' scales, but updates translation of child joints
    ScaleModelJoint_MODE_2_Scale_Specified_but_No_Child_Translation, //Only edits specified joints' scales, without updating translation or scale of child joints
} ScaleModelJoint_Modes;

/* Returns the model jointID associated with a particular seqJointID */
static s16 skeletonGetModelJointIDFromSeqJointID(Object* obj, s32 seqJointID) {
    ObjDef* romdef;
    u8* seqBones;
    s32 index;
    s32 listPosition;
    u32 jointID;
    s16 modelJointID;

    romdef = obj->def;
    modelJointID = -1;

    if (romdef) {
        listPosition = 0;
        for (index = 0; index < romdef->numSequenceBones; index++){
            jointID = romdef->pSequenceBones[(listPosition + 1) + obj->modelInstIdx];
            if ((jointID != 0xFF) && (seqJointID == romdef->pSequenceBones[listPosition])) {
                modelJointID = jointID;
                break;
            }

            listPosition += 1 + romdef->numModels;
        }
    }
    
    return modelJointID;
}

/**
  * DP's model format defines the joint hierachy by specifying each joint's parent jointID.
  * As far as I know (I could easily have missed something though!), it doesn't store information
  * about the opposite relationship: each parent joint's child jointIDs. 
  *
  * This function extracts this info, by creating child jointID arrays for each joint in the model, 
  * as well as storing each joint's child joint count.
  */
static void skeletonMapChildJointIDs(Model* model) {
    u32 i;
    u32 c;
    u32 offset;
    u32 parentJointID;
    u32 childJointArrayIdx;

    //Clear the parent joint info buffers
    {
        bzero(rsJointChildCounts, sizeof(rsJointChildCounts));
        bzero(rsJointMappedChildCounts, sizeof(rsJointMappedChildCounts));
        bzero(rsJointChildIDs, sizeof(rsJointChildIDs));
        bzero(rsJointChildData, sizeof(rsJointChildData));
    }

    //Find out how many children each joint has
    for (i = 0; i < model->jointCount; i++) {
        parentJointID = model->joints[i].parentJointID;
        if (parentJointID >= 0) {
            rsJointChildCounts[parentJointID]++;
        }
    }

    //Store a pointer to each joint's list of child jointIDs
    for (i = 0, offset = 0; i < model->jointCount; i++) {
        rsJointChildIDs[i] = &rsJointChildData[offset];
        offset += rsJointChildCounts[i];
    }
    
    //Store each joint's child jointIDs
    for (i = 0; i < model->jointCount; i++) {
        parentJointID = model->joints[i].parentJointID;

        if ((parentJointID >= 0) && (parentJointID < model->jointCount)) {
            childJointArrayIdx = rsJointMappedChildCounts[parentJointID];
            if (childJointArrayIdx < rsJointChildCounts[parentJointID]) {
                rsJointChildIDs[parentJointID][childJointArrayIdx] = i;
                rsJointMappedChildCounts[parentJointID]++;
            }
        }
    }

#ifdef DEBUG
    //Print the child joint info
    for (i = 0; i < model->jointCount; i++) {
        recomp_printf("\n[joint %d] %d children: ", i, rsJointChildCounts[i]);

        for (c = 0; c < rsJointChildCounts[i]; c++) {
            recomp_printf("%d, ", rsJointChildIDs[i][c]);
        }
    } 
#endif
}

/* Gets the parent jointID of a specified model joint (returns -1 if the joint has no parent, or the jointID was out of bounds) */
static s32 skeletonGetParentJointID(Model* model, u8 jointID) {
    u32 i;
    u32 c;
    u32 offset;
    u32 parentJointID;
    u32 childJointArrayIdx;

    if ((jointID > model->jointCount) || (jointID < 0)) {
        return -1;
    }

    return model->joints[jointID].parentJointID;
}

/* Returns a pointer to a specific jointID's transformation matrix, on a ModelInstance's currently-used model. */
static MtxF* skeletonGetModelJointMatrix(ModelInstance* modelInst, u32 jointID) {
    if (modelInst == NULL) {
        return NULL;
    }
    if ((modelInst->model == NULL) || (jointID >= modelInst->model->jointCount)) {
        return NULL;
    }

    return (MtxF*) &((f32*)modelInst->matrices[modelInst->unk34 & 1])[jointID << 4];
}

/* Edits a joint's transformation matrix, applying a uniform scaling factor. */
static void skeletonScaleModelJoint(ModelInstance* modelInst, s32 jointID, f32 scaleFactor) {
    MtxF* jointMtx = skeletonGetModelJointMatrix(modelInst, jointID);
    if (jointMtx == NULL) {
        return;
    }

    //Edit the joint's scale
    for (u32 i = 0; i < 3; i++) {
        for (u32 j = 0; j < 4; j++) {
            jointMtx->m[i][j] *= scaleFactor;
        }
    }
}

/* Extracts a joint's worldSpace translation from its transformation matrix. */
static void skeletonGetJointTranslation(ModelInstance* modelInst, s32 jointID, Vec3f* oTranslation) {
    MtxF* jointMtx = skeletonGetModelJointMatrix(modelInst, jointID);
    if (jointMtx == NULL) {
        return;
    }

    oTranslation->x = jointMtx->m[3][0];
    oTranslation->y = jointMtx->m[3][1];
    oTranslation->z = jointMtx->m[3][2];
}

/* Sets a joint's worldSpace translation by editing its transformation matrix. */
static void skeletonSetJointTranslation(ModelInstance* modelInst, s32 jointID, Vec3f* translation) {
    MtxF* jointMtx = skeletonGetModelJointMatrix(modelInst, jointID);
    if (jointMtx == NULL) {
        return;
    }

    jointMtx->m[3][0] = translation->x;
    jointMtx->m[3][1] = translation->y;
    jointMtx->m[3][2] = translation->z;
}

/* Shifts a joint's worldSpace translation by adding a displacement vector to its transformation matrix. */
static void skeletonShiftJointTranslation(ModelInstance* modelInst, s32 jointID, Vec3f* delta) {
    MtxF* jointMtx = skeletonGetModelJointMatrix(modelInst, jointID);
    if (jointMtx == NULL) {
        return;
    }

    jointMtx->m[3][0] += delta->x;
    jointMtx->m[3][1] += delta->y;
    jointMtx->m[3][2] += delta->z;
}

/* Edits a joint's worldSpace translation, by scaling up the position difference between it and its parent joint. */
static void skeletonScaleChildJointTranslation(ModelInstance* modelInst, s32 jointID, Vec3f* parentWSPosition, f32 scaleFactor, Vec3f* scaledJointPositionDelta) {
    Vec3f pInitial;
    Vec3f d;

    //Store the child joint's initial position
    skeletonGetJointTranslation(modelInst, jointID, &pInitial);

    // Get the position delta between the child joint and its parent
    d.x = pInitial.x - parentWSPosition->x;
    d.y = pInitial.y - parentWSPosition->y;
    d.z = pInitial.z - parentWSPosition->z;

    //Scale the vector
    VECTOR_MULTIPLY_BY_SCALAR(d, scaleFactor);

    //Convert the new position delta back into worldSpace
    d.x += parentWSPosition->x;
    d.y += parentWSPosition->y;
    d.z += parentWSPosition->z;

    //Reapply the position
    skeletonSetJointTranslation(modelInst, jointID, &d);

    //Compare the child joint's position before/after scaling
    if (scaledJointPositionDelta != NULL) {
        scaledJointPositionDelta->x = d.x - pInitial.x;
        scaledJointPositionDelta->y = d.y - pInitial.y;
        scaledJointPositionDelta->z = d.z - pInitial.z;
    }
}

/* Checks whether a jointID is included in an array of jointIDs that shouldn't be edited. */
static _Bool skeletonShouldJointBeIgnored(u32 jointID, s16* ignoreJoints, u32 numIgnoredJoints) {
    for (u32 j = 0; j < numIgnoredJoints; j++) {
        if ((ignoreJoints[j] >= 0) && ((u32)ignoreJoints[j] == jointID)) {
            return TRUE;
        }
    }

    return FALSE;
}

/** 
  * Applies a position delta to a joint and all of its descendants 
  * (branches stop early if they include a jointID that should be ignored). 
  */
static void skeletonShiftJointAndDescendants(ModelInstance* modelInst, s32 parentJointID, s16* ignoreJoints, u32 numIgnoredJoints, Vec3f* delta) {
    u32 childJointID;

    for (int i = 0; i < rsJointChildCounts[parentJointID]; i++) {
        childJointID = rsJointChildIDs[parentJointID][i];

        if (skeletonShouldJointBeIgnored(childJointID, ignoreJoints, numIgnoredJoints)) {
            continue;
        }

        skeletonShiftJointTranslation(modelInst, childJointID, delta);

        //Shift descendants recursively
        skeletonShiftJointAndDescendants(modelInst, childJointID, ignoreJoints, numIgnoredJoints, delta);
    }
}

/** 
  * Scales up a specific jointID, transforming all its descendant joints too 
  * (branches stop early if they include a jointID that should be ignored). 
  */
static void skeletonScaleJointAndDescendants(ModelInstance* modelInst, s32 parentJointID, f32 scaleFactor, s16* ignoreJoints, u32 numIgnoredJoints, ScaleModelJoint_Modes ignoreDescendantsMode) {
    u32 childJointID;
    u32 childCount;
    u32 i;
    u32 j;
    u32 ignored;
    Vec3f parentPosition;
    Vec3f scaledJointPositionDelta;

    skeletonScaleModelJoint(modelInst, parentJointID, scaleFactor);

    // Finish here if the joint doesn't have any children
    if (rsJointChildCounts[parentJointID] == 0) {
        return;
    }

    //Skip scaling descendants if needed (when there're lots of branches, like Balloon Baddie)
    if (ignoreDescendantsMode == ScaleModelJoint_MODE_2_Scale_Specified_but_No_Child_Translation) {
        return;
    }

    skeletonGetJointTranslation(modelInst, parentJointID, &parentPosition);

    for (i = 0; i < rsJointChildCounts[parentJointID]; i++) {
        childJointID = rsJointChildIDs[parentJointID][i];

        if (skeletonShouldJointBeIgnored(childJointID, ignoreJoints, numIgnoredJoints)) {
            continue;
        }

        //Adjust the length of the bone, and store the difference in the joint's position before/after scaling
        skeletonScaleChildJointTranslation(modelInst, childJointID, &parentPosition, scaleFactor, &scaledJointPositionDelta);
        
        //Add the joint-length-scaling's resultant position delta to all the child joint's subsequent children
        skeletonShiftJointAndDescendants(modelInst, childJointID, ignoreJoints, numIgnoredJoints, &scaledJointPositionDelta);

        //If the "only scale specified joints, but do translate children" mode is enabled, don't scale descendant joints
        if (ignoreDescendantsMode == ScaleModelJoint_MODE_1_Scale_Specified_and_Translate_Children) {
            continue;
        }

        //Scale the child joint and its children recursively
        skeletonScaleJointAndDescendants(modelInst, childJointID, scaleFactor, ignoreJoints, numIgnoredJoints, ignoreDescendantsMode);
    }
}

/* Edits a model's joint matrices, resizing a character's head. */
static void skeletonScaleHead(ModelInstance* modelInst, Model* model, Object* obj, MtxF* mtxs) {
    s16 modelJointIDs[8];
    f32 scaleFactors[8];
    s16 ignoreJointIDs[8];
    u8 numSpecifiedJoints = 1;
    u8 numIgnoredJoints = 0;
    u8 i;
    ScaleModelJoint_Modes ignoreDescendantsMode = ScaleModelJoint_MODE_0_Default;
    u32 setting;
    s32 parentJointID;

    //Initialise parameters
    for (i = 0; i < ARRAYCOUNT(modelJointIDs); i++) {
        modelJointIDs[i] = 0;
        scaleFactors[i] = 1.0f;
        ignoreJointIDs[i] = -1;
    }

    //Check cheats
    if (gDLL_29_Gplay->vtbl->is_cheat_active(CHEAT_Giant_Head)) {
        setting = CHEAT_Giant_Head;
        scaleFactors[0] = 2.0f;
    } else if (gDLL_29_Gplay->vtbl->is_cheat_active(CHEAT_Big_Head)) {
        setting = CHEAT_Big_Head;
        scaleFactors[0] = 1.5f;
    } else if (gDLL_29_Gplay->vtbl->is_cheat_active(CHEAT_Small_Head)) {
        setting = CHEAT_Small_Head;
        scaleFactors[0] = 0.75f;
    } else if (gDLL_29_Gplay->vtbl->is_cheat_active(CHEAT_Tiny_Head)) {
        setting = CHEAT_Tiny_Head;
        scaleFactors[0] = 0.5f;
    } else {
        return;
    }

    //Do nothing if the default scale is being used
    if (scaleFactors[0] == 1.0f) {
        return;
    }

    //Get jointID of model's head (and handle special cases, like characters that don't have a head seqJoint defined)
    switch (obj->id) { 
        case OBJ_Krystal:
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);

            if (setting == CHEAT_Small_Head || setting == CHEAT_Tiny_Head) {
                parentJointID = skeletonGetParentJointID(model, modelJointIDs[0]);
                if (parentJointID >= 0) {
                    modelJointIDs[0] = parentJointID;
                }
            }
            break;
        case OBJ_Sabre:
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            break;
        case OBJ_Tricky:
            //Has a head seqJoint defined, but we need to avoid his neck tweak joint sticking out awkwardly
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            ignoreJointIDs[0] = 13;
            numIgnoredJoints = 1;
            scaleFactors[0] = lerp_float(1.5f, 1.0f, scaleFactors[0]);
            break;
        case OBJ_Kyte:
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            scaleFactors[0] = lerp_float(2.5f, 1.0f, scaleFactors[0]);
            break;
        case OBJ_SHswapstone:
        case OBJ_GPSHswapstone:
            //Reduce scale factor, since this character has a gigantic head already!
            scaleFactors[0] = lerp_float(0.4f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = 4; //GPSHswapstone doesn't have seqJoints defined, so specify here
            break;
        case OBJ_DR_EarthWarrior:
        case OBJ_WL_deaddino:
            //Reduce scale factor, since this character has a big head already
            scaleFactors[0] = lerp_float(0.6f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            break;
        case OBJ_SHqueenearthwal:
        case OBJ_WCQueenEarthWal:
        case OBJ_AnimQueenEarthW:
        case OBJ_WCKingEarthWalk:
        case OBJ_AnimKingEarthWa:
            //Reduce scale factor, since this character has a big head already
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            scaleFactors[0] = lerp_float(0.75f, 1.0f, scaleFactors[0]);
            break;
        case OBJ_DR_CloudRunner:
        case OBJ_SB_Cloudrunner:
        case OBJ_SB_CloudrunnerA:
        case OBJ_SB_AnimCR:
            modelJointIDs[0] = 14;
            break;
        case OBJ_Mammoth:
        case OBJ_NWmammothhelp:
        case OBJ_NWmammothsquirt:
        case OBJ_NWmammothwalk:
        case OBJ_NWmammothguardi:
        case OBJ_NWguardiandaugh:
        case OBJ_DIMSnowHorn1:
        case OBJ_DIM2Mammoth:
        case OBJ_DIM2MammothWhee:
            //Reduce scale factor, since these characters have a big head already!
            scaleFactors[0] = lerp_float(0.6f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = 5; //OBJ_DIM2MammothWhee doesn't have seqJoints defined, so specify here
            break;
        case OBJ_DBPointMum:
            //Has a head seqJoint defined, but scaling it looks a bit odd: picking another!
            modelJointIDs[0] = 6;
            break;
        case OBJ_CChightop:
        case OBJ_DFanimhightop:
            //Has a head seqJoint defined, but we need to avoid his neck tweak joint sticking out awkwardly
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            ignoreJointIDs[0] = 19;
            numIgnoredJoints = 1;
            break;
        case OBJ_SC_animbabyligh:
        case OBJ_SC_animchieflig:
        case OBJ_SC_animlightfoo:
        case OBJ_SC_babylightfoo:
        case OBJ_SC_chieflightfo:
        case OBJ_SC_lightfoot:
        case OBJ_SC_lightfootSpe:
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            scaleFactors[0] = lerp_float(1.5f, 1.0f, scaleFactors[0]);

            if (setting == CHEAT_Small_Head || setting == CHEAT_Tiny_Head) {
                parentJointID = skeletonGetParentJointID(model, modelJointIDs[0]);
                if (parentJointID >= 0) {
                    modelJointIDs[0] = parentJointID;
                }
            }
            break;
        case OBJ_SC_musclelightf:
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            scaleFactors[0] = lerp_float(2.0f, 1.0f, scaleFactors[0]);
            break;
        case OBJ_AnimShadowHunte:
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            scaleFactors[0] = lerp_float(1.5f, 1.0f, scaleFactors[0]);
            break;
        case OBJ_DFmole:
            modelJointIDs[0] = 3;
            scaleFactors[0] = lerp_float(1.5f, 1.0f, scaleFactors[0]);
            break;
        case OBJ_SPShopKeeper:
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            scaleFactors[0] = lerp_float(1.25f, 1.0f, scaleFactors[0]);
            break;
        case OBJ_Krazoa:
        case OBJ_WMKrazoa1:
        case OBJ_WMKrazoa2:
        case OBJ_MMP_Krazoa:
        case OBJ_KP_AnimKrazoa:
        case OBJ_KP_RedKrazoa:
            modelJointIDs[0] = 19;
            break;
        case OBJ_mushroomAnim:
        case OBJ_SHbluemushroom:
        case OBJ_SHkillermushroo:
        case OBJ_SPRedMushroom:
        case OBJ_foodbagBlueMush:
        case OBJ_foodbagRedMushr:
        case OBJ_foodbagOldMushr:
            modelJointIDs[0] = 3;
            break;
        case OBJ_grubAnim:
        case OBJ_CCgrubBlue:
        case OBJ_CCgrubRed:
        case OBJ_SPRedMaggot:
        case OBJ_foodbagBlueGrub:
        case OBJ_foodbagRedGrub:
        case OBJ_foodbagOldGrub:
            scaleFactors[0] = lerp_float(0.75f, 1.0f, scaleFactors[0]);
            scaleFactors[1] = scaleFactors[0];
            modelJointIDs[0] = 5;
            modelJointIDs[1] = 6;
            numSpecifiedJoints = 2;
            break;
        case OBJ_BalloonBaddie:
            scaleFactors[0] = lerp_float(0.4f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = 0;
            ignoreDescendantsMode = ScaleModelJoint_MODE_2_Scale_Specified_but_No_Child_Translation;
            break;
        case OBJ_SnowWormLarge:
        case OBJ_SnowWormMed:
        case OBJ_CCsandwormBoss:
        case OBJ_SandSnakes:
        case OBJ_SandEels:
            modelJointIDs[0] = 7;
            break;
        case OBJ_SnowWormSmall:
            modelJointIDs[0] = 7;
            scaleFactors[0] = lerp_float(1.5f, 1.0f, scaleFactors[0]);
            break;
        case OBJ_Scorpion:
            scaleFactors[0] = lerp_float(0.75f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = 0;
            ignoreDescendantsMode = ScaleModelJoint_MODE_2_Scale_Specified_but_No_Child_Translation;
            break;
        case OBJ_RopeBaddie:
            modelJointIDs[0] = 7;
            break;
        case OBJ_WaterBaddie:
            //The shell is a descendant of the head (the head's the root), so it needs to be ignored
            scaleFactors[0] = lerp_float(0.7f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = 0;
            ignoreJointIDs[0] = 2;
            numIgnoredJoints = 1;
            break;
        case OBJ_VampireBat:
            modelJointIDs[0] = 0;

            //Ignore wings (they descend from the head/body)
            ignoreJointIDs[0] = 1;
            ignoreJointIDs[1] = 4;
            numIgnoredJoints = 2;
            break;
        case OBJ_Lunaimar:
            //Reduce scale factor, since this character has a fairly big head already!
            scaleFactors[0] = lerp_float(0.6f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = 1;

            //Ignore arms (they descend from the head)
            ignoreJointIDs[0] = 4;
            ignoreJointIDs[1] = 14;
            numIgnoredJoints = 2;
            break;
        case OBJ_GP_ChimneySwipe:
            // scaleFactors[0] = lerp_float(0.75f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = 0;
            modelJointIDs[1] = 1;
            numSpecifiedJoints = 2;

            ignoreJointIDs[0] = 3;
            ignoreJointIDs[1] = 5;
            ignoreJointIDs[2] = 11;
            ignoreJointIDs[3] = 13;
            ignoreJointIDs[4] = 19;
            ignoreJointIDs[5] = 25;
            ignoreJointIDs[6] = 27;
            ignoreJointIDs[7] = 33;
            numIgnoredJoints = 8;

            ignoreDescendantsMode = ScaleModelJoint_MODE_1_Scale_Specified_and_Translate_Children;
            break;
        case OBJ_Caictua:
            scaleFactors[0] = lerp_float(0.5f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = 2;
            modelJointIDs[1] = 3;
            numSpecifiedJoints = 2;
            break;
        case OBJ_fish:
        case OBJ_fishAnim:
        case OBJ_curveFish:
        case OBJ_colourfullfish:
            //Use quite subtle scaling, so the fin doesn't detach too badly from the body
            scaleFactors[0] = lerp_float(0.15f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = 1;
            modelJointIDs[1] = 4;
            numSpecifiedJoints = 2;

            //Scale the tail the opposite way, to make the difference more noticeable
            if (scaleFactors[0] != 0) {
                scaleFactors[1] = 1/scaleFactors[0];
            }
            break;
        case OBJ_Skeetla:
        case OBJ_WM_WallCrawler:
        case OBJ_DR_BigSkeetla:
        case OBJ_DR_SmallSkeetla:
            modelJointIDs[0] = 1;
            scaleFactors[0] = lerp_float(2.0f, 1.0f, scaleFactors[0]);
            break;
        case OBJ_Nurse:
            modelJointIDs[0] = 0;
            modelJointIDs[1] = 1;
            numSpecifiedJoints = 2;

            ignoreJointIDs[0] = 3;
            ignoreJointIDs[1] = 7;
            ignoreJointIDs[2] = 11;
            ignoreJointIDs[3] = 15;
            ignoreJointIDs[4] = 19;
            ignoreJointIDs[5] = 23;
            numIgnoredJoints = 6;
            break;
        case OBJ_SB_ShipHead:
            modelJointIDs[0] = 2;
            break;
        case OBJ_CFCheapGalleon:
            modelJointIDs[0] = 1;
            break;
        case OBJ_SB_ShipGun:
        case OBJ_SB_KyteCage:
            return;
        case OBJ_DR_TrackLever:
            return;
        case OBJ_DIM_Boss:
            scaleFactors[0] = lerp_float(0.75f, 1.0f, scaleFactors[0]);
            scaleFactors[1] = lerp_float(0.75f, 1.0f, scaleFactors[0]);
            modelJointIDs[0] = 8;
            modelJointIDs[1] = 10;
            numSpecifiedJoints = 2;
            break;
        case OBJ_KT_Rex:
            modelJointIDs[0] = 31;
            scaleFactors[0] = lerp_float(0.75f, 1.0f, scaleFactors[0]);
            break;
        case OBJ_AnimDrakor:
        case OBJ_BossDrakor:
            modelJointIDs[0] = 6;
            scaleFactors[0] = lerp_float(2.0f, 1.0f, scaleFactors[0]);
            break;
        default:
            modelJointIDs[0] = skeletonGetModelJointIDFromSeqJointID(obj, 0);
            break;
    }
    if (modelJointIDs[0] < 0) {
        return;
    }

    //Figure out which jointIDs are descendants of the head
    /* TODO: store this info somewhere so it persists while a Model is referenced/loaded,
       avoiding having to recalculate it every time this runs. Maybe allocate it for each
       loaded model, but only if its joints actually require editing (i.e. calculate when
       Big Heads code attempts to edit the model, if not yet calculated) and deallocate it
       when the model's being unloaded? */
    skeletonMapChildJointIDs(model); 

    //Transform the joints' matrices
    /* 
        It'd be more efficient to do this as the joint matrices are being calculated,
        but that'd mean diving into the scary scary world of the hASM model animation code.
        So instead, let's make do and edit the joint matrices after they've been calculated!

        Note that the game's SeqJoints system unfortunately can't be used as an easier
        alternative. It does mostly work, but there's a bug/constraint where a SeqJoint's scale
        briefly gets ignored/goes haywire when animState0 and animState1 both have animation 
        blending active. This causes the player's head scale to flicker back to its 
        usual scale while Z-target strafing or riding the speeder bike, for example. 
        So with that method ruled out, I'm going with the matrix maths approach instead!
    */
    for (i = 0; i < numSpecifiedJoints; i++) {
        skeletonScaleJointAndDescendants(modelInst, 
            modelJointIDs[i], 
            scaleFactors[i], 
            ignoreJointIDs, numIgnoredJoints, 
            ignoreDescendantsMode
        );
    }
}

/** 
  * Used to intercept a model's joint matrices immediately after their animation pose has been calculated, 
  * so that tweaks can be applied. Used for Big Head / Giant Head cheats!
  */
void skeletonEditJointMatrices(ModelInstance* modelInst, Model* model, Object* obj, MtxF* mtxs) {
    skeletonScaleHead(modelInst, model, obj, mtxs);

    //NOTE: More edits could be added here
}
