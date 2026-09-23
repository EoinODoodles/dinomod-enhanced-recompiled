/** Various helpers for working with game Maps */

#include "reasset.h"
#include "recomputils.h"

#include "common_objsetups.h"
#include "map_util.h"

#include "sys/camera.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/print.h"

/* Logs the visGrid ranges of one cell of a particular map, from one of the map's 4 visGrid files. */
s32 mapUtil_printVisGridCell(MapVisGrids grid, MapIDs mapID, u8 x, u8 z) {
    u32 cellIdx;
    u32 size;
    VisGridCellROM* cell;
    ReAssetID map = reasset_base_id(mapID);
    MapHeader* header = reasset_maps_get_header(map);

    switch (grid) {
    case VisGrid_A1:
        cell = reasset_maps_get_grid_a1(map, &size);
        break;
    case VisGrid_B1:
        cell = reasset_maps_get_grid_b1(map, &size);
        break;
    case VisGrid_A2:
        cell = reasset_maps_get_grid_a2(map, &size);
        break;
    case VisGrid_B2:
        cell = reasset_maps_get_grid_b2(map, &size);
        break;
    }

    if (cell == NULL) {
        recomp_eprintf("visGrid file empty/not found! Map: %d, visGrid: %d\n", mapID, grid);
        return 1;
    }

    cellIdx = VISGRID_CELL_IDX(map, x, z);

    recomp_printf("Cell %d %d) ", x, z);
    for (u32 r = 0; r < 4; r++) {
        VisGridRangeROM* range = &cell[cellIdx].range[r];
        recomp_printf("range%d: %x %x %x %x ", 
            r, 
            range->xMin, range->zMin, range->xMax, range->zMax
        );
    }
    recomp_printf("\n");

    return 0;
}

/* Logs the visGrid ranges of every cell in a particular map, from one of the map's 4 visGrid files. */
void mapUtil_printVisGrid(MapVisGrids grid, MapIDs mapID) {
    u8 width;
    u8 depth;
    u32 size;
    MapHeader* header = reasset_maps_get_header(reasset_base_id(mapID));

    width = header->gridSizeX;
    depth = header->gridSizeZ;

    recomp_printf("VisGrid: Map %d\n", mapID);
    for (u32 i = 0; i < width * depth; i++) {
        if (mapUtil_printVisGridCell(grid, mapID, i % width, i / width) != 0) {
            break;
        }
    }
}

// #define DEBUG_GRID

#ifdef DEBUG_GRID
RECOMP_CALLBACK("*", recomp_on_game_tick_start) void mapUtil_printCurrentMapCell(void) {
    extern MapHeader* gLoadedMapsDataTable[120];
    Object* player;
    s32 mapID;
    MapHeader* map;
    s32 localGridX;
    s32 localGridZ;

    player = objGetPlayer();
    if (player == NULL) {
        return;
    }

    mapID = mapWorldXZToMapID(player->globalPosition.x, player->globalPosition.z);
    map = gLoadedMapsDataTable[mapID];
    if (map == NULL) {
        return;
    }

    localGridX = ((player->globalPosition.x - map->originWorldX)/BLOCKS_GRID_UNIT) + map->originOffsetX;
    localGridZ = ((player->globalPosition.z - map->originWorldZ)/BLOCKS_GRID_UNIT) + map->originOffsetZ;

    diPrintf("Map cell: %d %d\n", localGridX, localGridZ);
}
#endif
