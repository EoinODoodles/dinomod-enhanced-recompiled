#pragma once

#include "PR/ultratypes.h"
#include "sys/gfx/texture.h"
#include "dlls/engine/59_minimap.h"

#define MINIMAP_SCREEN_X 50
#define MINIMAP_SCREEN_Y 200
#define NO_MAP_ID -1

typedef struct {
    s16 minX; //Section's bounds in worldSpace
    s16 maxX; //Section's bounds in worldSpace
    s16 minZ; //Section's bounds in worldSpace
    s16 maxZ; //Section's bounds in worldSpace
    s16 minY; //Section's bounds in worldSpace
    s16 maxY; //Section's bounds in worldSpace
    u16 gamebitSection; //For toggling a specific section of a map (e.g. deciding when to switch between the exterior/interior maps of LightFoot Village's ring fort)
    s8 screenOffsetX; //For repositioning map tiles' framing on screen
    s8 screenOffsetY; //For repositioning map tiles' framing on screen
    u16 texTableID; //Index into TEXTABLE.bin, used to find the texture's index in TEX0.bin
} MinimapSection;

typedef struct {
    MinimapSection* tiles;
    u16 gamebitLevel; //Overall gamebitID for having obtained the level's map (not checked)
    u8 mapID;
    u8 tileCount;
} MinimapLevel;

s16 minimap_get_opacity();
