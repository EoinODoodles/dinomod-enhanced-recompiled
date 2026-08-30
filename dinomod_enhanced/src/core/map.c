#include "modding.h"
#include "recomputils.h"

#include "sys/intersect.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/memory.h"

#include "core/map.h"
#include "custom/dlls/LODAnimator.h"

extern Struct_D_800B9768 D_800B9768;

// Add CRF Traprooms, the old Krazoa Shrine, and old Earthwalker Temple to global map.
// Original patch by nuggs.
RECOMP_HOOK_RETURN("mainInit") void dinomod_global_map_init(void) {
    D_800B9768.unk4[MAP_CLOUDRUNNER_TRAPROOMS].xMin = -32;
    D_800B9768.unk4[MAP_CLOUDRUNNER_TRAPROOMS].xMax = -27;
    D_800B9768.unk4[MAP_CLOUDRUNNER_TRAPROOMS].zMin = -50;
    D_800B9768.unk4[MAP_CLOUDRUNNER_TRAPROOMS].zMax = -47;
    D_800B9768.unk4[MAP_CLOUDRUNNER_TRAPROOMS].unk8 = 0;
    D_800B9768.unk4[MAP_CLOUDRUNNER_TRAPROOMS].unk9 = 2;

    D_800B9768.unk4[MAP_KRAZOA_SHRINE].xMin = -32;
    D_800B9768.unk4[MAP_KRAZOA_SHRINE].xMax = -31;
    D_800B9768.unk4[MAP_KRAZOA_SHRINE].zMin = -34;
    D_800B9768.unk4[MAP_KRAZOA_SHRINE].zMax = -31;
    D_800B9768.unk4[MAP_KRAZOA_SHRINE].unk8 = 0;
    D_800B9768.unk4[MAP_KRAZOA_SHRINE].unk9 = 2;

    D_800B9768.unk4[MAP_EARTHWALKER_TEMPLE].xMin = -19;
    D_800B9768.unk4[MAP_EARTHWALKER_TEMPLE].xMax = -12;
    D_800B9768.unk4[MAP_EARTHWALKER_TEMPLE].zMin = -38;
    D_800B9768.unk4[MAP_EARTHWALKER_TEMPLE].zMax = -31;
    D_800B9768.unk4[MAP_EARTHWALKER_TEMPLE].unk8 = 3;
    D_800B9768.unk4[MAP_EARTHWALKER_TEMPLE].unk9 = 6;

    D_800B9768.unk10[MAP_CLOUDRUNNER_TRAPROOMS].unk0[0] = 0xC2;
    D_800B9768.unk10[MAP_CLOUDRUNNER_TRAPROOMS].unk0[1] = 0x51;
    D_800B9768.unk10[MAP_CLOUDRUNNER_TRAPROOMS].unk0[2] = 0x00;
    D_800B9768.unk10[MAP_CLOUDRUNNER_TRAPROOMS].unk0[3] = 0x00;

    D_800B9768.unk10[MAP_KRAZOA_SHRINE].unk0[0] = 0xF0;
    D_800B9768.unk10[MAP_KRAZOA_SHRINE].unk0[1] = 0x00;
    D_800B9768.unk10[MAP_KRAZOA_SHRINE].unk0[2] = 0x00;
    D_800B9768.unk10[MAP_KRAZOA_SHRINE].unk0[3] = 0x00;

    D_800B9768.unk10[MAP_EARTHWALKER_TEMPLE].unk0[0] = 0x38;
    D_800B9768.unk10[MAP_EARTHWALKER_TEMPLE].unk0[1] = 0x3F;
    D_800B9768.unk10[MAP_EARTHWALKER_TEMPLE].unk0[2] = 0x3F;
    D_800B9768.unk10[MAP_EARTHWALKER_TEMPLE].unk0[3] = 0x1E;
    D_800B9768.unk10[MAP_EARTHWALKER_TEMPLE].unk0[4] = 0x3E;
    D_800B9768.unk10[MAP_EARTHWALKER_TEMPLE].unk0[5] = 0x3C;
    D_800B9768.unk10[MAP_EARTHWALKER_TEMPLE].unk0[6] = 0x78;
    D_800B9768.unk10[MAP_EARTHWALKER_TEMPLE].unk0[7] = 0x78;

    D_800B9768.unkC[MAP_EARTHWALKER_TEMPLE] = 0;
    D_800B9768.unkC[MAP_KRAZOA_SHRINE] = 0;
    D_800B9768.unkC[MAP_CLOUDRUNNER_TRAPROOMS] = 0;
}

// #define DEBUG_LOD_ANIMATORS
#define LOD_ANIMATORS_MAX 16
static Object* gLODObjects[LOD_ANIMATORS_MAX];
static u8 gLODObjectCount = 0;

/* Logs the array of currently loaded LODAnimator objects */
void blockLogLODAnimators(void) {
    recomp_printf("gLODObjects (%d):\n", gLODObjectCount);
    for (u32 i = 0; i < gLODObjectCount; i++) {
        recomp_printf("%d) %8x\n", i, gLODObjects[i]);
    }
    recomp_printf("\n");
}

/* Adds an LODAnimator object to the array of currently-loaded LODAnimator objects */
void blockAddLODAnimator(Object* lodAnimator) {
    if (gLODObjectCount >= LOD_ANIMATORS_MAX) {
        return;
    }

    for (u32 i = 0; i < gLODObjectCount; i++) {
        if (gLODObjects[i] == lodAnimator) {
            return;
        }
    }

    gLODObjects[gLODObjectCount] = lodAnimator;
    gLODObjectCount++;

#ifdef DEBUG_LOD_ANIMATORS
    blockLogLODAnimators();
#endif
}

/* Removes an LODAnimator object from the array of currently-loaded LODAnimator objects */
void blockRemoveLODAnimator(Object* lodAnimator) {
    u32 i;
    u32 j;
    u32 removed = FALSE;

    if (gLODObjectCount == 0) {
        return;
    }

    for (i = 0; i < gLODObjectCount; i++) {
        if (gLODObjects[i] == lodAnimator) {
            gLODObjects[i] = NULL;
            removed = TRUE;
            break;
        }
    }

    if (removed == FALSE) {
        return;
    }

    for (j = i; j < gLODObjectCount - 1; j++) {
        gLODObjects[j] = gLODObjects[j + 1];
    }

    gLODObjectCount--;

#ifdef DEBUG_LOD_ANIMATORS
    blockLogLODAnimators();
#endif
}

/* Checks if the block being freed is referenced by an LODAnimator object, and handles it */
static void blockFreeCheckLODAnimators(s32 freedBlockIndex) {
    Object* obj;
    u32 i;

    for (i = 0; i < gLODObjectCount; i++) {
        obj = gLODObjects[i];
        if (obj == NULL) {
            continue;
        }

        //Switch on LOD if the nearby Block it's acting as a stand-in for is being unloaded
        if (dll_lodAnimator(obj)->GetNearbyBlockIdx(obj) == freedBlockIndex) {
            dll_lodAnimator(obj)->ClearNearbyBlock(obj);
            dll_lodAnimator(obj)->UpdateShapes(obj, TRUE);
        }

        //Clear LOD's local blockIdx if its local Block is being unloaded
        else if (dll_lodAnimator(obj)->GetOwnBlockIdx(obj) == freedBlockIndex) {
            dll_lodAnimator(obj)->ClearOwnBlock(obj);
        }
    }
}

extern u8* gBlockRefCounts;
extern Block** gLoadedBlocks;
extern void blockColorTableFreeBlock(Block *block);
extern s16 *gLoadedBlockIds;
extern void blockFreeTextureAnims(Block*);
extern void blockTexscrollFree(u32 id);

RECOMP_PATCH void blockFree(s32 blockIndex) {
    Block *block;
    s32 i;
    u8 runtimeValue;
    s32* temp_a0_2;

    if (blockIndex < 0) {
        return;
    }
    
    gBlockRefCounts[blockIndex] -= 1;
    if (gBlockRefCounts[blockIndex] == 0) {

        //@recomp: handle switching on LODAnimators as soon as the block they target gets unloaded 
        blockFreeCheckLODAnimators(blockIndex);

        block = gLoadedBlocks[blockIndex];
        blockColorTableFreeBlock(block);
        gLoadedBlockIds[blockIndex] = -1;
        gLoadedBlocks[blockIndex] = NULL;
        if (block->numTexAnims != 0) {
            blockFreeTextureAnims(block);
        }

        //Loop over shapes and free them
        for (i = 0; i < block->shapeCount; i++){
            runtimeValue = (&block->shapes[i])->texScrollerID;
            if (runtimeValue != 0xFF) {
                blockTexscrollFree(runtimeValue);
            }
        }

        //Loop over materials and free their textures
        for (i = 0; i < block->materialCount; i++){
            texFreeTexture(block->materials[i].texture);
        }
        
        if ((u32*)block->unk1C != NULL) {
            mmFree((u32*)block->unk1C);
        }
        
        trackIntersectMarkBlocksDirty();
        mmFree(block);
    }
}
