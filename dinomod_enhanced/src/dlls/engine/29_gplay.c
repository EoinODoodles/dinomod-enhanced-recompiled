#include "modding.h"
#include "recomputils.h"
#include "recompsavedata.h"

#include "engine/29_gplay_dinomod.h"

#include "PR/ultratypes.h"
#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/29_gplay.h"
#include "game/gamebits.h"
#include "sys/map_enums.h"
#include "sys/main.h"

static _Bool recomp_sSavepointGotoRequested = FALSE;
static _Bool recomp_sRestartPointGotoRequested = FALSE;

#include "recomp/dlls/engine/29_gplay_recomp.h"

extern u16 sMapObjGroupBitKeys[];

extern GplayOptions *sGameOptions;
extern s8 sSavegameIdx;
extern GplaySaveFlash *sSavegame;
extern Savegame *sRestartSave;
extern GameState sState;

extern void gplay_start_game(void);

RECOMP_HOOK_DLL(gplay_ctor) void gplay_ctor_hook(void) {
    // Reset custom static variables
    recomp_sSavepointGotoRequested = FALSE;
    recomp_sRestartPointGotoRequested = FALSE;
}

/** Modifies the flagIDs used to track maps' objectGroup load states (originally by MusicalProgrammer) */
RECOMP_HOOK_DLL(gplay_ctor) void gplay_patch_map_object_group_flags(void) {
    sMapObjGroupBitKeys[MAP_EARTHWALKER_TEMPLE] = BIT_WC_ObjGroup_Bits; //Shares Walled City's gamebit
    sMapObjGroupBitKeys[MAP_BOSS_KAMERIAN_DRAGON] = BIT_DR_Bottom_ObjGroup_Bits; //Shares the same gamebit as the rest of Dragon Rock (Bottom)
}

/** Checks if the Scarab collection cutscene has already played, and if so unlocks the Scarab UI
  * (NOTE: the Scarab object DLL has also been patched to set the Scarab UI bit upon collection, so no need to check on every update!
  *  This is mostly intended for players who start out playing unmodded, since it'll put the Scarab UI into its correct state) 
  */
RECOMP_HOOK_RETURN_DLL(gplay_start_game) void dll_gplay_hook_enable_scarabs_if_already_collected(void) {
    if (mainGetBits(BIT_Tutorial_Collected_Scarab) && mainGetBits(BIT_UI_Scarab_Counter_Enabled) == FALSE){
        mainSetBits(BIT_UI_Scarab_Counter_Enabled, TRUE);
    }
}

/**
  * Ensures the player doesn't start at 0 health after saving and quitting on the Game Over screen
  */
RECOMP_HOOK_RETURN_DLL(gplay_start_game) void hook_start_game_fix_zero_health(void) {
    PlayerStats* stats = gDLL_29_Gplay->vtbl->get_player_stats();

    //Set health to 3 apples when reloading after saving on the Game Over screen
    if (stats && (stats->health <= 0)) {
        stats->health = 3 * 4;
    }
}

/**
  * Prevents the language from resetting to English, and ensures the languageID is known.
  */
RECOMP_PATCH u32 gplay_load_game_options(void) {
    u32 ret;
    s32 loadStatus;
    
    ret = 1;

    loadStatus = gDLL_31_Flash->vtbl->load_game(
        sGameOptions, 3, sizeof(GplayOptions), FALSE);
    
    if (!loadStatus) {
        // Failed to load
        // "gplayLoadOptions error: saveoptions failed to load.\n" (default.dol)
        bzero(sGameOptions, sizeof(GplayOptions));
        ret = 0;
        sGameOptions->volumeMusic = MAX_VOLUME;
        sGameOptions->volumeAudio = MAX_VOLUME;
        sGameOptions->unkA = 0x7f;
    }

    //@recomp: prevent language from resetting to English, and make sure it's a known languageID
    if (sGameOptions->languageID < 0 || sGameOptions->languageID > LANGUAGE_JAPANESE) {
        sGameOptions->languageID = 0;
    }
    
    if (sGameOptions->screenOffsetX < -7) {
        sGameOptions->screenOffsetX = -7;
    }
    if (sGameOptions->screenOffsetX > 7) {
        sGameOptions->screenOffsetX = 7;
    }
    if (sGameOptions->screenOffsetY < -7) {
        sGameOptions->screenOffsetY = -7;
    }
    if (sGameOptions->screenOffsetY > 7) {
        sGameOptions->screenOffsetY = 7;
    }

    return ret;
}

/**
 * Normally, timesaves and saved objects (moved objsetups) *always* have their latest state
 * persisted to flash when the player saves the game, regardless of when the last savepoint
 * was triggered. This causes many many issues with state being desync'd upon reloading the
 * save often causing softlocks. The below hooks effectively change gplay_save_game to copy
 * around those savefile fields before saving to flash (this patch is done indirectly in recomp
 * as gplay_save_game is a base recomp patch and cannot be overwritten directly here).
 *
 * The result of these changes is that timesaves and saved objects are bound to savepoints
 * (i.e. they work like savetype 1 bits now).
 */
static void* recomp_sBackedUpFileState = NULL;

RECOMP_HOOK_DLL(gplay_save_game) void hook_backup_timesaves_and_setupmoves(void) {
    if (sSavegameIdx != -1) {
        if (recomp_sBackedUpFileState != NULL) {
            recomp_free(recomp_sBackedUpFileState);
            recomp_sBackedUpFileState = NULL;
        }

        s32 backupSize = OFFSETOF(Savefile, bitString) - OFFSETOF(Savefile, numSavedObjects);
        recomp_sBackedUpFileState = recomp_alloc(backupSize);
        bcopy(&sSavegame->asSave.file.numSavedObjects, recomp_sBackedUpFileState, backupSize);
    }
}

RECOMP_SAVEDATA_ON_SAVE void restore_backup_timesaves_and_setupmoves(void) {
    if (recomp_sBackedUpFileState != NULL) {
        s32 backupSize = OFFSETOF(Savefile, bitString) - OFFSETOF(Savefile, numSavedObjects);
        bcopy(recomp_sBackedUpFileState, &sSavegame->asSave.file.numSavedObjects, backupSize);

        //recomp_printf("restoring gplay timesaves/objmoves\n");

        recomp_free(recomp_sBackedUpFileState);
        recomp_sBackedUpFileState = NULL;
    }
}

/** Defer savepoint loads */
RECOMP_PATCH void gplay_start_loaded_game(void) {
    recomp_sSavepointGotoRequested = TRUE;
    recomp_sRestartPointGotoRequested = FALSE;
}

/** Defer restart point loads */
RECOMP_PATCH void gplay_restart_goto(void) {
    recomp_sSavepointGotoRequested = FALSE;
    recomp_sRestartPointGotoRequested = TRUE;
}

/**
 * Normally, when a savepoint or restart point is loaded the global gplay state is swapped *immediately*. Loading
 * either of these does not, however, free objects or reload the map until the end of the game tick. The result is
 * that objects that haven't ran their tick yet after a savepoint/restart point load will be reading/writing the
 * loaded state before the game actually reloads everything. This can result in game state being messed up depending
 * on how early in the tick the savepoint/restart point is loaded.
 *
 * This behavior is patched to instead queue up the load until the end of the tick (see main.c patches).
 */
void dinomod_gplay_handle_goto(void) {
    if (recomp_sSavepointGotoRequested) {
        // do real gplay_start_loaded_game
        bcopy(&sSavegame->asSave, &sState.save, sizeof(Savegame));
        gplay_start_game();
    } else if (recomp_sRestartPointGotoRequested) {
        // do real gplay_restart_goto
        if (sRestartSave != NULL) {
            bcopy(sRestartSave, &sState.save, sizeof(Savegame));
            gplay_start_game();
        } else {
            // restore default.dol printf
            recomp_eprintf("WARNING gplay : Restart Point Not Set \n");
        }
    }

    recomp_sSavepointGotoRequested = FALSE;
    recomp_sRestartPointGotoRequested = FALSE;
}
