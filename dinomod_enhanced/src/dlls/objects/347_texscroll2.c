#include "common_objsetups.h"
#include "core/map.h"
#include "modding.h"
#include "recomputils.h"

#include "common.h"

#include "recomp/dlls/objects/347_texscroll2_recomp.h"

//TEMPORARY DEFINES
#define TexScroll2_setupTextureScrolling TexScroll2_setup_texture_scrolling
//END OF TEMPORARY DEFINES

typedef struct {
    s32 materialIndex;      //Index of the main material being animated (in the local Block's materials)
    s32 blendMaterialIndex; //Index of the secondary material being animated (optional, -1 if unused)
    u8 scrollSetupNeeded;
    s8 uSpeedA;             //U scroll speed for primary material
    s8 vSpeedA;             //V scroll speed for primary material
    s8 uSpeedB;             //U scroll speed for secondary blend material
    s8 vSpeedB;             //V scroll speed for secondary blend material
} TexScroll2_Data;

RECOMP_PATCH void TexScroll2_setupTextureScrolling(Object* self, TexScroll2_Data* objData) {
    Block* block;
    TexScroll2_Setup* objSetup;
    Texture* texture;
    Texture* textureBlended;
    s32 textureCount;
    s32 materialIndex;
    s32 shapeIndex;
    s32 widthA;
    s32 heightA;
    s32 widthB;
    s32 heightB;
    s32* scrollTable;

    objSetup = (TexScroll2_Setup*)self->setup;

    //Get object's local Block
    block = mapGetBlockByIndex(mapWorldCoordsToBlockIndex(self->srt.transl.x, self->srt.transl.y, self->srt.transl.z));
    if (block == NULL) {
        objData->scrollSetupNeeded = TRUE;
        return;
    }

    //Get TABLES.bin subfile #14 ("scroll table")
    scrollTable = objGetTable(14);
    if (scrollTable == NULL) {
        STUBBED_PRINTF("TEXSCROLL: no scroll table\n");
        return;
    }

    //Get texture from table
    texture = texGetCached(-scrollTable[objSetup->textureIndex]);
    if (texture == NULL) {
        STUBBED_PRINTF("TEXSCROLL: cannot find texture '%d' %f %f %f %d\n",
            scrollTable[objSetup->textureIndex],
            self->srt.transl.x, self->srt.transl.y, self->srt.transl.z,
            objSetup->base.uID);
        return;
    }

    //Loop over Block's materials until finding one that uses the texture
    for (materialIndex = 0; materialIndex < block->materialCount; materialIndex++){
        if (texture == block->materials[materialIndex].texture){
            break;
        }
    }
    //Bail if material not found
    if (materialIndex == block->materialCount) {
        return;
    }

    widthA = texture->width << 6;
    heightA = texture->height << 6;
    objData->materialIndex = materialIndex;
    objData->blendMaterialIndex = -1;
    widthB = widthA;
    heightB = heightA;

    //Loop over Block's materials until finding one that uses the blended texture (if it's in use)
    if (objSetup->blendTextureIndex != -1) {
        textureBlended = texGetCached(-scrollTable[objSetup->blendTextureIndex]);
        if (textureBlended != NULL) {
            for (materialIndex = 0; materialIndex < block->materialCount; materialIndex++) {
                if (textureBlended == block->materials[materialIndex].texture) {
                    break;
                }
            }
            if (materialIndex == block->materialCount) {
                return;
            }
            widthB = textureBlended->width << 6;
            heightB = textureBlended->height << 6;
            objData->blendMaterialIndex = materialIndex;
        }
    }

    //Loop over Block's shapes and enable UV scrolling on any that use the relevant materials
    for (shapeIndex = 0; shapeIndex < block->shapeCount; shapeIndex++){
        if (block->shapes[shapeIndex].materialIndex == objData->materialIndex) {
            //If blend material is specified, only apply scrolling if the shape uses it as its secondary multitexture material
            if (objData->blendMaterialIndex == -1 || 
                objData->blendMaterialIndex == block->shapes[shapeIndex].blendMaterialIndex) {
                    
                //@recomp: fix a bug where this can sometimes target the wrong texScroller 
                // when using the `changeScrollSpeed` export. This used happen in Discovery Falls:
                // the cradle ropes' texScroll2 object would sometimes start scrolling nearby waterfalls instead!
                if (block->shapes[shapeIndex].texScrollerID != 0xFF && 
                    //@recomp: make sure the texScrollerID is still valid, by checking it uses the same texture as this material does
                    (blockTexscrollGetTexture(block->shapes[shapeIndex].texScrollerID) == texture) 
                ) {
                    blockTexscrollSetWithTexture(block->shapes[shapeIndex].texScrollerID, 
                        objData->uSpeedA, objData->vSpeedA, widthA, heightA, 
                        objData->uSpeedB, objData->vSpeedB, widthB, heightB,
                        texture); //@recomp: use a custom version of this function with a reference to the scroller's Texture
                } else {
                    block->shapes[shapeIndex].texScrollerID = blockTexscrollAddWithTexture(
                        objData->uSpeedA, objData->vSpeedA, widthA, heightA,
                        objData->uSpeedB, objData->vSpeedB, widthB, heightB, 
                        texture); //@recomp: use a custom version of this function with a reference to the scroller's Texture
                }
            }
        }
    }
}
