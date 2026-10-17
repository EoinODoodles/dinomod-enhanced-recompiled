#include "modding.h"
#include "recomputils.h"
#include "common_objsetups.h"

#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/memory.h"
#include "sys/objprint.h"
#include "sys/print.h"

#include "recomp/dlls/_asm/356_recomp.h"

//TEMPORARY DEFINES
#define XYZAnimator_obj_Control dll_356_obj_Control
#define XYZAnimator_obj_Free dll_356_obj_Free
#define XYZAnimator_obj_GetDataSize dll_356_obj_GetDataSize
#define XYZAnimator_updateShapesAndVertices dll_356_func_1174
#define XYZAnimator_calculateAnimatedVertexDistances dll_356_func_17EC
#define XYZAnimator_getInitialValues dll_356_func_1A04
//END OF TEMPORARY DEFINES

// #define DEBUG_ANIMATOR

typedef struct {
    s32 animShapeCount;     //The number of animated shapes
    s32 animVtxCount;       //The number of animated vertices
    f32 radius;             //For `XYZAnimator_MODE_3_Radial_Falloff_Set_Gamebit`
    f32 innerRadius;        //For `XYZAnimator_MODE_3_Radial_Falloff_Set_Gamebit`
    f32 goalRadius;         //For `XYZAnimator_MODE_3_Radial_Falloff_Set_Gamebit`
    f32* distances;         //Lateral distance from the XYZAnimator to each vertex it animates (used for `XYZAnimator_MODE_3_Radial_Falloff_Set_Gamebit`)
    s16* vtxCoords;         //Block-local position of each animated vertex
    s16* bbYmin;            //Updated bounds for each animated shape
    s16* bbYmax;            //Updated bounds for each animated shape
    s16* bbXmin;            //Updated bounds for each animated shape
    s16* bbXmax;            //Updated bounds for each animated shape
    s16* bbZmin;            //Updated bounds for each animated shape
    s16* bbZmax;            //Updated bounds for each animated shape
    Vec3f displacement;     //Position delta
    s8 animatorID;          //The animator tag of the shapes to animate
    s8 goal;                //`XYZAnimator_Goals`
    s8 goalTimer;           //How many ticks the animator has spent at its goal
    s8 prevGoal;            //`XYZAnimator_Goals`
    /* RECOMP */
    u8 flags;
    u8 prevFlags;
} XYZAnimator_Data;

typedef enum {
    XYZAnimator_MODE_0_Oneshot_and_Set_Gamebit,     //Interpolate to the goal, and set gamebitFinished
    XYZAnimator_MODE_1_Oneshot,                     //Interpolate to the goal
    XYZAnimator_MODE_2_Reversible_Set_Gamebit,      //Interpolate to/from the goal, and set gamebitFinished
    XYZAnimator_MODE_3_Radial_Falloff_Set_Gamebit,  //Animate vertices within an expanding radius, and set gamebitFinished
    XYZAnimator_MODE_4_Oneshot_and_Set_Gamebit_No_Initial_Update //Interpolate to the goal, and set gamebitFinished (without updating the vertices during initialisation)
} XYZAnimator_Modes;

typedef enum {
    XYZAnimator_POSITION_Initial,
    XYZAnimator_POSITION_Goal
} XYZAnimator_Goals;

typedef enum {
    XYZAnimator_FLAG_Initialised = 1,   //Memory allocated and Block data retrieved
    XYZAnimator_FLAG_Block_Loaded = 2   //XYZAnimator's local Block is currently loaded
} XYZAnimator_Flags;

extern void XYZAnimator_updateShapesAndVertices(XYZAnimator_Data* objData, XYZAnimator_Setup* objSetup, Block* block);
extern void XYZAnimator_calculateAnimatedVertexDistances(Object* self, XYZAnimator_Data* objData);
extern void XYZAnimator_getInitialValues(Object* self, XYZAnimator_Data* objData);

RECOMP_PATCH void XYZAnimator_obj_Control(Object* self) {
    XYZAnimator_Setup* objSetup;
    XYZAnimator_Data* objData;
    Block* block;
    s32 delta;
    s32 numAtGoal;
    
    objSetup = (XYZAnimator_Setup*)self->setup;
    objData = self->data;
    
    //@recomp: save previous flags
    if (objData->flags & XYZAnimator_FLAG_Initialised) {
        objData->prevFlags = objData->flags;
    }

#ifdef DEBUG_ANIMATOR
    diPrintf("XYZAnimator %x (MODE %d)\n\tgoal: %d prevGoal: %d\n", objSetup->base.uID, objSetup->mode, objData->goal, objData->prevGoal);
#endif

    block = mapGetBlockByIndex(mapWorldCoordsToBlockIndex(self->srt.transl.x, self->srt.transl.y, self->srt.transl.z));
    if (block == NULL) {
        objData->goalTimer = 0;
        
        //@recomp: note that the Block isn't loaded
        objData->flags &= ~XYZAnimator_FLAG_Block_Loaded;

#ifdef DEBUG_ANIMATOR
        diPrintf("\tblock not loaded!\n");
#endif
        return;
    }

    //@recomp: note that the Block is loaded
    objData->flags |= XYZAnimator_FLAG_Block_Loaded;
    
    //@recomp: check if flags changed
    if ((objData->flags & XYZAnimator_FLAG_Initialised) && (objData->flags != objData->prevFlags)) {
        if (objSetup->options & XYZAnimator_OPTION_Reanimate_on_Block_Reload) {
            if ((objData->flags & XYZAnimator_FLAG_Block_Loaded) && !(objData->prevFlags & XYZAnimator_FLAG_Block_Loaded)) {
                XYZAnimator_updateShapesAndVertices(objData, objSetup, block);
                block->vtxFlags ^= 1;
                XYZAnimator_updateShapesAndVertices(objData, objSetup, block);
                block->vtxFlags ^= 1;
#ifdef DEBUG_ANIMATOR
                recomp_printf("XYZAnimator %x - RESTORED ANIMATION!\n", objSetup->base.uID);
#endif
            }
        }
    }

    if ((block->vtxFlags & 8) == FALSE) {
        return;
    }
    
    //Initialise XYZAnimator
    if (objData->animVtxCount == 0) {
        objData->animatorID = objSetup->animatorID;
        objData->animVtxCount = blockGetAnimatorVertexCount(self, objData->animatorID);
        objData->animShapeCount = blockGetAnimatorShapeCount(self, objData->animatorID);
        if (objData->animVtxCount == 0) {
            objData->animatorID = 0;
        }
        
        if (objData->animatorID == 0) {
            return;
        }
        
        objData->radius = 0.0f;
        objData->innerRadius = 0.0f;
        objData->goalRadius = objSetup->goalRadius;
        
        //Check if the animation should start
        if (objSetup->gamebitActivate == NO_GAMEBIT) {
            objData->goal = XYZAnimator_POSITION_Goal;
        } else {
            objData->goal = mainGetBits(objSetup->gamebitActivate);
        }
        
        objData->displacement.x = objSetup->initDisplacementX;
        objData->displacement.y = objSetup->initDisplacementY;
        objData->displacement.z = objSetup->initDisplacementZ;
        
        //Check if the animation should already be finished
        if (objSetup->gamebitFinished != NO_GAMEBIT && mainGetBits(objSetup->gamebitFinished)) {
            objData->displacement.x = objSetup->endDisplacementX;
            objData->displacement.y = objSetup->endDisplacementY;
            objData->displacement.z = objSetup->endDisplacementZ;
            objData->goal = XYZAnimator_POSITION_Goal;
            objData->radius = objData->goalRadius + 1;
        }
        
        //Allocate and set up the initial values
        {
            if (objSetup->mode == XYZAnimator_MODE_3_Radial_Falloff_Set_Gamebit) {
                objData->distances = mmAlloc(objData->animVtxCount * sizeof(f32), ALLOC_TAG_TRACK_COL, NULL);
                XYZAnimator_calculateAnimatedVertexDistances(self, objData);
            }
            
            objData->vtxCoords = mmAlloc((objData->animVtxCount * (sizeof(s16))) * 3, ALLOC_TAG_TRACK_COL, NULL);
            objData->bbYmin = mmAlloc(objData->animShapeCount * (sizeof(s16)), ALLOC_TAG_TRACK_COL, NULL);
            objData->bbYmax = mmAlloc(objData->animShapeCount * (sizeof(s16)), ALLOC_TAG_TRACK_COL, NULL);
            objData->bbXmin = mmAlloc(objData->animShapeCount * (sizeof(s16)), ALLOC_TAG_TRACK_COL, NULL);
            objData->bbXmax = mmAlloc(objData->animShapeCount * (sizeof(s16)), ALLOC_TAG_TRACK_COL, NULL);
            objData->bbZmin = mmAlloc(objData->animShapeCount * (sizeof(s16)), ALLOC_TAG_TRACK_COL, NULL);
            objData->bbZmax = mmAlloc(objData->animShapeCount * (sizeof(s16)), ALLOC_TAG_TRACK_COL, NULL);
            XYZAnimator_getInitialValues(self, objData);
        }

        //Update the vertices and vertex buffers
        if (objSetup->mode != XYZAnimator_MODE_4_Oneshot_and_Set_Gamebit_No_Initial_Update) {
            XYZAnimator_updateShapesAndVertices(objData, objSetup, block);
            block->vtxFlags ^= 1;
            XYZAnimator_updateShapesAndVertices(objData, objSetup, block);
            block->vtxFlags ^= 1;
        }

        //@recomp: note initialisation
        objData->flags |= XYZAnimator_FLAG_Initialised;
    }
    
    if (objData->animatorID == 0) {
        return;
    }
    
    //Check if the animation should start/reverse
    if (objSetup->mode == XYZAnimator_MODE_2_Reversible_Set_Gamebit) {
        objData->goal = mainGetBits(objSetup->gamebitActivate);
        if (objData->goalTimer >= 3) {
            if (objData->goal != objData->prevGoal) {
#ifdef DEBUG_ANIMATOR
                recomp_printf("XYZAnimator %x, goal: %d -> %d\n", objSetup->base.uID, objData->prevGoal, objData->goal);
#endif
                objData->goalTimer = 0;
                objData->prevGoal = objData->goal;
            }
        }
        
        if (objData->goalTimer >= 3) {
            return;
        }
    } else if (objData->goalTimer >= 3) {
        return;
    } else if (objData->goal == XYZAnimator_POSITION_Initial) {
        objData->goal = mainGetBits(objSetup->gamebitActivate);
        if (objData->goal == XYZAnimator_POSITION_Initial) {
            return;
        }
    }
    
    switch (objSetup->mode) {
    case XYZAnimator_MODE_0_Oneshot_and_Set_Gamebit:
    case XYZAnimator_MODE_4_Oneshot_and_Set_Gamebit_No_Initial_Update:
        //Animate towards the goal position
        numAtGoal = 0;
        if (objSetup->endDisplacementX < objSetup->initDisplacementX) {
            objData->displacement.x -= (objSetup->speedX * gUpdateRate) * 0.1f;
            if (objData->displacement.x <= objSetup->endDisplacementX) {
                objData->displacement.x = objSetup->endDisplacementX;
                numAtGoal++;
            }
        } else {
            objData->displacement.x += (objSetup->speedX * gUpdateRate) * 0.1f;
            if (objData->displacement.x >= objSetup->endDisplacementX) {
                objData->displacement.x = objSetup->endDisplacementX;
                numAtGoal++;
            }
        }
        
        if (objSetup->endDisplacementY < objSetup->initDisplacementY) {
            objData->displacement.y -= (objSetup->speedY * gUpdateRate) * 0.1f;
            if (objData->displacement.y <= objSetup->endDisplacementY) {
                objData->displacement.y = objSetup->endDisplacementY;
                numAtGoal++;
            }
        } else {
            objData->displacement.y += (objSetup->speedY * gUpdateRate) * 0.1f;
            if (objData->displacement.y >= objSetup->endDisplacementY) {
                objData->displacement.y = objSetup->endDisplacementY;
                numAtGoal++;
            }
        }
        
        if (objSetup->endDisplacementZ < objSetup->initDisplacementZ) {
            objData->displacement.z -= (objSetup->speedZ * gUpdateRate) * 0.1f;
            if (objData->displacement.z <= objSetup->endDisplacementZ) {
                objData->displacement.z = objSetup->endDisplacementZ;
                numAtGoal++;
            }
        } else {
            objData->displacement.z += (objSetup->speedZ * gUpdateRate) * 0.1f;
            if (objData->displacement.z >= objSetup->endDisplacementZ) {
                objData->displacement.z = objSetup->endDisplacementZ;
                numAtGoal++;
            }
        }
        
        //Check if all position components have reached their goal
        if (numAtGoal == 3) {
            if (objSetup->gamebitFinished != NO_GAMEBIT) {
                mainSetBits(objSetup->gamebitFinished, TRUE);
            }
            objData->goalTimer++;
        }
        break;
    case XYZAnimator_MODE_1_Oneshot:
        //Animate towards the goal position
        if (objSetup->endDisplacementX < objSetup->initDisplacementX) {
            objData->displacement.x -= (objSetup->speedX * gUpdateRate) * 0.1f;
            if (objData->displacement.x < objSetup->endDisplacementX) {
                delta = objSetup->endDisplacementX - objData->displacement.x;
                objData->displacement.x = objSetup->initDisplacementX - delta;
            }
        } else {
            objData->displacement.x += (objSetup->speedX * gUpdateRate) * 0.1f;
            if (objSetup->initDisplacementX < objData->displacement.x) {
                delta = objData->displacement.x - objSetup->endDisplacementX;
                objData->displacement.x = objSetup->endDisplacementX + delta;
            }
        }
        
        if (objSetup->endDisplacementY < objSetup->initDisplacementY) {
            objData->displacement.y -= (objSetup->speedY * gUpdateRate) * 0.1f;
            if (objData->displacement.y < objSetup->endDisplacementY) {
                delta = objSetup->endDisplacementY - objData->displacement.y;
                objData->displacement.y = objSetup->initDisplacementY - (delta * 0.1f);
            }
        } else {
            objData->displacement.y += (objSetup->speedY * gUpdateRate) * 0.1f;
            if (objSetup->initDisplacementY < objData->displacement.y) {
                delta = objData->displacement.y - objSetup->endDisplacementY;
                objData->displacement.y = objSetup->endDisplacementY + delta;
            }
        }
        
        if (objSetup->endDisplacementZ < objSetup->initDisplacementZ) {
            objData->displacement.z -= (objSetup->speedZ * gUpdateRate) * 0.1f;
            if (objData->displacement.z < objSetup->endDisplacementZ) {
                delta = objSetup->endDisplacementZ - objData->displacement.z;
                objData->displacement.z = objSetup->initDisplacementZ - delta;
            }
        } else {
            objData->displacement.z += (objSetup->speedZ * gUpdateRate) * 0.1f;
            if (objSetup->initDisplacementZ < objData->displacement.z) {
                delta = objData->displacement.z - objSetup->endDisplacementZ;
                if (((!objSetup->endDisplacementZ) && (!objSetup->endDisplacementZ)) && (!objSetup->endDisplacementZ)) { } //fake
                objData->displacement.z = objSetup->endDisplacementZ + delta;
            }
        }
        break;
    case XYZAnimator_MODE_2_Reversible_Set_Gamebit:
        numAtGoal = 0;

        if (objData->goal != XYZAnimator_POSITION_Initial) {
            //Animate towards the goal position
            if (objSetup->endDisplacementX < objSetup->initDisplacementX) {
                objData->displacement.x -= (objSetup->speedX * gUpdateRate) * 0.1f;
                if (objData->displacement.x <= objSetup->endDisplacementX) {
                    objData->displacement.x = objSetup->endDisplacementX;
                    numAtGoal++;
                }
            } else {
                objData->displacement.x += (objSetup->speedX * gUpdateRate) * 0.1f;
                if (objData->displacement.x >= objSetup->endDisplacementX) {
                    objData->displacement.x = objSetup->endDisplacementX;
                    numAtGoal++;
                }
            }
            
            if (objSetup->endDisplacementY < objSetup->initDisplacementY) {
                objData->displacement.y -= (objSetup->speedY * gUpdateRate) * 0.1f;
                if (objData->displacement.y <= objSetup->endDisplacementY) {
                    objData->displacement.y = objSetup->endDisplacementY;
                    numAtGoal++;
                }
            } else {
                objData->displacement.y += (objSetup->speedY * gUpdateRate) * 0.1f;
                if (objData->displacement.y >= objSetup->endDisplacementY) {
                    objData->displacement.y = objSetup->endDisplacementY;
                    numAtGoal++;
                }
            }
            
            if (objSetup->endDisplacementZ < objSetup->initDisplacementZ) {
                objData->displacement.z -= (objSetup->speedZ * gUpdateRate) * 0.1f;
                if (objData->displacement.z <= objSetup->endDisplacementZ) {
                    objData->displacement.z = objSetup->endDisplacementZ;
                    numAtGoal++;
                }
            } else {
                objData->displacement.z += (objSetup->speedZ * gUpdateRate) * 0.1f;
                if (objData->displacement.z >= objSetup->endDisplacementZ) {
                    objData->displacement.z = objSetup->endDisplacementZ;
                    numAtGoal++;
                }
            }
            
            //Check if all position components have reached their goal
            if (numAtGoal == 3) {
                if (objSetup->gamebitFinished != NO_GAMEBIT) {
                    mainSetBits(objSetup->gamebitFinished, TRUE);
                }
                objData->goalTimer++; 
            } 
        } else {
            //Animate towards the initial position
            if (objSetup->endDisplacementX < objSetup->initDisplacementX) { 
                objData->displacement.x += (objSetup->speedX * gUpdateRate) * 0.1f; 
                if (objData->displacement.x >= objSetup->initDisplacementX) { 
                    objData->displacement.x = objSetup->initDisplacementX; 
                    numAtGoal++; 
                } 
            } else {
                objData->displacement.x -= (objSetup->speedX * gUpdateRate) * 0.1f;
                if (objData->displacement.x <= objSetup->initDisplacementX) {
                    objData->displacement.x = objSetup->initDisplacementX;
                    numAtGoal++;
                }
            }
            
            if (objSetup->endDisplacementY < objSetup->initDisplacementY) {
                objData->displacement.y += (objSetup->speedY * gUpdateRate) * 0.1f;
                if (objData->displacement.y >= objSetup->initDisplacementY) {
                    objData->displacement.y = objSetup->initDisplacementY;
                    numAtGoal++;
                }
            } else {
                objData->displacement.y -= (objSetup->speedY * gUpdateRate) * 0.1f;
                if (objData->displacement.y <= objSetup->initDisplacementY) {
                    objData->displacement.y = objSetup->initDisplacementY;
                    numAtGoal++;
                }
            }
            
            if (objSetup->endDisplacementZ < objSetup->initDisplacementZ) {
                objData->displacement.z += (objSetup->speedZ * gUpdateRate) * 0.1f;
                if (objData->displacement.z >= objSetup->initDisplacementZ) {
                    objData->displacement.z = objSetup->initDisplacementZ;
                    numAtGoal++;
                }
            } else {
                objData->displacement.z -= (objSetup->speedZ * gUpdateRate) * 0.1f;
                if (objData->displacement.z <= objSetup->initDisplacementZ) {
                    objData->displacement.z = objSetup->initDisplacementZ;
                    numAtGoal++;
                }
            }
            
            //Check if all position components have reached their goal
            if (numAtGoal == 3) {
                if (objSetup->gamebitFinished != NO_GAMEBIT) {
                    mainSetBits(objSetup->gamebitFinished, TRUE);
                }
                objData->goalTimer++;
            }
        }
        break;
    case XYZAnimator_MODE_3_Radial_Falloff_Set_Gamebit:
        //Expand the radius towards its goal
        objData->radius += (objSetup->speedX * gUpdateRateF) * 0.1f;
        if (objData->radius > objData->goalRadius) {
            objData->radius = objData->goalRadius;
            mainSetBits(objSetup->gamebitFinished, TRUE);
            objData->goalTimer++;
        }
        objData->innerRadius = objData->radius - 50.0f;
        break;
    }
    
    XYZAnimator_updateShapesAndVertices(objData, objSetup, block);
}

RECOMP_PATCH void XYZAnimator_obj_Free(Object* self, s32 onlySelf) {
    s32 pad[2];
    XYZAnimator_Setup* objSetup;
    XYZAnimator_Data* objData;
    Block* block;

    objData = self->data;
    objSetup = (XYZAnimator_Setup*)self->setup;
    
    objData->displacement.x = 0;
    objData->displacement.y = 0;
    objData->displacement.z = 0;
    
    if (onlySelf == FALSE) {
        block = mapGetBlockByIndex(mapWorldCoordsToBlockIndex(self->srt.transl.x, self->srt.transl.y, self->srt.transl.z));
        if ((block != NULL) && (objData->animVtxCount != 0)) {
            //@bug: only updates one vertex buffer (should maybe use `block->vtxFlags ^= 1` and then update a second time)
            XYZAnimator_updateShapesAndVertices(objData, objSetup, block);
            
            //@recomp: update both buffers
            block->vtxFlags ^= 1;
            XYZAnimator_updateShapesAndVertices(objData, objSetup, block);
        }
    }
    
    if (objData->distances != NULL) {
        mmFree(objData->distances);
    }
    if (objData->vtxCoords != NULL) {
        mmFree(objData->vtxCoords);
    }
    if (objData->bbYmin != NULL) {
        mmFree(objData->bbYmin);
    }
    if (objData->bbYmax != NULL) {
        mmFree(objData->bbYmax);
    }
    if (objData->bbXmin != NULL) {
        mmFree(objData->bbXmin);
    }
    if (objData->bbXmax != NULL) {
        mmFree(objData->bbXmax);
    }
    if (objData->bbZmin != NULL) {
        mmFree(objData->bbZmin);
    }
    if (objData->bbZmax != NULL) {
        mmFree(objData->bbZmax);
    }
}

/* Extend objData */
RECOMP_PATCH u32 XYZAnimator_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(XYZAnimator_Data);
}

#ifdef DEBUG_ANIMATOR
RECOMP_HOOK_DLL(XYZAnimator_updateShapesAndVertices) void XYZAnimator_logUpdate(XYZAnimator_Data* objData, XYZAnimator_Setup* objSetup, Block* block) {
    recomp_printf("XYZAnimator %x: BLOCK UPDATED! goal: %d\n", objSetup->base.uID, objData->goal);
}
#endif
