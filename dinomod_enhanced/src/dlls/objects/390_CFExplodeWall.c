#include "common_objsetups.h"
#include "modding.h"
#include "object_util.h"
#include "recomputils.h"

#include "common.h"
#include "dlls/engine/6_amsfx.h"
// #include "dlls/objects/390_CFExplodeWall.h"
// #include "dlls/objects/404_CFExplodePieces.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/objtype.h"

#include "recomp/dlls/_asm/390_recomp.h"

//TEMPORARY DEFINES
typedef enum {
    CFExplodeWall_STATE_0_Waiting,
    CFExplodeWall_STATE_1_Exploding,
    CFExplodeWall_STATE_2_Finished
} CFExplodeWall_States;

#define CFExplodeWall_obj_Setup dll_390_obj_Setup
#define CFExplodeWall_obj_Control dll_390_obj_Control
#define CFExplodeWall_calculatePieceData dll_390_func_3E8
#define CFExplodeWall_explode dll_390_func_884
#define CFExplodeWall_createPiece dll_390_func_C60

#define dExplodeDefs data_2E4

typedef enum {
    CFExplodePieces_STATE_0_Stopped,
    CFExplodePieces_STATE_1_Moving,
    CFExplodePieces_STATE_2_Finished
} CFExplodePieces_States;

typedef enum {
    CFExplodePieces_FLAGS_4_Touching_Ground = 4
} CFExplodePieces_Flags;

DLL_INTERFACE(DLL_404_CFExplodePieces) {
    /*:*/ DLL_INTERFACE_BASE(DLL_IObject);
    /*7*/ CFExplodePieces_States (*GetState)(Object* self);
};

#define dll_CFExplodePieces(obj) (((DLL_404_CFExplodePieces*)obj->dll)->vtbl)

#define SOUND_5B5_Explosion_Debris_Crash 0x5B5
//END OF TEMPORARY DEFINES

#define MAX_PIECES 15

typedef struct {
    Object* unk0; //Unused
    Vec3f centrepoint;
    Vec3f positionOffset;
    f32 yawSpeed;
    f32 pitchSpeed;
    f32 rollSpeed;
    f32 yawAcceleration;
    f32 pitchAcceleration;
    f32 rollAcceleration;
    Vec3f acceleration;
    Vec3f velocity;
    Vec3f initialPosition;
    f32 floorOffset;
    s32 lifetimeMax;
    s32 fadeEndTime;
    s16 roll;
    s16 pitch;
    s16 yaw;
    u8 state;
    u8 unk6B; //Unused in practice, but possibly meant as opacity?
    u8 flags;
    u8 rotateFactor;
} CFExplodeWall_PieceData;

typedef struct {
    CFExplodeWall_PieceData pieceData[MAX_PIECES];
    Object* pieceObjs[MAX_PIECES];
    s32 piecesSoundBitfield;
    s32 explodeSoundID;
    u8 pieceCount;
    u8 piecesConfigured[MAX_PIECES];
    u8 state;
} CFExplodeWall_Data;

typedef struct {
    s32 objIDWhole;
    s32 objIDPieces;
    s32 soundID;
    u8 rotateSpeed;
} ExplodeObjDef;

/*0x2E4*/ extern ExplodeObjDef dExplodeDefs[11];

extern void CFExplodeWall_calculatePieceData(Object* self, CFExplodeWall_PieceData* piece, CFExplodeWall_CustomSetup* objSetup);
extern void CFExplodeWall_explode(Object* self, CFExplodeWall_CustomSetup* objSetup, s32 skipModelCentrepointCalc, CFExplodeWall_Data* objData);
extern Object* CFExplodeWall_createPiece(Object* self, s32 objectID, CFExplodeWall_PieceData* pieceData, s32 index);

/* Set finished state if either gamebit is set on load (TODO: apply throughout game? Only in Discovery Falls for now) */
RECOMP_PATCH void CFExplodeWall_obj_Setup(Object* self, CFExplodeWall_CustomSetup* objSetup, s32 reset) {
    CFExplodeWall_Data* objData;
    u32 i;

    objAddObjectType(self, OBJTYPE_ExplodeObj);
    
    objData = self->data;
    
    if (objSetup->pieceCount == 0) {
        objData->pieceCount = 1;
    } else {
        objData->pieceCount = objSetup->pieceCount;
    }
    
    objData->piecesSoundBitfield = 0;
    
    for (i = 0; i < ARRAYCOUNT(objData->pieceObjs); i++) {
        objData->pieceObjs[i] = NULL;
    }
    
    self->srt.yaw = objSetup->yaw;
    self->srt.pitch = objSetup->pitch;
    self->srt.roll = objSetup->roll;
    
    // @recomp: set finished state if either of the explosion bits are set, to avoid replaying the explosion if 
    // the CFExplodeWall unloads before the demolition animation fully finishes. This could happen in Discovery Falls
    // if you entered the Whirlpool Cave before the last approach wall (DFdebris3) finished collapsing!
    // TODO: edit only applied in Discovery Falls for now, need to check it's safe to apply throughout the game.
    if (self->mapID == MAP_DISCOVERY_FALLS) {
        if (GAMEBIT_SPECIFIED_AND_SET(objSetup->gamebitFinished) || //@recomp: check gamebit is actually specified
            GAMEBIT_SPECIFIED_AND_SET(objSetup->gamebitExplode)     //@recomp: either bit causes finished state on revisit
        ) {
            //@recomp: ensure both bits are set on revisit
            if (GAMEBIT_SPECIFIED(objSetup->gamebitExplode) && GAMEBIT_SPECIFIED(objSetup->gamebitFinished)) {
                if (mainGetBits(objSetup->gamebitExplode) == TRUE && mainGetBits(objSetup->gamebitFinished) == FALSE) {
                    mainSetBits(objSetup->gamebitFinished, TRUE);
                }
            }
            objData->state = CFExplodeWall_STATE_2_Finished;
        }
    } else {
        if (mainGetBits(objSetup->gamebitFinished)) {
            objData->state = CFExplodeWall_STATE_2_Finished;
        }
    }
}

RECOMP_PATCH void CFExplodeWall_obj_Control(Object* self) {
    u32 i;
    CFExplodeWall_CustomSetup* objSetup;
    CFExplodeWall_Data* objData;
    s32 pieceVal;
    Object* obj;

    objData = self->data;
    objSetup = (CFExplodeWall_CustomSetup*)self->setup;

    //Finished
    if (objData->state == CFExplodeWall_STATE_2_Finished) {
        return;
    }

    //Waiting to explode
    if (objData->state == CFExplodeWall_STATE_0_Waiting) {
        if (mainGetBits(objSetup->gamebitExplode)) {
            CFExplodeWall_explode(self, objSetup, FALSE, objData);

            //@recomp: optionally force a default explosion sound
            if (objSetup->options & CFExplodeWall_CUSTOMOPTION_Use_Default_Explosion_Sound) {
                dll_amSfx->Play(self, SOUND_860_Explosion_Mid, MAX_VOLUME, NULL, NULL, 0, NULL);
            } else {
                if (objData->explodeSoundID != NO_SOUND) {
                    dll_amSfx->Play(self, objData->explodeSoundID, MAX_VOLUME, NULL, NULL, 0, NULL);
                }
            }

            //@recomp: optional shake camera
            if (objSetup->options & CFExplodeWall_CUSTOMOPTION_Camera_Shake) {
                camUseShake();
                camSetShakeOffset(10.0f);
            }

            objData->state = CFExplodeWall_STATE_1_Exploding;
            self->opacity = 0;
        }
        return;
    }

    //Exploding
    for (i = 0; i < ARRAYCOUNT(objData->pieceObjs); i++) {
        obj = objData->pieceObjs[i];
        if (obj) {
            pieceVal = dll_CFExplodePieces(obj)->GetState(obj);
            
            switch (pieceVal) {
            case CFExplodePieces_STATE_2_Finished:
                //Free piece
                mainSetBits(objSetup->gamebitFinished, TRUE);
                objFreeObject(objData->pieceObjs[i]);
                objData->pieceObjs[i] = NULL;
                break;
            case CFExplodePieces_STATE_0_Stopped:
                //Play piece impact sound
                mainSetBits(objSetup->gamebitFinished, TRUE);
                if ((objData->piecesSoundBitfield & (1 << i)) == FALSE) {
                    if (objSetup->options & CFExplodeWall_CUSTOMOPTION_No_Falloff_On_Piece_Sounds) {
                        //@recomp: optionally use no falloff on piece impact sounds
                        dll_amSfx->Play(NULL, SOUND_5B5_Explosion_Debris_Crash, MAX_VOLUME, NULL, NULL, 0, NULL); 
                    } else if (objSetup->options & CFExplodeWall_CUSTOMOPTION_Piece_Plays_Impact_Sound) {
                        //@recomp: optionally play the impact sound from the piece itself
                        dll_amSfx->Play(obj, SOUND_5B5_Explosion_Debris_Crash, MAX_VOLUME, NULL, NULL, 0, NULL); 
                    } else {
                        dll_amSfx->Play(obj, SOUND_5B5_Explosion_Debris_Crash, MAX_VOLUME, NULL, NULL, 0, NULL); 
                    }
                    objData->piecesSoundBitfield |= 1 << i;
                }
                break;
            }
        }
    }
}

/* Break out of loop once the explodeDef is found, instead of continuing through. */
RECOMP_PATCH void CFExplodeWall_explode(Object* self, CFExplodeWall_CustomSetup* objSetup, s32 skipModelCentrepointCalc, CFExplodeWall_Data* objData) {
    #define PIECE (&objData->pieceData[i])
    s32 sumX;
    s32 sumY;
    s32 sumZ;
    Vtx* vertices;
    s32 i;
    s32 vtxIdx;
    s32 objectID;
    s16 count;
    u8 rotateFactor;
    ModelInstance* modelInstance;
    Model* model;
    CFExplodeWall_PieceData* piece;

    objectID = -1;
    rotateFactor = 1;

    for (i = 0; i < ARRAYCOUNT_S(dExplodeDefs); i++) {
        if (self->id == dExplodeDefs[i].objIDWhole) {
            objectID = dExplodeDefs[i].objIDPieces;
            objData->explodeSoundID = dExplodeDefs[i].soundID;
            rotateFactor = dExplodeDefs[i].rotateSpeed;
            break; //@recomp: break out after finding the matching objIDWhole
        }
    }
    
    if (objectID == -1) {
        STUBBED_PRINTF(" Warning : DefNumber Lists in Exploder DLL is not set up correctly for this object");
        return;
    }

    for (i = 0; i < objData->pieceCount; i++) {
        objData->piecesConfigured[i] = TRUE;
        PIECE->rotateFactor = rotateFactor;
        
        if (skipModelCentrepointCalc == FALSE) {
            PIECE->centrepoint.x = 0.0f;
            PIECE->centrepoint.y = 0.0f;
            PIECE->centrepoint.z = 0.0f;
            
            sumX = 0;
            sumY = 0;
            sumZ = 0;
            modelInstance = self->modelInsts[i];
            model = modelInstance->model;
            for (vtxIdx = 0; vtxIdx < model->vertexCount; vtxIdx++) {
                sumX += model->vertices[vtxIdx].v.ob[0];
                sumY += model->vertices[vtxIdx].v.ob[1];
                sumZ += model->vertices[vtxIdx].v.ob[2];
            }
            sumX /= model->vertexCount;
            sumY /= model->vertexCount;
            sumZ /= model->vertexCount;
            PIECE->centrepoint.x = sumX;
            PIECE->centrepoint.y = sumY;
            PIECE->centrepoint.z = sumZ;
        }
        
        PIECE->positionOffset.x = PIECE->centrepoint.x;
        PIECE->positionOffset.y = PIECE->centrepoint.y;
        PIECE->positionOffset.z = PIECE->centrepoint.z;

        CFExplodeWall_calculatePieceData(self, PIECE, objSetup);

        PIECE->unk6B = 0xFF;
        
        if (mainGetBits(objSetup->gamebitFinished)) {
            PIECE->state = CFExplodeWall_STATE_2_Finished;
        } else {
            PIECE->state = CFExplodeWall_STATE_0_Waiting;
        }
        
        objData->pieceObjs[i] = CFExplodeWall_createPiece(self, objectID, PIECE, i);
    }

    if (mainGetBits(objSetup->gamebitFinished)) {
        objData->state = CFExplodeWall_STATE_1_Exploding;
        return;
    }
    
    objData->state = CFExplodeWall_STATE_0_Waiting;
}
