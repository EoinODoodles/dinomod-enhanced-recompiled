#include "common_objsetups.h"
#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "common.h"
#include "game/gamebits.h"
#include "sys/dll.h"
#include "sys/gfx/modgfx.h"
#include "sys/gfx/texture.h"
#include "sys/main.h"
#include "sys/objtype.h"
#include "dll.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/272_collectable.h"

#include "recomp/dlls/_asm/351_recomp.h"

//TODO: remove after decomp update
#define GroundAnimator_store_shapeIDs_and_vertex_weights dll_351_func_A28

typedef struct {
    f32* vtxWeights;        //Displacement strengths (from 0 to 1) for each vertex being animated (influence falls off from centre)
    Object* collectable;
    s32 digDepth;           //Current dig progress (starts at 0, increases while digging)
    f32 falloffRadius;      //Vertex influence tapers off with this radius, and it also decides how far Tricky scoots backwards while digging
    f32 collectableDepth;   //Affects how far down the collectable is buried
    s16 animatedShapeIDs[5];
    s16 misconfiguredShapeID;
    s16 animatedVtxCount;   //The total number of vertices being animated
    u8 animatedShapesCount; //Total shapes animated (i.e. number of items in `animatedShapeIDs`)
    s8 previousDigDepth;    //Previous dig progress value (vertex updates are queued when this differs from current dig value)
    u8 magicCaveID;         //Seems intended as an index for a specific Magic Cave instance (can be queried with `get_magic_cave_index` export)
    u8 animUpdates;         //Used to queue vertex animation updates
    u8 flags;               //Various state flags: Block search, dig finished, Magic Cave entrance/glow
} GroundAnimator_Data;

typedef enum {
    GroundAnimator_FLAG_0_None = 0,
    GroundAnimator_FLAG_1_Block_Found = 1,
    GroundAnimator_FLAG_2_Dig_Finished = 2,
    GroundAnimator_FLAG_4_Unused = 4,
    GroundAnimator_FLAG_8_Magic_Cave_Entrance = 8,
    GroundAnimator_FLAG_10_Glow_Created = 0x10,
    GroundAnimator_FLAG_20_Glow_Required = 0x20
} GroundAnimator_Flags;

extern u16 dDigJingles[2];

#define OPACITY_MAX 0xFF
#define TOTAL_JINGLES 2

RECOMP_PATCH void GroundAnimator_store_shapeIDs_and_vertex_weights(Object* self, GroundAnimator_Data* objData, GroundAnimator_Setup* objSetup) {
    BlockShape* shapes;
    s32 pad[6];
    f32 radiusSq;
    f32 tValue;
    s32 blockWorldGridX;
    s32 blockWorldGridZ;
    Block* block;
    f32 digBlockX;
    f32 digBlockZ;
    s32 shapeIdx;
    s32 animVtxIdx;
    f32 dx;
    f32 dz;
    s32 vtxID;
    
    block = map_get_block_by_index(map_world_coords_to_block_index(self->srt.transl.x, self->srt.transl.y, self->srt.transl.z));
    
    if ((block == NULL) || !(block->vtxFlags & 8)) {
        return;
    }
        
    //Get the GroundAnimator's position relative to the Blocks model's local origin
    blockWorldGridX = floor_f((self->srt.transl.x - gWorldX) / 640.0f);
    blockWorldGridZ = floor_f((self->srt.transl.z - gWorldZ) / 640.0f);

    digBlockX = self->srt.transl.x - (blockWorldGridX * 640.0f + gWorldX);
    digBlockZ = self->srt.transl.z - (blockWorldGridZ * 640.0f + gWorldZ);
    
    //Search through the Block's shapes, looking for any tagged with the animatorID
    objData->animatedShapesCount = 0;
    shapes = block->shapes;
    for (shapeIdx = 0, animVtxIdx = 0; shapeIdx < block->shapeCount; shapeIdx++) {
        //Only animate shapes that have a matching animatorID
        if (objSetup->animatorID == shapes[shapeIdx].animatorID) {

            //@recomp: unhide shape (for revealing dig spots in SH Well after stalactites crack up the ground)
            if (shapes[shapeIdx].flags & RENDER_SHAPE_HIDE) {
                shapes[shapeIdx].flags &= ~RENDER_SHAPE_HIDE;
            }

            //Calculate vertices' influence weights, based on GroundAnimator's distance and falloff
            for (vtxID = shapes[shapeIdx].vtxBase; vtxID < shapes[shapeIdx + 1].vtxBase; vtxID++, animVtxIdx++) {
                dx = block->vertices[vtxID].ob[0];
                dx -= digBlockX;
                dz = block->vertices[vtxID].ob[2];
                dz -= digBlockZ;
                
                radiusSq = SQ(objData->falloffRadius);
                tValue = (SQ(dx) + SQ(dz)) / radiusSq;
                if (tValue > 1.0f) {
                    tValue = 1.0f;
                }

                tValue = SQ(tValue);
                tValue = 1.0f - tValue;
                objData->vtxWeights[animVtxIdx] = tValue;
            }
            
            //Store last tagged shapeID that doesn't have RENDER_SUBSURFACE enabled (doesn't do anything with this)
            if ((shapes[shapeIdx].flags & RENDER_SUBSURFACE) == FALSE) {
                objData->misconfiguredShapeID = shapeIdx;
            }
            
            //Store ID of animated shape
            objData->animatedShapeIDs[objData->animatedShapesCount++] = shapeIdx;

            //Warn if too many shapes are being animated
            if (objData->animatedShapesCount >= ARRAYCOUNT(objData->animatedShapeIDs)) {
                STUBBED_PRINTF("groundanim group overflow\n");
            }
        }
    }
}
