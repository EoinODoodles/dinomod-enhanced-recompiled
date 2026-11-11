#pragma once

#include "dlls/engine/29_gplay.h"

#define DEFAULT_LACTIONS { { -1, -1, -1, -1}, { -1, -1 }, -1, -1, 0, -1 }
#define DEFAULT_ENVACTIONS { 43000.0f,  -1, -1, -1,  -1, -1, -1,  0, 0, 0,  0, 0, 0,  0, 0, 0,  {0},  1, -1, -1, -1 }

enum ObjGroupStatusBits {
    G_None = 0,
    G0 = (1 << 0),
    G1 = (1 << 1),
    G2 = (1 << 2),
    G3 = (1 << 3),
    G4 = (1 << 4),
    G5 = (1 << 5),
    G6 = (1 << 6),
    G7 = (1 << 7),
    G8 = (1 << 8),
    G9 = (1 << 9),
    G10 = (1 << 10),
    G11 = (1 << 11),
    G12 = (1 << 12),
    G13 = (1 << 13),
    G14 = (1 << 14),
    G15 = (1 << 15),
    G16 = (1 << 16),
    G17 = (1 << 17),
    G18 = (1 << 18),
    G19 = (1 << 19),
    G20 = (1 << 20),
    G21 = (1 << 21),
    G22 = (1 << 22),
    G23 = (1 << 23),
    G24 = (1 << 24),
    G25 = (1 << 25),
    G26 = (1 << 26),
    G27 = (1 << 27),
    G28 = (1 << 28),
    G29 = (1 << 29),
    G30 = (1 << 30),
    G31 = (1 << 31)
};

typedef void (*ChapterCallback)(void);

typedef struct {
    const char* name;
    ChapterCallback callback;
} Chapter;

Chapter* chselect_get_chapters(s32* count);

void chselect_set_player(s32 playerno);
void chselect_set_location(s32 playerno, f32 x, f32 y, f32 z, s16 yaw, s32 layer);
void chselect_set_music(s32 playerno, s32 ambience1, s32 ambience2, s32 music1, s32 music2);
void chselect_set_light(s32 playerno, PlayerLightActions* actions);
void chselect_set_env(s32 playerno, PlayerEnvActions* actions);

void chselect_set_map_state(s32 mapID, s32 act, enum ObjGroupStatusBits groupBits);
void chselect_move_setup(s32 uid, s32 mapID, f32 x, f32 y, f32 z);
/** in timesave units, not ticks */
void chselect_set_timesave(s32 uid, f32 time);

void chselect_set_min_magic(s32 playerno, s32 magic);
void chselect_set_min_scarabs(s32 playerno, s32 scarabs);
