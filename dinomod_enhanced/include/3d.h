// From https://github.com/Francessco121/dino-recomp-mods/blob/main/shinx121_debug/src/3d.h

#pragma once

#include "PR/ultratypes.h"
#include "sys/math.h"

void draw_3d_line(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2, u32 color);
void draw_3d_text(f32 x, f32 y, f32 z, const char *text, u32 color);
void draw_3d_cube(f32 x, f32 y, f32 z, f32 size, u32 color);
void draw_3d_box(MtxF *mtx, f32 width, f32 height, f32 depth, u32 color);
void draw_3d_sphere_ext(f32 x, f32 y, f32 z, f32 radius, u32 color, s32 numSegments, f32 thickness, _Bool filled);
void draw_3d_sphere(f32 x, f32 y, f32 z, f32 radius, u32 color);
