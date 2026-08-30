#include "math_util.h"
#include "recomputils.h"

#include "game/objects/object.h"
#include "macros.h"
#include "sys/map.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objprint.h"
#include "sys/print.h"

#include "core/map.h"
#include "core/print.h"
#include "custom_object_ids.h"
#include "custom/dlls/LODAnimator.h"

// #define DEBUG_ANIMATOR
// #define DEBUG_UNLOAD
// #define DEBUG_VIS_GRID

/*
    TODO: Maybe consider adding these?

    - Optional activator gamebit
    - Optional gamebit that gets set when the nearby block is missing/found
    - A mode where the LOD fades out when the nearby block is found, instead of vanishing immediately
*/

typedef struct {
    Block* ownBlock;        //The LODAnimator's local BLOCKS model (which contains some LOD stand-in shapes for another nearby Block)
    u8 flags;
    u8 prevFlags;
    s8 nearbyBlockIndex;    //The loadedBlockIdx of the nearby Block that needs an LOD stand-in
    s8 ownBlockIndex;       //The loadedBlockIdx of the LODAnimator's local Block (which contains extra LOD shapes)
} LODAnimator_Data;

typedef enum {
    LODAnimator_FLAG_1_Nearby_Block_Found = 1
} LODAnimator_Flags;

static s32 LODAnimator_UpdateShapes(Object* self, s32 showLOD);
static void LODAnimator_CheckIfNearbyBlockAppeared(Object* self);

void LODAnimator_ctor(void* dll){ }

void LODAnimator_dtor(void* dll){ }

// export: 0
void LODAnimator_obj_Setup(Object* self, LODAnimator_Setup* objSetup, s32 reset) {
    LODAnimator_Data* objData = self->data;

    objData->ownBlockIndex = -1;
    objData->nearbyBlockIndex = -1;

    LODAnimator_CheckIfNearbyBlockAppeared(self);

    blockAddLODAnimator(self);
}

// export: 1
void LODAnimator_obj_Control(Object* self) {
    LODAnimator_Setup* objSetup;
    LODAnimator_Data* objData;
    s8 nearbyBlockFound;
    Block* prevOwnBlock;
    f32 distance;
    f32 blendValue;

    objSetup = (LODAnimator_Setup*)self->setup;
    objData = self->data;

#ifdef DEBUG_ANIMATOR
    {
        Vec3f blockCoords;

        blockCoords.x = self->globalPosition.x + BLOCKS_GRID_UNIT * objSetup->gridOffsetX;
        blockCoords.y = self->globalPosition.y;
        blockCoords.z = self->globalPosition.z + BLOCKS_GRID_UNIT * objSetup->gridOffsetZ;
        diPrintf("Checking for block at %.1f, %.1f, %.1f:\n", &blockCoords.x, &blockCoords.y, &blockCoords.z);
        
        if (objData->flags & LODAnimator_FLAG_1_Nearby_Block_Found) {
            diPrintf("\tNearby block found! Hiding LOD.\n");
        } else {
            diPrintf("\tNearby block not found. Showing LOD!\n");
        }
    }
#endif

    //Try to find the object's local BLOCKS model
    prevOwnBlock = objData->ownBlock;
    if (objData->ownBlock == NULL) {
        objData->ownBlockIndex = mapWorldCoordsToBlockIndex(self->globalPosition.x, self->globalPosition.y, self->globalPosition.z);
        objData->ownBlock = mapGetBlockByIndex(objData->ownBlockIndex);
    }
    if (objData->ownBlock == NULL) {
        return;
    }

    //Try to find the nearby BLOCK that has an LOD stand-in
    LODAnimator_CheckIfNearbyBlockAppeared(self);

    //Update shapes when the flags change
    if ((objData->prevFlags != objData->flags) ||
        ((objSetup->options & LODAnimator_OPTION_2_Update_Shapes_on_Local_Block_Load) && (prevOwnBlock == NULL)) //Optionally update shapes when the local Block loads, too
    ) {
        objData->prevFlags = objData->flags;
        LODAnimator_UpdateShapes(self, !(objData->flags & LODAnimator_FLAG_1_Nearby_Block_Found));
    }
}

// export: 2
void LODAnimator_obj_Update(Object* self) { }

// export: 3
void LODAnimator_obj_Print(Object* self, Gfx** gfx, Mtx** mtx, Vertex** vtx, Triangle** pols, s8 visibility) {
    if (visibility) {
        objprintDrawModel(self, gfx, mtx, vtx, pols, 1.0f);
    }
}

// export: 4
void LODAnimator_obj_Free(Object* self, s32 onlySelf) {
    LODAnimator_Setup* objSetup = (LODAnimator_Setup*)self->setup;
    Object* player;
    Vec3f uDirection;
    s32 angleTargetToAnimator;
    Vec3f targetToAnimator;
    Vec3f targetToPlayer;
    
    //Optionally show the LOD again on unload (only if the player is moving away from the target block)
    if (objSetup->options & LODAnimator_OPTION_1_Show_LOD_on_Unload) {
        player = objGetPlayer();
        if (player) {
            //Get the angle from the target to the LODAnimator
            uDirection.x = -objSetup->gridOffsetX;
            uDirection.z = -objSetup->gridOffsetZ;
            vec3Normalize(&uDirection);
            angleTargetToAnimator = Arctanf(uDirection.x, uDirection.z) - M_90_DEGREES;
            CIRCLE_WRAP(angleTargetToAnimator);
#ifdef DEBUG_UNLOAD
            recomp_printf("angleTargetToAnimator: %4x\n", angleTargetToAnimator);
#endif

            //Get the vector from the target to the LODAnimator (in worldSpace)
            targetToAnimator.x = -(BLOCKS_GRID_UNIT * objSetup->gridOffsetX);
            targetToAnimator.z = -(BLOCKS_GRID_UNIT * objSetup->gridOffsetZ);

            //Get the vector from the target to the player (in worldSpace)
            targetToPlayer.x = player->globalPosition.x - (self->globalPosition.x + BLOCKS_GRID_UNIT * objSetup->gridOffsetX);
            targetToPlayer.z = player->globalPosition.z - (self->globalPosition.z + BLOCKS_GRID_UNIT * objSetup->gridOffsetZ);

#ifdef DEBUG_UNLOAD
            recomp_printf("targetToAnimator, targetToPlayer in WorldSpace:\n");
            vec3_recomp_printf(&targetToAnimator);
            vec3_recomp_printf(&targetToPlayer);
#endif

            //Rotate both vectors so they're now in the target-to-LODAnimator coordinate space
            rotate_point_by_angle_2D(targetToAnimator.x, targetToAnimator.z, &targetToAnimator.x, &targetToAnimator.z, -angleTargetToAnimator);
            rotate_point_by_angle_2D(targetToPlayer.x, targetToPlayer.z, &targetToPlayer.x, &targetToPlayer.z, -angleTargetToAnimator);

#ifdef DEBUG_UNLOAD
            recomp_printf("targetToAnimator, targetToPlayer in LocalSpace:\n");
            vec3_recomp_printf(&targetToAnimator);
            vec3_recomp_printf(&targetToPlayer);
#endif

            //Show the LOD if the player is moving away from the target (in targetToAnimator space)
            if (targetToPlayer.x > targetToAnimator.x) {
                LODAnimator_UpdateShapes(self, TRUE);
            }
        }
    }

    blockRemoveLODAnimator(self);
}

// export: 5
u32 LODAnimator_obj_GetModelFlags(Object* self){
    return MODFLAGS_NONE;
}

// export: 6
u32 LODAnimator_obj_GetDataSize(Object* self, u32 offsetAddr){
    return sizeof(LODAnimator_Data);
}


// export: 7
/* Updates the render flags of the shapes the LODAnimator affects (showing/hiding the LOD). 
   Returns TRUE if the shapes were successfully updated. */
static s32 LODAnimator_UpdateShapes(Object* self, s32 showLOD) {
    LODAnimator_Data* objData;
    LODAnimator_Setup* objSetup;
    Block* block;
    BlockShape* shapes;
    u8 shapeIdx;
    u8 shapeUpdated = FALSE;

    objData = self->data;
    objSetup = (LODAnimator_Setup*)self->setup;

    block = objData->ownBlock;
    if (block == NULL) {
        return FALSE;
    }

    //Loop over shapes, and update render flags on shapes with a matching animatorID tag
    for (shapeIdx = 0, shapes = block->shapes; shapeIdx < block->shapeCount; shapeIdx++){
        if (objSetup->animatorID == shapes[shapeIdx].animatorID){
            if (showLOD){
                shapes[shapeIdx].flags &= ~RENDER_SHAPE_HIDE;
            } else {
                shapes[shapeIdx].flags |= RENDER_SHAPE_HIDE;
            }
            shapeUpdated = TRUE;
        }
    }

    return shapeUpdated;
}

// export: 8
/* Returns the loadedBlockIdx of the nearby Block the LODAnimator is checking (the one that needs an LOD stand-in). */
static s32 LODAnimator_GetNearbyBlockIdx(Object* self) {
    LODAnimator_Data* objData = self->data;
    if (objData == NULL) {
        return -1;
    }

    return objData->nearbyBlockIndex;
}

// export: 9
/* Clears the loadedBlockIdx of the nearby Block the LODAnimator is checking (the one that needs an LOD stand-in). */
static void LODAnimator_ClearNearbyBlock(Object* self) {
    LODAnimator_Data* objData = self->data;
    if (objData == NULL) {
        return;
    }

    objData->nearbyBlockIndex = -1;
}

// export: 10
/* Returns the loadedBlockIdx of the LODAnimator's own local Block (the one that contains extra LOD stand-in shapes for a nearby Block). */
static s32 LODAnimator_GetOwnBlockIdx(Object* self) {
    LODAnimator_Data* objData = self->data;
    if (objData == NULL) {
        return -1;
    }

    return objData->ownBlockIndex;
}

// export: 11
/* Clears references to the LODAnimator's own local Block (the one that contains extra LOD stand-in shapes for a nearby Block). */
static void LODAnimator_ClearOwnBlock(Object* self) {
    LODAnimator_Data* objData = self->data;
    if (objData == NULL) {
        return;
    }

    objData->ownBlockIndex = -1;
    objData->ownBlock = NULL;
}

/* Checks the visGrid to see if a particular cell of the world map is loaded */
static _Bool LODAnimator_checkVisGrid(Object* self, LODAnimator_Data* objData, LODAnimator_Setup* objSetup) {
    u8 ownBlockVisGridX;
    u8 ownBlockVisGridZ;
    s8 nearbyVisGridX;
    s8 nearbyVisGridZ;
    s8 layer;
    s8* layerGrid;

    ownBlockVisGridX = (self->globalPosition.x - gWorldX)/BLOCKS_GRID_UNIT;
    ownBlockVisGridZ = (self->globalPosition.z - gWorldZ)/BLOCKS_GRID_UNIT;

    nearbyVisGridX = ownBlockVisGridX + objSetup->gridOffsetX;
    nearbyVisGridZ = ownBlockVisGridZ + objSetup->gridOffsetZ;

    //TODO: just using the current map layer for now, but maybe have an option to choose a specific map layer in objSetup?
    layer = mapGetLayer();

    if (mapGetBlockFromGrid(nearbyVisGridX, nearbyVisGridZ, layer)) {
        layerGrid = mapGetBlockGridLayer(layer);
        if (layerGrid == NULL) {
            objData->nearbyBlockIndex = -1;
            return FALSE;
        }
        objData->nearbyBlockIndex = layerGrid[GRID_INDEX(nearbyVisGridZ, nearbyVisGridX)];
        return TRUE;
    } else {
        objData->nearbyBlockIndex = -1;
        return FALSE;
    }
}

// export: 12
static void LODAnimator_CheckIfNearbyBlockAppeared(Object* self) {
    LODAnimator_Data* objData = self->data;
    LODAnimator_Setup* objSetup = (LODAnimator_Setup*)self->setup;
    s8 nearbyBlockFound;

    if (objData == NULL || objSetup == NULL) {
        return;
    }

    //Check the VisGrid, and try to find the nearby BLOCKS model that needs an LOD stand-in
    nearbyBlockFound = LODAnimator_checkVisGrid(self, objData, objSetup);
    if (nearbyBlockFound == NULL) {
        if (objData->flags & LODAnimator_FLAG_1_Nearby_Block_Found) {
            objData->flags &= ~LODAnimator_FLAG_1_Nearby_Block_Found;
        }
    } else {
        if ((objData->flags & LODAnimator_FLAG_1_Nearby_Block_Found) == FALSE) {
            objData->flags |= LODAnimator_FLAG_1_Nearby_Block_Found;
        }
    }
}

DLL_LODAnimator_Vtbl DLL_LODAnimator_vtbl = {
    .base = {
        .Setup = (void*)LODAnimator_obj_Setup,
        .Control = LODAnimator_obj_Control,
        .Update = LODAnimator_obj_Update,
        .Print = LODAnimator_obj_Print,
        .Free = LODAnimator_obj_Free,
        .GetModelFlags = LODAnimator_obj_GetModelFlags,
        .GetDataSize = LODAnimator_obj_GetDataSize,
    },
    .UpdateShapes = LODAnimator_UpdateShapes,
    .GetNearbyBlockIdx = LODAnimator_GetNearbyBlockIdx,
    .ClearNearbyBlock = LODAnimator_ClearNearbyBlock,
    .GetOwnBlockIdx = LODAnimator_GetOwnBlockIdx,
    .ClearOwnBlock = LODAnimator_ClearOwnBlock,
    .CheckIfNearbyBlockAppeared = LODAnimator_CheckIfNearbyBlockAppeared,
};

#ifdef DEBUG_VIS_GRID
static void printVisGrid(LODAnimator_Data* objData, LODAnimator_Setup* objSetup) {
    de_print_set_fixedwidth_all(TRUE);

    diPrintf("LAYOUT:\n");
    for (u32 z = 0; z < BLOCKS_GRID_SPAN; z++) {
        for (u32 x = 0; x < BLOCKS_GRID_SPAN; x++) {
            if (x == 7 && z == 7) {
                diPrintf("%s ", mapGetBlockFromGrid(x, z, 0) ? "x" : "_");
            } else {
                diPrintf("%s ", mapGetBlockFromGrid(x, z, 0) ? "O" : "_");
            }
        }
        diPrintf("\n");
    }

    de_print_set_fixedwidth_all(FALSE);
}

RECOMP_CALLBACK("*", recomp_on_game_tick_start) void printGridVisibilities(Object* self) {
    printVisGrid(NULL, NULL);
}
#endif
