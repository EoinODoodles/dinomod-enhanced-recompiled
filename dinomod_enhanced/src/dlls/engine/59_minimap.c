#include "configs.h"
#include "dll_util.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "rt64_extended_gbi.h"

#include "PR/os.h"
#include "PR/ultratypes.h"
#include "macros.h"
#include "types.h"
#include "game/gamebits.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "dll.h"
#include "dlls/engine/59_minimap.h"

#include "recomp/dlls/engine/59_minimap_recomp.h"

#include "engine/1_cmdmenu.h"
#include "engine/59_minimap.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

extern s32 sLoadedTexTableID;       //Current map tile's index in TEXTABLE.bin (maps a texture in TEX0.bin)
extern u8 sMinimapVisible;
extern s16 sOpacity;
extern Texture* sMapTile;           //current map texture
extern Texture* sMarkerSidekick;    //green dot texture
extern Texture* sMarkerPlayer;      //blue diamond texture
extern s8 sOffsetX;
extern s8 sOffsetY;
extern u8 sGridX;
extern u8 sGridZ;
extern s16 sLevelMaxX;
extern s16 sLevelMaxZ;
extern s16 sLevelMinX;
extern s16 sLevelMinZ;

extern MinimapLevel sMinimapLevels[30];

extern s32 D_8008C890;

/** Hijack the print function (Base Recomp already patches it) */
typedef s32 (*MinimapPrint)(Gfx **gdl, s32 arg1);
static MinimapPrint print_func; 
static s32 minimap_print_custom(Gfx **gdl, s32 arg1);

RECOMP_HOOK_DLL(minimap_ctor) void minimap_ctor_hook(DLLFile *dll) {
    print_func = dinomod_hijack_dll_export(dll, 1, minimap_print_custom);
}

RECOMP_HOOK_RETURN_DLL(minimap_dtor) void minimap_dtor_hook() {
    print_func = NULL;
}

static s32 minimap_print_custom(Gfx **gdl, s32 arg1) {
    Object* player;
    Object* sidekick;
    s32 loadTextureID;
    s16 playerX;
    s16 playerZ;
    s16 playerY;
    u8 index;
    u8 index2;
    u16 texTableID;
    s16 tileCount;
    MinimapSection* mapTiles;
    u8 mapID;
    u8 tileIndex;
    s8 mapFound;
    MinimapLevel* mapLevel;
    MinimapSection* tile;

    loadTextureID = 0;
    mapFound = FALSE;
    index = 0;
    index2 = 0;
    mapID = 0;
    player = get_player();
    sidekick = get_sidekick();

    if (player == NULL) {
        return 0;
    }

    //Get mapID from player's coords, or from the mobile map Object they're parented to
    if (player->parent) {
        mapID = player->parent->mapID;
    } else {
        mapID = map_get_map_id_from_xz_ws(player->srt.transl.x, player->srt.transl.z);
    }

    //Iterate over the map definitions until the one with the relevant mapID is found
    while (mapFound == FALSE && (u32)index < ARRAYCOUNT(sMinimapLevels)){
        if (mapID == sMinimapLevels[index].mapID) { 
            mapFound = TRUE;
        } else {
            index++;
        }
    }
    
    if (mapFound) {
        playerX = player->globalPosition.x;
        playerZ = player->globalPosition.z;
        playerY = player->globalPosition.y;
        
        //Iterate over the map's tiles, until finding a tile whose bounding volume contains the player coords
        mapLevel = &sMinimapLevels[index];
        mapTiles = mapLevel->tiles;
        for (mapFound = FALSE, index = 0; index2 < mapLevel->tileCount; index2++){
            if ((playerX >= mapTiles[index2].minX) && (playerX < mapTiles[index2].maxX) && 
                (playerZ >= mapTiles[index2].minZ) && (playerZ < mapTiles[index2].maxZ) && 
                (playerY >= mapTiles[index2].minY) && (playerY < mapTiles[index2].maxY) && 
                main_get_bits(mapTiles[index2].gamebitSection)) { //Make sure the tile's gamebit is set

                tileIndex = 0;
                mapFound = TRUE;
                loadTextureID = 0;
                //Store the tile's texTableID, for loading a TEX0 minimap texture
                if (mapTiles[index2].texTableID) {
                    loadTextureID = mapTiles[index2].texTableID;
                }
                
                //@bug?: calculates the bounds every frame
                if (sLoadedTexTableID == mapTiles[index2].texTableID) {
                    tileCount = mapLevel->tileCount;
                    sLevelMaxX = -0x8000;
                    sLevelMaxZ = -0x8000;
                    sLevelMinX = 0x7FFF;
                    sLevelMinZ = 0x7FFF;

                    //Getting minX/maxX, minZ/maxZ for whole map
                    while (tileIndex < tileCount){
                        if (loadTextureID == mapTiles[tileIndex].texTableID) {
                            sLevelMinX = (mapTiles[tileIndex].minX < sLevelMinX) ? mapTiles[tileIndex].minX : sLevelMinX;
                            sLevelMaxX = (mapTiles[tileIndex].maxX > sLevelMaxX) ? mapTiles[tileIndex].maxX : sLevelMaxX;
                            sLevelMinZ = (mapTiles[tileIndex].minZ < sLevelMinZ) ? mapTiles[tileIndex].minZ : sLevelMinZ;
                            sLevelMaxZ = (mapTiles[tileIndex].maxZ > sLevelMaxZ) ? mapTiles[tileIndex].maxZ : sLevelMaxZ;
                        }
                        tileIndex++;
                    }

                    //Getting gridX and gridZ
                    sGridX = ((sLevelMaxX - sLevelMinX) * 8) / BLOCKS_GRID_UNIT;
                    if (sGridX > 24) {
                        sGridX = 24;
                    }
                    sGridZ = ((sLevelMaxZ - sLevelMinZ) * 8) / BLOCKS_GRID_UNIT;
                    if (sGridZ > 24) {
                        sGridZ = (sGridZ * 2) - 24;
                    }

                    //Store screen offset for the tile
                    sOffsetX = mapTiles[index2].screenOffsetX;
                    sOffsetY = mapTiles[index2].screenOffsetY;
                }
                break;                
            }
        }
        
    }
    
    //Set/get minimap gamebits
    main_set_bits(BIT_Toggle_Minimap, mapFound);
    if (sMinimapVisible == FALSE || main_get_bits(BIT_Hide_Minimap)) {
        loadTextureID = 0;
    }
    
    //Hide during cutscenes
    if (camera_get_letterbox()) {
        loadTextureID = 0;
        sOpacity = 0;
    }

    //@recomp: fade out if cmdmenu item info pop-up should appear
    if (recomp_get_config_u32("cmdmenu_info_popup_fix") &&
        cmdmenu_info_popup_is_visible()
    ) {
        sOpacity -= 0x10 * gUpdateRate;
        if (sOpacity < 0) {
            sOpacity = 0;
        }
    } else {
        //Fade minimap in/out
        if (loadTextureID == sLoadedTexTableID) {
            // sOpacity += 0x20; //@framerate-dependent
            sOpacity += 0x10 * gUpdateRate; //@recomp: fix framerate dependency
            if (sOpacity > 0xFF) {
                sOpacity = 0xFF;
            }
        } else {
            // sOpacity -= 0x20; //@framerate-dependent
            sOpacity -= 0x10 * gUpdateRate; //@recomp: fix framerate dependency
            if (sOpacity < 0) {
                sOpacity = 0;

                if (sMapTile && (loadTextureID || sOpacity == 0)) {
                    tex_free(sMapTile);
                    sMapTile = NULL;
                    sLoadedTexTableID = 0;
                }

                if (loadTextureID) {
                    sMapTile = tex_load_deferred(loadTextureID);
                    sLoadedTexTableID = loadTextureID;
                }
            }
        }
    }

    if (sMapTile && sOpacity) {
        #ifndef DINOMOD_ROM_PATCH
        {
            // @recomp: Fullscreen scissor
            gEXPushScissor((*gdl)++);
            gEXSetScissorAlign((*gdl)++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, 0, -SCREEN_WIDTH, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            gDPSetScissor((*gdl)++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            // @recomp: Align minimap to left
            gEXSetViewportAlign((*gdl)++, G_EX_ORIGIN_LEFT, 0, 0);
            gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, 0, 0, 0, 0);
        }
        #endif

        //Draw minimap tile
        rcp_screen_full_write(gdl, sMapTile, 
                (MINIMAP_SCREEN_X + sOffsetX - sGridX),
                (MINIMAP_SCREEN_Y + sOffsetY - sGridZ),
                0, 0, sOpacity, SCREEN_WRITE_TRANSLUCENT);

        //Draw player marker
        #ifdef DINOMOD_ROM_PATCH
        {
            if (D_8008C890) {
                //Widescreen aspect
                rcp_screen_full_write(gdl, sMarkerPlayer, 
                    (MINIMAP_SCREEN_X - sGridX - (player->globalPosition.x - sLevelMaxX) * 0.02f) - 3.0f, //@rom-patch: widescreen, scale down along X to suit aspect
                    (MINIMAP_SCREEN_Y - sGridZ - (player->globalPosition.z - sLevelMaxZ) * 0.025f) - 4.0f,
                    0, 0, sOpacity, SCREEN_WRITE_TRANSLUCENT
                );
            } else {
                //Standard aspect
                rcp_screen_full_write(gdl, sMarkerPlayer, 
                    (MINIMAP_SCREEN_X - sGridX - (player->globalPosition.x - sLevelMaxX) * 0.025f) - 4.0f,
                    (MINIMAP_SCREEN_Y - sGridZ - (player->globalPosition.z - sLevelMaxZ) * 0.025f) - 4.0f,
                    0, 0, sOpacity, SCREEN_WRITE_TRANSLUCENT
                );
            }
        }
        #else
        rcp_screen_full_write(gdl, sMarkerPlayer, 
            (MINIMAP_SCREEN_X - sGridX - (player->globalPosition.x - sLevelMaxX) * 0.025f) - 4.0f,
            (MINIMAP_SCREEN_Y - sGridZ - (player->globalPosition.z - sLevelMaxZ) * 0.025f) - 4.0f,
            0, 0, sOpacity, SCREEN_WRITE_TRANSLUCENT
        );
        #endif

        //Draw sidekick marker (if the sidekick's somewhere inside the current map's extended bounds)
        if (sidekick != NULL) {
            if (
                (sLevelMinX - BLOCKS_GRID_UNIT_HALF < sidekick->globalPosition.x) && (sidekick->globalPosition.x < sLevelMaxX + BLOCKS_GRID_UNIT_HALF) &&
                (sLevelMinZ - BLOCKS_GRID_UNIT_HALF < sidekick->globalPosition.z) && (sidekick->globalPosition.z < sLevelMaxZ + BLOCKS_GRID_UNIT_HALF)
            ) {
                #ifdef DINOMOD_ROM_PATCH
                {
                    if (D_8008C890) {
                        //Widescreen aspect
                        rcp_screen_full_write(gdl, sMarkerSidekick, 
                            (MINIMAP_SCREEN_X - sGridX - (sidekick->globalPosition.x - sLevelMaxX) * 0.020f) - 3.0f, //@rom-patch: widescreen, scale down along X to suit aspect
                            (MINIMAP_SCREEN_Y - sGridZ - (sidekick->globalPosition.z - sLevelMaxZ) * 0.025f) - 4.0f,
                            0, 0, sOpacity, SCREEN_WRITE_TRANSLUCENT
                        );
                    } else {
                        //Standard aspect
                        rcp_screen_full_write(gdl, sMarkerSidekick, 
                            (MINIMAP_SCREEN_X - sGridX - (sidekick->globalPosition.x - sLevelMaxX) * 0.025f) - 4.0f,
                            (MINIMAP_SCREEN_Y - sGridZ - (sidekick->globalPosition.z - sLevelMaxZ) * 0.025f) - 4.0f,
                            0, 0, sOpacity, SCREEN_WRITE_TRANSLUCENT
                        );
                    }
                }
                #else
                rcp_screen_full_write(gdl, sMarkerSidekick, 
                    (MINIMAP_SCREEN_X - sGridX - (sidekick->globalPosition.x - sLevelMaxX) * 0.025f) - 4.0f,
                    (MINIMAP_SCREEN_Y - sGridZ - (sidekick->globalPosition.z - sLevelMaxZ) * 0.025f) - 4.0f,
                    0, 0, sOpacity, SCREEN_WRITE_TRANSLUCENT
                );
                #endif
            }
        }

        #ifndef DINOMOD_ROM_PATCH
        {
            // @recomp: Reset alignment
            gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
            gEXSetViewportAlign((*gdl)++, G_EX_ORIGIN_NONE, 0, 0);
            // @recomp: Reset scissor align
            gEXSetScissorAlign((*gdl)++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            gEXPopScissor((*gdl)++);
        }
        #endif
    }

    return 0;
}

/** 
  * Custom pseudo-export, so the cmdmenu can check the minimap's opacity
  * (to avoid clash between minimap and item info pop-up). 
  */
s16 minimap_get_opacity() {
    return sOpacity;
}
