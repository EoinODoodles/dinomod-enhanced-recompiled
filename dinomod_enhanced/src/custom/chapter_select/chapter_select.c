#include "modding.h"
#include "dbgui.h"
#include "recomputils.h"

#include "chapter_select.h"
#include "dinomod_dbgui.h"

#include "PR/ultratypes.h"
#include "dlls/engine/29_gplay.h"
#include "game/gamebits.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/math.h"
#include "sys/menu.h"
#include "sys/main.h"
#include "dll.h"
#include "macros.h"

static s32 windowOpen = FALSE;
static s32 queuedChapterSelect = -1;

static GameState* get_gplay_state(void);
static void reload_from_gplay_state(void);
static void gplay_act_reset(void);
static void set_act(s32 mapID, s32 act);
static void set_group_bits(s32 mapID, enum ObjGroupStatusBits groupBits);

void chselect_set_player(s32 playerno) {
    GameState* state = get_gplay_state();
    state->save.file.playerno = playerno;
}

void chselect_set_location(s32 playerno, f32 x, f32 y, f32 z, s16 yaw, s32 layer) {
    GameState* state = get_gplay_state();
    PlayerLocation* loc = &state->save.map.playerLocations[playerno];
    loc->vec.x = x;
    loc->vec.y = y;
    loc->vec.z = z;
    loc->rotationY = (u8)(yaw >> 8);
    loc->mapLayer = layer;
}

void chselect_set_music(s32 playerno, s32 ambience1, s32 ambience2, s32 music1, s32 music2) {
    GameState* state = get_gplay_state();
    PlayerMusicActions* actions = &state->save.map.musicActions[playerno];
    actions->actionNums[0] = ambience1;
    actions->actionNums[1] = ambience2;
    actions->actionNums[2] = music1;
    actions->actionNums[3] = music2;
}

void chselect_set_light(s32 playerno, PlayerLightActions* actions) {
    GameState* state = get_gplay_state();
    PlayerLightActions* dst = &state->save.map.lightActions[playerno];
    bcopy(actions, dst, sizeof(PlayerLightActions));
}

void chselect_set_env(s32 playerno, PlayerEnvActions* actions) {
    GameState* state = get_gplay_state();
    PlayerEnvActions* dst = &state->save.map.envActions[playerno];
    bcopy(actions, dst, sizeof(PlayerEnvActions));
}

void chselect_set_map_state(s32 mapID, s32 act, enum ObjGroupStatusBits groupBits) {
    set_act(mapID, act);
    set_group_bits(mapID, groupBits);
}

void chselect_move_setup(s32 uid, s32 mapID, f32 x, f32 y, f32 z) {
    ObjSetup setup = {0};
    setup.uID = uid;
    mapSaveObject(&setup, mapID, x, y, z);
}

void chselect_set_timesave(s32 uid, f32 time) {
    gDLL_29_Gplay->vtbl->add_time(uid, time);
}

void chselect_set_min_magic(s32 playerno, s32 magic) {
    GameState* state = get_gplay_state();
    PlayerStats* playerStats = &state->save.file.players[playerno];
    playerStats->magic = MAX(playerStats->magic, magic);
    playerStats->magic = MIN(playerStats->magic, playerStats->magicMax);
}

void chselect_set_min_scarabs(s32 playerno, s32 scarabs) {
    GameState* state = get_gplay_state();
    PlayerStats* playerStats = &state->save.file.players[playerno];
    playerStats->scarabs = MAX(playerStats->scarabs, scarabs);
    playerStats->scarabsLargestRecent = playerStats->scarabs;

    if (playerno == PLAYER_KRYSTAL) {
        mainSetBits(BIT_Scarab_Count_Krystal, playerStats->scarabs);
    } else {
        mainSetBits(BIT_Scarab_Count_Sabre, playerStats->scarabs);
    }
}

// pretty much the same as gplay_init_save
static void init_save(Savegame* savegame, const char* filename) {
    savegame->file.playerno = PLAYER_KRYSTAL;

    for (s32 i = 0; i < 2; i++) {
        savegame->file.players[i].health = 12;
        savegame->file.players[i].healthMax = 12;
        savegame->file.players[i].magicMax = 25;
        savegame->file.players[i].magic = 0;

        savegame->file.sidekicks[i].blueFood = 4;
        savegame->file.sidekicks[i].redFood = 5;

        savegame->map.lightActions[i].skylights[0] = -1;
        savegame->map.lightActions[i].skylights[1] = -1;
        savegame->map.lightActions[i].skylights[2] = -1;
        savegame->map.lightActions[i].skylights[3] = -1;
        savegame->map.lightActions[i].trklights[0] = -1;
        savegame->map.lightActions[i].trklights[1] = -1;
        savegame->map.lightActions[i].unkC = -1;
        savegame->map.lightActions[i].newlfx = -1;
        savegame->map.lightActions[i].cam = -1;

        savegame->map.envActions[i].unk0 = 43000.0f;
        savegame->map.envActions[i].unk4 = -1;
        savegame->map.envActions[i].unk6 = -1;
        savegame->map.envActions[i].unk8 = -1;
        savegame->map.envActions[i].unkA = -1;
        savegame->map.envActions[i].unkC = -1;
        savegame->map.envActions[i].unkE = -1;
        savegame->map.envActions[i].unk3D = -1;
        savegame->map.envActions[i].unk3E = -1;
        savegame->map.envActions[i].unk3F = -1;
        savegame->map.envActions[i].unk3C = 1;

        for (s32 k = 0; k < 4; k++) {
            savegame->map.musicActions[i].actionNums[k] = -1;
        }
    }

    gplay_act_reset();

    mainSetBits(BIT_Tricky_Spawns, 1);

    bcopy(filename, savegame->file.name, sizeof(savegame->file.name));

    // ship battle
    Vec3f startPos = VEC3F(-14149.352f, -82.0f, -15569.178f);
    gDLL_29_Gplay->vtbl->savepoint(&startPos, 0, 0, 0);;
}

static void goto_chapter(s32 index) {
    GameState* state = get_gplay_state();
    Savegame* savegame = &state->save;

    char prevFilename[6];
    s32 prevTimePlayed = savegame->file.timePlayed;
    bcopy(savegame->file.name, prevFilename, sizeof(prevFilename));

    bzero(savegame, sizeof(*savegame));
    init_save(savegame, prevFilename);
    savegame->file.timePlayed = prevTimePlayed + 1;

    s32 numChapters;
    Chapter* chapters = chselect_get_chapters(&numChapters);

    for (s32 i = 0; i <= index && i < numChapters; i++) {
        Chapter* chapter = &chapters[i];
        if (chapter->callback != NULL) {
            chapter->callback();
        }
    }

    reload_from_gplay_state();
}

void dinomod_dbgui_chapter_select_menu(void) {
    dbgui_menu_item("Chapter Select", &windowOpen);
}

RECOMP_CALLBACK(".", dinomod_dbgui_event) void chapter_select_dinomod_dbgui_callback(void) {
    if (windowOpen) {
        if (dbgui_begin("Chapter Select", &windowOpen)) {
            s32 currMenu = menuGetCurrent();
            _Bool disabled = currMenu >= MENU_POST && currMenu <= MENU_OPTIONS;
            
            s32 numChapters;
            Chapter* chapters = chselect_get_chapters(&numChapters);

            if (dbgui_begin_child("scroll")) {
                dbgui_text_wrapped("Warning: Selecting a chapter will overwrite your save file!");

                for (s32 i = 0; i < numChapters; i++) {
                    dbgui_push_str_id(recomp_sprintf_helper("ch%d", i));

                    Chapter* chapter = &chapters[i];
                    if (chapter->callback == NULL) {
                        dbgui_separator_text(chapter->name);
                    } else {
                        if (disabled) {
                            dbgui_begin_disabled(TRUE);
                        }
                        if (dbgui_button("->")) {
                            queuedChapterSelect = i;
                        }
                        dbgui_same_line();
                        dbgui_text(chapter->name);
                        if (disabled) {
                            dbgui_end_disabled();
                        }
                    }

                    dbgui_pop_id();
                }
            }
            dbgui_end_child();
        }
        dbgui_end();
    }
}

RECOMP_HOOK_RETURN("mmFreeTick") void chapter_select_tick(void) {
    if (queuedChapterSelect != -1) {
        goto_chapter(queuedChapterSelect);
        queuedChapterSelect = -1;
    }
}

#include "recomp/dlls/engine/29_gplay_recomp.h"

typedef struct {
    s8 mapID;
    s8 act;
} MapAct;

typedef struct {
    s32 mapID;
    // Bitfield of the status of each object group
    s32 groupBits;
} MapObjGroupBits;

extern u16 sMapActBitKeys[120];
extern u16 sMapObjGroupBitKeys[120];

extern GplaySaveFlash* sSavegame;
extern GameState sState;
extern MapAct sMapAct;
extern MapObjGroupBits sMapObjGroups;

extern void gplay_start_game(void);
extern void gplay_save_game(void);

static GameState* get_gplay_state(void) {
    return &sState;
}

static void reload_from_gplay_state(void) {
    gDLL_29_Gplay->vtbl->restart_clear();
    bcopy(&sState.save, &sSavegame->asSave, sizeof(Savegame));
    
    sMapAct.mapID = -1;
    sMapObjGroups.mapID = -1;

    bzero(&sState.bitString, sizeof(sState.bitString));

    mainUnpause();

    dll_task->LoadRecentlyCompleted();

    main_func_800142A0(
        sState.save.map.playerLocations[sState.save.file.playerno].vec.x,
        sState.save.map.playerLocations[sState.save.file.playerno].vec.y,
        sState.save.map.playerLocations[sState.save.file.playerno].vec.z);
    
    if (menuGetCurrent() != MENU_TITLE_SCREEN) {
        menuSet(MENU_GAMEPLAY);
    }
    menuDoMenuSwap();

    gDLL_28_ScreenFade->vtbl->fade_reversed(40, SCREEN_FADE_BLACK);
}

static void gplay_act_reset(void) {
    for (s32 i = 0; i < 120; i++) {
        if (sMapActBitKeys[i] != 0) {
            gDLL_29_Gplay->vtbl->set_act(i, 1);
        }
    }
}

static void set_act(s32 mapID, s32 act) {
    mainSetBits(sMapActBitKeys[mapID], act);
}

static void set_group_bits(s32 mapID, enum ObjGroupStatusBits groupBits) {
    mainSetBits(sMapObjGroupBitKeys[mapID], groupBits);
}
