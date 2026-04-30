#include "modding.h"

#include "common.h"
#include "gbi_extra.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "sys/bitstream.h"
#include "sys/dl_debug.h"
#include "sys/gfx/texture.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/menu.h"
#include "sys/newshadows.h"
#include "sys/segment_1D900.h"

#include "core/map_render.h"
#include "core/debug_fog.h"

extern Struct_D_800B9768 D_800B9768;

// Add CRF Traprooms, the old Krazoa Shrine, and old Earthwalker Temple to global map.
// Original patch by nuggs.
RECOMP_HOOK_RETURN("game_init") void dinomod_global_map_init(void) {
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

#define RENDER_WATER RENDER_UNK2000
#define RENDER_UNK40000000 0x40000000
#define RENDER_UNK20000000 0x20000000
#define RENDER_UNK4000000 0x4000000
#define RENDER_UNK2000000 0x2000000
#define RENDER_UNK800000 0x800000
#define RENDER_UNK400000 0x400000

RECOMP_PATCH void block_setup_gdl_groups(Block *block)
{
    s32 i;

    for (i = 0; i < block->shapeCount; i++)
    {
        BlockShape *shape;
        Texture *texture;
        s32 texFlags = 0;
        s32 aa;
        s32 flags;
        Gfx *mygdl;

        block->shapes[i].unk16 = 0xff;
        
        shape = &block->shapes[i];
        flags = shape->flags;
        aa = flags & RENDER_ANTI_ALIASING;

        if (shape->materialIndex == 0xff) {
            texture = NULL;
        } else {
            texture = block->materials[shape->materialIndex].texture;
            if (texture != NULL) {
                texFlags = texture->flags;
            }
        }

        if ((flags & RENDER_DECAL_SIMPLE) == 0) {
            if ((flags & RENDER_UNK1000000) != 0) {
                flags |= (RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_UNK10);
            }
        }

        if (texFlags & RENDER_CUTOUT) {
            flags |= RENDER_Z_COMPARE;
        }

        if (aa != 0) {
            flags &= ~RENDER_ANTI_ALIASING;
        } else {
            flags |= RENDER_ANTI_ALIASING;
        }

        flags |= RENDER_Z_COMPARE;

        if (flags & RENDER_DECAL_SIMPLE)
        {
            if (flags & RENDER_SUBSURFACE) {
                flags &= ~RENDER_SUBSURFACE;
                tex_disable_modes(RENDER_SEMI_TRANSPARENT);
            } else {
                if ((flags & RENDER_WATER) || (flags & RENDER_SEMI_TRANSPARENT) || (flags & RENDER_DECAL)) {
                    flags |= RENDER_SEMI_TRANSPARENT;
                } else {
                    flags |= RENDER_SEMI_TRANSPARENT;
                    tex_disable_modes(RENDER_SEMI_TRANSPARENT);
                }
            }
        }

        mygdl = &block->gdlGroups[i * 3];
        // only set modes, don't set texture!
        tex_gdl_set_texture_simple(&mygdl, texture, 
            flags | RENDER_NO_CULL, 
            TEX_FRAME(0),
            TRUE, // force
            TEXOPT_INVISIBLE | TEXOPT_SET_MODES | TEXOPT_SKIP_MODE_CACHE);

        if ((flags & RENDER_WATER) && texture != NULL && (texture->flags & (RENDER_COMPOSITE_BASE | RENDER_COMPOSITE_OVERLAY)))
        {
            mygdl = &block->gdlGroups[i * 3];
            gSPLoadGeometryMode(mygdl++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
            if (texture->flags & (RENDER_COMPOSITE_BASE | RENDER_COMPOSITE_OVERLAY)) {
                gDPSetCombineMode(
                    mygdl++, 
                    G_CC_DINO_BLENDTEX_ENV,
                    G_CC_DINO_LERP_FROM_SHADE2
                );
            } else {
                gDPSetCombineMode(
                    mygdl++, 
                    G_CC_DINO_BLEND_TEX_SHADE_ENVA,
                    G_CC_DINO_LERP_FROM_SHADE_INVA2
                );
            }
            gDPSetOtherMode(
                mygdl++,
                G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | G_CYC_2CYCLE | G_PM_NPRIMITIVE,
                G_AC_NONE | G_ZS_PIXEL | G_RM_NOOP | G_RM_AA_ZB_XLU_SURF2
            );
        }

        if (flags & RENDER_DECAL_SIMPLE) {
            tex_enable_modes(RENDER_SEMI_TRANSPARENT);
        }
    }
}

extern DLBuilder D_800B49F0;
extern DLBuilder D_800B4A20;
extern s32 D_800B4A50;
extern s32 D_800B4A54;
extern s8 D_800B4A58; //gStartWarp?
extern s8 D_800B4A59;
extern s16 D_800B4A5C;
extern s16 D_800B4A5E; //gFadeDelayTimerStarted
extern Warp D_800B4A60;
extern s16 gMobileMapID;
extern s16 gMobileMapUnknown;
extern Plane gFrustumPlanes[MAP_LAYER_COUNT];
extern u32 gRenderList[MAX_RENDER_LIST_LENGTH];
extern s16 gRenderListLength;
extern Block *gBlocksToDraw[MAX_BLOCKS];
extern s16 gBlocksToDrawIdx;
extern Gfx* gMainDL;
extern Mtx* gWorldRSPMatrices;
extern Vertex* D_800B51D4;
extern Triangle* D_800B51D8;
extern s16 SHORT_800b51dc;
extern u32 UINT_800b51e0;
extern Camera* D_800B51E4;
extern struct Vec3_Int Vec3_Int_array[20];
extern MapHeader* gLoadedMapsDataTable[120];
extern MapObjSetupList gMapObjSetupLists[120];
extern s8 gMapType;
extern MapHeader* gMapActiveStreamMap;
extern SavedObject* D_800B96B0;
extern Block **gLoadedBlocks;
extern u8 gLoadedBlockCount;
extern s16 *gLoadedBlockIds;
extern s16 gNumTRKBLKEntries;
extern u8 *gBlockRefCounts;
extern s32* gFile_BLOCKS_TAB; // unknown pointer type
extern s32 gNumTotalBlocks;
extern s8 *gBlockIndices[MAP_LAYER_COUNT];
extern GlobalMapCell *gDecodedGlobalMap[MAP_LAYER_COUNT]; //16*16 grid of GlobalMapCell structs, one for each layer!
extern s8 *D_800B9700[MAP_LAYER_COUNT];
extern s8 *D_800B9714;
extern u16 *gFile_TRKBLK;
extern u32 *gFile_HITS_TAB;
extern s32* gFile_MAPS_TAB; // unknown pointer type
extern u8 *gMapReadBuffer;
extern StreamMap gMapStreamMapTable[8];
extern Struct_D_800B9768 D_800B9768;
extern BitStream D_800B9780;
extern u8 D_800B9794;
extern u8 *D_800B9798;
extern u8 D_800B979C;
extern s16 D_800B979E;
extern s16 *D_800B97A0;
extern BlockTexture *gBlockTextures;
extern BlockTextureScroller *D_800B97A8; //gMapTextureScrollers?
extern f32 D_800B97AC; //x
extern f32 D_800B97B0; //y
extern f32 D_800B97B4; //z
extern f32 D_800B97B8;
extern f32 D_800B97BC;
extern MapsUnk_800B97C0 *D_800B97C0; // 255 items
extern s16 D_800B97C4;
extern u8 _bss_800b97c8[0x8];

extern s32 D_80092A60;
extern s32 D_80092A64;
extern s32 gMapCurrentStreamCoordsX;
extern s32 gMapCurrentStreamCoordsZ;
extern f32 gWorldX;
extern f32 gWorldZ;
extern s8 D_80092A78;
extern s32 D_80092A7C[2];
extern s32 D_80092A84[2];
extern s8 gMapLayer;
extern DLBuilder *gDLBuilder;
extern s32 D_80092A94;
extern u32 UINT_80092a98;
extern s8 D_80092A9C[MAP_LAYER_COUNT];
extern s8 gMapNumStreamMaps;
extern s32 D_80092AA8;
extern f32 D_80092AAC[24];
extern s8 D_80092B0C[];
extern s32 D_80092BBC;
extern Unk80092BC0 D_80092BC0;

extern void func_8004D328(void);
extern void map_restore_saved_objects(MapHeader* arg0, s32 mapID);
extern HitsLine* block_load_hits(Block *block, s32 blockID, u8 unused, HitsLine* hits_ptr);
extern void func_800441F4(u32* arg0, s32 arg1);
extern void func_80048B14(Block *block);
extern void func_80048C24(Block *block);
extern u32 hits_get_size(s32 id);
extern void block_setup_vertices(Block *block);
extern void block_setup_gdl_groups(Block *block);
extern s32 block_setup_textures(Block *block);
extern void block_setup_xz_bitmap(Block *block);
extern void block_compute_vertex_colors(Block*,s32,s32,s32);
extern void func_80049D38(u32 arg0);
extern void func_80049FA8(Block*);
extern void func_800499BC(void);
extern void func_80049D88(void);
extern void func_80044BEC(void);
extern void func_80048F58(void);
extern void track_c_func(void);
extern u8 is_sphere_in_frustum(Vec3f *v, f32 radius);
extern void map_convert_objpositions_to_ws(MapHeader *map, f32 X, f32 Z);
extern void map_init_obj_setup_list(MapHeader* map, MapObjSetupList* setupList, s32 mapID, s32 curvesOnly);
extern MapHeader *map_load_streammap(s32, s32);
extern void map_read_layout(Struct_D_800B9768_unk4 *arg0, u8 *arg1, s16 arg2, s16 arg3, s32 maptabindex);
extern void map_update_objects_streaming(s32);
extern s32 map_func_800485FC(s32, s32, s32, s32, s32);
extern void func_80047404(s32, s32, s32*, s32*, s32*, s32*, s32, s32, s32);
extern void func_800496E4(s32 blockIndex);
extern s32 func_8004A058(Texture* tex, u32 flags, s32 arg2);
extern s32 map_should_obj_unload(Object*);
extern void func_8004B548(MapHeader*, s32, s32, Object*);
extern s32 map_should_stream_load_object(ObjSetup*, s8, s32);
extern s32 map_check_some_mapobj_flag(s32, u32);
extern void func_8004B710(s32 cellIndex_plusBitToCheck, u32 mapIndex, u32 arg2);
extern s32 func_8004AEFC(s32 mapID, s16 *arg1, s16 searchLimit);
extern s32 func_8004B4A0(ObjSetup* obj, s32 mapID);
extern void block_add_to_render_list(Block *block, f32 x, f32 z);
extern void func_800436DC(Object* arg0, s32 arg1);
extern s32 func_80045DC0(s32, s32, s32); //unsure of last arg
extern s32 map_find_streammap_index(s32);
extern s32 map_load_streammap_add_to_table(s32);  //unsure of worldGridZ here
extern s32 func_80048E04(u8, u8, u8, u8);
extern void func_8004A164(Texture*, s32);
extern void draw_render_list(Mtx *rspMtxs, s8 *visibilities);
extern void func_80043950(Block*, s16, s16, s16);
extern void func_80043FD8(s8* arg0);
extern s32 func_800451A0(s32 xPos, s32 zPos, Block* blocks);
extern void some_cell_func(BitStream* stream);
extern BlockTextureScroller* func_80049D68(s32 arg0);
extern s32 func_80045600(s32 arg0, BitStream *stream, s16 arg2, s16 arg3, s16 arg4);

static void draw_render_list_MODIFIED(Mtx* rspMtxs, s8* visibilities) {
    BlockShape* shape;
    Vtx_t *tempVtx;
    s32 f3dIdx;
    BlockTextureScroller* texScroll;
    s32 i;
    BlocksTextureIndexData* temp_v0_4;
    u32 spE4;
    s32 colourR;
    s32 colourG;
    s32 colourB;
    s32 spD4;
    s32 spD0;
    s32 spCC;
    EncodedTri* endTri;
    s32 spC4;
    EncodedTri* tempTri;
    EncodedTri* tri;
    Gfx* baseDL;
    Texture* tex1;
    Texture* tex0;
    s32 cmdCount;
    s32 temp_v1;
    Mtx* spA4;
    s8 spA3;
    s8 temp2;
    s32 shapeIdx;
    s32 renderFlags;
    s32 frameValue;
    Block* block;
    Object** objects;
    Object *obj;
    /* RECOMP */
    u32 fogColourPrev = gDLBuilder->fogColor;

    spC4 = -1;
    objects = get_world_objects(NULL, NULL);
    gDLL_57->vtbl->func2(&colourR, &colourG, &colourB, &spD4, &spD0, &spCC);
    
    for (i = 1; i < gRenderListLength; i++) {
        shapeIdx = f3dIdx = (gRenderList[i] & 0x3F80) >> 7;
        if (gRenderList[i] & 0x40) {
            obj = objects[shapeIdx];
            func_800436DC(obj, visibilities[shapeIdx]);
            spA3 = 0;
        } else {
            // @fake
            if (i) {}
            temp_v1 = gRenderList[i] & 0x3F;
            spE4 = 0;

            if (temp_v1 != spC4) {
                spA3 = -1;
                SHORT_800b51dc = -1;
                spC4 = temp_v1;
                UINT_800b51e0 = 0;
                spA4 = (temp_v1 * 2) + rspMtxs;
                block = gBlocksToDraw[temp_v1];
            }

            //@recomp
            if (gDLBuilder->fogColor != fogColourPrev){
                dl_set_fog_color(&gMainDL, 
                    (fogColourPrev & 0xFF000000) >> (8*3), 
                    (fogColourPrev & 0xFF0000) >> (8*2), 
                    (fogColourPrev & 0xFF00) >> (8*1), 
                    (fogColourPrev & 0xFF)
                );
            }

            shape = &block->shapes[shapeIdx];
            if (shape->flags & RENDER_UNK20000000) {
                if (spA3 != 2) {
                    gSPMatrix(gMainDL++, OS_K0_TO_PHYSICAL(&spA4[1]), G_MTX_MODELVIEW | G_MTX_LOAD);
                    spA3 = 2;
                }
            } else if (spA3 != 1) {
                gSPMatrix(gMainDL++, OS_K0_TO_PHYSICAL(spA4), G_MTX_MODELVIEW | G_MTX_LOAD);
                spA3 = 1;
            }

            if (shape->materialIndex == 0xFF) {
                tex0 = NULL;
            } else {
                tex0 = block->materials[shape->materialIndex].texture;
            }

            if (shape->flags & RENDER_WATER) {
                if ((tex0->flags & 0xC000) && !(shape->flags & RENDER_WATER)) {
                    dl_set_prim_color(&gMainDL, 0xFF, 0xFF, 0xFF, 160);
                } else {
                    dl_set_prim_color(&gMainDL, 0xFF, 0xFF, 0xFF, 100);
                }
            } else {
                if (shape->envColourMode == 0xFF) {
                    dl_set_prim_color(&gMainDL, colourR, colourG, colourB, 0xFF);
                } else if (shape->envColourMode == 0xFE) {
                    dl_set_prim_color(&gMainDL, 0xFF, 0xFF, 0xFF, 0xFF);
                } else if (shape->flags & (RENDER_UNK40000000 | RENDER_UNK4000000 | RENDER_UNK2000000 | RENDER_UNK800000 | RENDER_UNK400000)) {
                    dl_set_prim_color(&gMainDL, 0xFF, 0xFF, 0xFF, 0xFF);
                } else {
                    func_8001F848(&gMainDL);
                }
            }
            
            renderFlags = shape->flags;
            if (renderFlags & RENDER_UNK10000) {
                temp_v0_4 = func_8004A284(block, shape->animatorID);
                if (temp_v0_4 != NULL) {
                    frameValue = gBlockTextures[temp_v0_4->textureIndex].unk4 << 8;
                    renderFlags |= gBlockTextures[temp_v0_4->textureIndex].flags;
                } else {
                    frameValue = 0;
                }

                if ((shape->animatorID != SHORT_800b51dc) || (frameValue != UINT_800b51e0)) {
                    SHORT_800b51dc = shape->animatorID;
                    UINT_800b51e0 = frameValue;
                    spE4 = 1;
                }
            } else {
                SHORT_800b51dc = -1;
                frameValue = 0;
            }

            if (shape->blendMaterialIndex != 0xFF) {
                tex1 = block->materials[shape->blendMaterialIndex].texture;
            } else {
                tex1 = NULL;
            }

            tex_gdl_set_textures(&gMainDL, tex0, tex1, renderFlags, frameValue, spE4, 0);
            
            if (shape->unk16 != 0xFF) {
                texScroll = func_80049D68(shape->unk16);
                gDPSetTileSize(gMainDL++, 0, texScroll->uOffsetA, texScroll->vOffsetA, (tex0->width - 1) << 2, (tex0->height - 1) << 2);
                if (tex1 != NULL) {
                    gDPSetTileSize(gMainDL++, 1, texScroll->uOffsetB, texScroll->vOffsetB, (tex1->width - 1) << 2, (tex1->height - 1) << 2);
                }
            } else if ((tex0 != NULL) && (tex0->flags & 0xC000)) {
                gDPSetTileSize(gMainDL++, 0, 0, 0, (tex0->width - 1) << 2, (tex0->height - 1) << 2);
                if (tex1 != NULL) {
                    gDPSetTileSize(gMainDL++, 1, 0, 0, (tex1->width - 1) << 2, (tex1->height - 1) << 2);
                }
            }

            f3dIdx = (shape - block->shapes) * 3;
            gMainDL->words.w0 = block->gdlGroups[f3dIdx].words.w0;
            gMainDL->words.w1 = block->gdlGroups[f3dIdx].words.w1;
            f3dIdx++;
            dl_apply_geometry_mode(&gMainDL);

            gMainDL->words.w0 = block->gdlGroups[f3dIdx].words.w0;
            gMainDL->words.w1 = block->gdlGroups[f3dIdx].words.w1;
            f3dIdx++;
            dl_apply_combine(&gMainDL);
            
            gMainDL->words.w0 = block->gdlGroups[f3dIdx].words.w0;
            gMainDL->words.w1 = block->gdlGroups[f3dIdx].words.w1;
            dl_apply_other_mode(&gMainDL);

            tempVtx = block->vertices2[(block->vtxFlags & 1) ^ 1];
            tempTri = block->encodedTris;
            tempTri += shape->triBase;
            endTri = block->encodedTris;
            endTri += shape[1].triBase;
            baseDL = gMainDL;

            gSPVertex2(gMainDL++, OS_K0_TO_PHYSICAL(&tempVtx[shape->vtxBase]), shape[1].vtxBase - shape->vtxBase, 0);

            tri = NULL;
            while ((u32) tempTri < (u32) endTri) {
                if (tempTri->d1 & 1) {
                    if (tri == NULL) {
                        tri = tempTri;
                    } else {
                        gSP2TrianglesBlock(gMainDL, tri->d0, tempTri->d0);
                        gMainDL++;
                        tri = NULL;
                    }
                }
                tempTri++;
            }

            if ((tri != NULL) && (tri->d1 & 1)) {
                gSP1TriangleBlock(gMainDL, tri->d0);
                gMainDL++;
            }

            gDLBuilder->needsPipeSync = TRUE;

            //Draw fog separately for decals (redraws the faces with different settings)
            //This is necessary because decals use vertex alpha, which can't be used at the same time as fog
            //(when using fog, the RSP replaces each pixel's A with the Z-depth value)
            if ((renderFlags & (RENDER_DECAL | RENDER_DECAL_SIMPLE | RENDER_FOG_ACTIVE)) == (RENDER_DECAL | RENDER_DECAL_SIMPLE | RENDER_FOG_ACTIVE)) {
                
                cmdCount = gMainDL - baseDL;
                dl_set_geometry_mode(&gMainDL, G_FOG);
                
                if (renderFlags & (RENDER_WATER | RENDER_SEMI_TRANSPARENT)) {
                    //Semi-transparent decals
                    // (e.g. cross-fading two sections of water)

                    s32 override = get_override_fog();
                    s32 twoCycle = get_two_cycle();
                    
                    s32 blend1 = get_fog_param(0);
                    s32 blend2 = get_fog_param(1);
                    s32 blend3 = get_fog_param(2);
                    s32 blend4 = get_fog_param(3);
                    
                    s32 blend21 = get_fog2_param(0);
                    s32 blend22 = get_fog2_param(1);
                    s32 blend23 = get_fog2_param(2);
                    s32 blend24 = get_fog2_param(3);

                    //@recomp
                    f32 r = ((f32)get_fog_colour_component(0))/100.0f;
                    f32 g = ((f32)get_fog_colour_component(1))/100.0f;
                    f32 b = ((f32)get_fog_colour_component(2))/100.0f;
                    f32 a = ((f32)get_fog_colour_component(3))/100.0f;
                    fogColourPrev = gDLBuilder->fogColor;

                    if (override) {
                        dl_set_fog_color(&gMainDL, 
                            (u8)(((fogColourPrev & 0xFF000000) >> (8*3))*r), 
                            (u8)(((fogColourPrev & 0xFF0000) >> (8*2))*g), 
                            (u8)(((fogColourPrev & 0xFF00) >> (8*1))*b), 
                            (u8)((fogColourPrev & 0xFF)*a)
                        );

                        gDPSetCombineLERP(gMainDL, 
                            0, 0, 0, 1, 0, 0, 0, 0, 
                            0, 0, 0, 1, 0, 0, 0, 0
                        );
                    } else {
                        gDPSetCombineLERP(gMainDL, 
                            0, 0, 0, 1, 0, 0, 0, 1, 
                            0, 0, 0, 1, 0, 0, 0, 1
                        );
                    }
                    dl_apply_combine(&gMainDL);

                    if (override) {
                        gDPSetOtherMode(
                            gMainDL, 
                            G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_POINT | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | (twoCycle ? G_CYC_2CYCLE : G_CYC_1CYCLE) | G_PM_NPRIMITIVE, 
                            G_AC_NONE | G_ZS_PIXEL | Z_CMP | IM_RD | CVG_DST_FULL | ZMODE_XLU | FORCE_BL | 
                            // GBL_c1(blend1, blend2, blend3, blend4) | 
                            // (twoCycle ? GBL_c2(blend21, blend22, blend23, blend24) : GBL_c2(blend1, blend2, blend3, blend4))
                            GBL_c1(G_BL_CLR_FOG, G_BL_A_FOG, G_BL_CLR_MEM, G_BL_1MA) | 
                            GBL_c2(G_BL_CLR_FOG, G_BL_A_FOG, G_BL_CLR_MEM, G_BL_1MA)
                        );
                    } else {
                        gDPSetOtherMode(
                            gMainDL, 
                            G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_POINT | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | G_CYC_1CYCLE | G_PM_NPRIMITIVE, 
                            G_AC_NONE | G_ZS_PIXEL | AA_EN | Z_CMP | IM_RD | CLR_ON_CVG | CVG_DST_FULL | ZMODE_XLU | FORCE_BL | 
                            GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_MEM, G_BL_1MA) | 
                            GBL_c2(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_MEM, G_BL_1MA)
                        );
                    }
                    dl_apply_other_mode(&gMainDL);
                } else {
                    //Opaque decals 
                    //(e.g. cross-fading two opaque textures, like grassy ground fading into leafy autumnal ground)
                    gDPSetCombineLERP(gMainDL, 
                        0, 0, 0, 1, 0, 0, 0, 1, 
                        0, 0, 0, 1, 0, 0, 0, 1
                    );
                    dl_apply_combine(&gMainDL);

                    gDPSetOtherMode(
                        gMainDL, 
                        G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_POINT | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | G_CYC_1CYCLE | G_PM_NPRIMITIVE, 
                        G_AC_NONE | G_ZS_PIXEL | AA_EN | Z_CMP | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_DEC | FORCE_BL | 
                        GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_MEM, G_BL_1MA) | 
                        GBL_c2(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_MEM, G_BL_1MA)
                    );
                    dl_apply_other_mode(&gMainDL);
                }

                //Redraw the previous facebatch by copying its commands
                bcopy(baseDL, gMainDL, cmdCount * sizeof(Gfx));
                gMainDL += cmdCount;
                gDLBuilder->needsPipeSync = TRUE;
            }
        }
    }
}

static void track_c_func_MODIFIED(void) {
    s32 sp294;
    Block* block;
    s32 temp_v1;
    s8 *var_s8;
    s32 temp_v0;
    s32 sp274[4];
    s32 sp264[4];
    s32 sp254[4];
    s32 sp244[4];
    s32 sp240;
    s32 var_s2;
    s32 var_s3;
    s32 temp_s1;
    s8* sp230;
    u8 sp130[BLOCKS_GRID_TOTAL_CELLS];
    s8 sp7C[180];
    Mtx* sp78;

    dl_add_debug_info(gMainDL, 0, "track/track.c", 0x52B);
    some_cell_func(&D_800B9780);
    shadows_func_8004D9B8();
    shadows_func_8004DABC();
    gRenderListLength = 1;
    gBlocksToDrawIdx = 0;
    dl_add_debug_info(gMainDL, 0, "track/track.c", 0x53D);
    gDLL_9_Newclouds->vtbl->func6(&gMainDL, gUpdateRate, 0);
    gDLL_15_Projgfx->vtbl->func5(&gMainDL, &gWorldRSPMatrices, &D_800B51D4, 3);
    if (UINT_80092a98 & 0x10) {
        gDLL_12_Minic->vtbl->func3(&gMainDL, &gWorldRSPMatrices);
    }
    dl_add_debug_info(gMainDL, 0, "track/track.c", 0x545);
    var_s8 = D_80092B0C;
    sp78 = gWorldRSPMatrices;
    sp240 = ARRAYCOUNT(gBlockIndices);
    while (--sp240 >= 0) {
        sp230 = gBlockIndices[sp240];
        D_800B9714 = D_800B9700[sp240];
        func_80047404(gMapCurrentStreamCoordsX + 7, gMapCurrentStreamCoordsZ + 7, sp274, sp264, sp254, sp244, sp240, 1, D_800B4A54);
        for (temp_v1 = 0; temp_v1 < (s32)ARRAYCOUNT(sp130); temp_v1++) { sp130[temp_v1] = 0; }
        
        for (var_s2 = sp274[2]; sp274[3] >= var_s2; var_s2++) {
            for (temp_s1 = sp274[0]; sp274[1] >= temp_s1; temp_s1++) {
                sp130[(temp_s1 + 7) + ((var_s2 + 7) << 4)] = 1;
            }
        }
        for (var_s2 = sp264[2]; sp264[3] >= var_s2; var_s2++) {
            for (temp_s1 = sp264[0]; sp264[1] >= temp_s1; temp_s1++) {
                sp130[(temp_s1 + 7) + ((var_s2 + 7) << 4)] = 1;
            }
        }
        for (var_s2 = sp254[2]; sp254[3] >= var_s2; var_s2++) {
            for (temp_s1 = sp254[0]; sp254[1] >= temp_s1; temp_s1++) {
                sp130[(temp_s1 + 7) + ((var_s2 + 7) << 4)] = 1;
            }
        }
        for (var_s2 = sp244[2]; sp244[3] >= var_s2; var_s2++) {
            for (temp_s1 = sp244[0]; sp244[1] >= temp_s1; temp_s1++) {
                sp130[(temp_s1 + 7) + ((var_s2 + 7) << 4)] = 1;
            }
        }
        // @fake
        if (sp240){}

        for (sp294 = 0; sp294 < BLOCKS_GRID_SPAN; sp294++) {
            temp_s1 = var_s8[sp294];
            for (var_s3 = 0; var_s3 < BLOCKS_GRID_SPAN; var_s3++) {
                var_s2 = var_s8[var_s3];
                temp_v1 = GRID_INDEX(var_s2, temp_s1);
                temp_v0 = sp230[temp_v1];
                if (temp_v0 < 0) {
                    block = NULL;
                } else {
                    block = gLoadedBlocks[temp_v0];
                    block->vtxFlags ^= 1;
                    if (sp130[temp_v1] == 0) {
                        continue;
                    }
                }
                if (temp_v0 < 0 || func_800451A0(temp_s1, var_s2, block) == 0) {
                    continue;
                }
                D_800B97B8 = temp_s1 * BLOCKS_GRID_UNIT_F;
                D_800B97BC = var_s2 * BLOCKS_GRID_UNIT_F;
                func_80043950(block, temp_s1, var_s2, sp240);
                if (UINT_80092a98 & 0x8000) {
                    if (block->unk3E != 0) {
                        block_compute_vertex_colors(block, temp_s1, var_s2, 0);
                    }
                    if ((block->unk49 != 0) && (UINT_80092a98 & 0x100)) {
                        func_8001F4C0(block, temp_s1, var_s2);
                    }
                }
                block_add_to_render_list(block, D_800B97B8, D_800B97BC);
            }
        }
    }
    func_80043FD8(sp7C);
    draw_render_list_MODIFIED(sp78, sp7C);
    dl_add_debug_info(gMainDL, 0, "track/track.c", 0x5B2);
    gDLL_15_Projgfx->vtbl->func5(&gMainDL, &gWorldRSPMatrices, &D_800B51D4, 2);
    gDLL_15_Projgfx->vtbl->func5(&gMainDL, &gWorldRSPMatrices, &D_800B51D4, 1);
    gDLL_14_Modgfx->vtbl->func11(sp7C);
    gDLL_14_Modgfx->vtbl->func6(&gMainDL, &gWorldRSPMatrices, &D_800B51D4, 0, 0);
    gDLL_24_Waterfx->vtbl->func_C7C(&gMainDL, &gWorldRSPMatrices);
    gDLL_15_Projgfx->vtbl->func5(&gMainDL, &gWorldRSPMatrices, &D_800B51D4, 0);
    gDLL_2_Camera->vtbl->lock_icon_print(&gMainDL, &gWorldRSPMatrices, &D_800B51D4, &D_800B51D8);
    gDLL_59_Minimap->vtbl->func1(&gMainDL, &gWorldRSPMatrices);
    shadows_func_8004D974(0);
    D_800B1847 = 0;
    dl_add_debug_info(gMainDL, 0, "track/track.c", 0x5C6);
}

void func_8004225C_MODIFIED(Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, Vertex** vtxs2, Triangle** pols2) {
    Mtx* mtx;

    gMainDL = *gdl;
    gWorldRSPMatrices = *mtxs;
    D_800B51D4 = *vtxs;
    D_800B51D8 = *pols;
    UINT_80092a98 |= 0x21;
    if ((gMapType == MAPTYPE_MOBILE) || (gMapType == MAPTYPE_3)) {
        UINT_80092a98 &= ~1;
    }
    gSPTexture(gMainDL++, -1, -1, 3, 0, 1);
    mtx = get_some_model_view_mtx();
    gSPMatrix(gMainDL++, OS_K0_TO_PHYSICAL(mtx), G_MTX_MODELVIEW | G_MTX_LOAD);
    camera_setup_viewport_and_matrices(&gMainDL, 0);
    func_80044BEC();
    if (func_80010048() != 0) {
        if (!(UINT_80092a98 & 8)) {
            UINT_80092a98 |= 8;
        }
        camera_set_aspect(1.7777778f);
    } else if (UINT_80092a98 & 8) {
        UINT_80092a98 &= ~8;
        camera_set_aspect(1.3333334f);
    }
    if (UINT_80092a98 & 0x10000) {
        if (UINT_80092a98 & 8) {
            camera_set_aspect(1.7777778f);
        } else {
            camera_set_aspect(1.3333334f);
        }
        viewport_disable(get_camera_selector(), 0U);
        vi_some_video_setup(0);
        UINT_80092a98 &= ~0x10000;
    }
    if (UINT_80092a98 & 0x10) {
        setup_rsp_camera_matrices(&gMainDL, &gWorldRSPMatrices);
        gDLL_7_Newday->vtbl->func13(&gMainDL, &gWorldRSPMatrices);

        if (UINT_80092a98 & 0x40) {
            gDLL_10_Newstars->vtbl->func1(&gMainDL);
        }
        gDLL_7_Newday->vtbl->func3(&gMainDL, &gWorldRSPMatrices, UINT_80092a98 & 0x40);
    } else {
        setup_rsp_camera_matrices(&gMainDL, &gWorldRSPMatrices);
    }
    gDLL_11_Newlfx->vtbl->func2();
    gDLL_57->vtbl->func3();
    gDLL_58->vtbl->func2();
    if (UINT_80092a98 & 0x20000) {
        if (gDLL_7_Newday->vtbl->func23(&gMainDL) == 0) {
            gDLL_8->vtbl->func3(&gMainDL);
        }
    } else {
        gDLL_8->vtbl->func3(&gMainDL);
    }
    D_800B51E4 = get_camera();
    func_80048F58();
    track_c_func_MODIFIED();
    gDLL_9_Newclouds->vtbl->func4(&gMainDL);
    camera_setup_fullscreen_viewport(&gMainDL);
    *gdl = gMainDL;
    *mtxs = gWorldRSPMatrices;
    *vtxs = D_800B51D4;
    *pols = D_800B51D8;
    UINT_80092a98 &= ~2;
    // @fake
    if (1) { } if (1) { } if (1) { } if (1) { }
    // diProfEnd("Trackdraw") (default.dol)
}

RECOMP_PATCH void dl_apply_geometry_mode(Gfx **gdl)
{
    u8 dirty;

    if (UINT_80092a98 & 0x2000) {
        (**gdl).words.w1 &= ~0x1;
    }

    // (**gdl).words.w1 |= 1;
    // (**gdl).words.w1 |= 4;

    if (gDLBuilder->dirtyFlags & DIRTY_FLAGS_GEOMETRY_MODE) {
        gDLBuilder->dirtyFlags &= ~DIRTY_FLAGS_GEOMETRY_MODE;
        dirty = TRUE;
    } else {
        dirty = gDLBuilder->geometryMode != (**gdl).words.w1;
    }

    if (dirty) {
        gDLBuilder->geometryMode = (**gdl).words.w1;
        (*gdl)++;
    }
}
