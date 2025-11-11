#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/map.h"

#include "recomp/dlls/objects/362_portaltexanimator_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 flagID;
/*1A*/ s16 unk1A;
/*1C*/ u8 maxOpacity;
/*1D*/ u8 minOpacity;
/*1E*/ s8 animatorID;
/*1E*/ s8 unk1F;
/*20*/ s8 unk20;
/*21*/ u8 unk21;
/*22*/ u16 minDistance;
} PortalTexAnimator_Setup;

typedef struct {
/*00*/ s32 animatedVertexCount;
/*04*/ f32 minDistance;
/*08*/ f32 maxDistance;
/*0C*/ f32 unkC;
/*10*/ s32 unk10;
/*14*/ s16 vertexOpacity;
/*16*/ s8 animatorID;
/*17*/ s8 enabled; //unfinished, doesn't affect behaviour
/*18*/ s8 blockFound;
/*19*/ s8 unk19;
/*1A*/ s8 unk1A;
/*1B*/ s8 unk1B;
} PortalTexAnimator_Data;

// Clamps the PortalTexAnimator's 16-bit opacity value into an 8-bit range, fixing a bug where the vertices' alpha would suddenly wrap around to a low value 
RECOMP_PATCH void portaltexanimator_animate_vertices(PortalTexAnimator_Data* objdata, PortalTexAnimator_Setup* setup, Block* block) {
    BlockShape *shapes;
    Vtx_t *animatedVertices;
    s32 shapeIndex;
    s32 vertexIndex;
    
    //@recomp: clamp the opacity value before it's applied (Banjeoin)
    if (objdata->vertexOpacity > 0xFF){
        objdata->vertexOpacity = 0xFF;
    } else if (objdata->vertexOpacity < 0){
        objdata->vertexOpacity = 0;
    }

    animatedVertices = block->vertices2[block->vtxFlags & 1];

    shapeIndex = 0;
    shapes = block->shapes;

    //Iterate over shapes, and update all vertices' alpha on shapes with matching animatorID tag
    while (shapeIndex < block->shapeCount){
        
        if (objdata->animatorID == shapes[shapeIndex].animatorID){
            for (vertexIndex = shapes[shapeIndex].vtxBase; vertexIndex < shapes[shapeIndex + 1].vtxBase; vertexIndex++){
                //@recomp: ignore vertices with 0 base opacity (i.e. preserve vertex alpha based texture blending)
                if (block->vertices[vertexIndex].cn[3] == 0)
                    continue;

                animatedVertices[vertexIndex].cn[3] = objdata->vertexOpacity;
            }
            
            //Switch shape's draw flags when opacity is zero
            if (objdata->vertexOpacity == 0){
                shapes[shapeIndex].flags |= 0x200000;
                if (setup->unk21 != 0){
                    shapes[shapeIndex].flags |= 0x800;
                }
            } else {
                shapes[shapeIndex].flags &= 0xFFDFFFFF;
                if (setup->unk21 != 0){
                    shapes[shapeIndex].flags &= ~0x800;
                }
            }
        }

        shapeIndex++;
    }
}
