#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "sys/map_enums.h"
#include "sys/main.h"
#include "dll.h"

#include "recomp/dlls/engine/29_gplay_recomp.h"

extern u16 sMapObjGroupBitKeys[];

extern GplayOptions *sGameOptions;

/** Modifies the flagIDs used to track maps' objectGroup load states (originally by MusicalProgrammer) */
RECOMP_HOOK_DLL(gplay_ctor) void gplay_patch_map_object_group_flags() {
    sMapObjGroupBitKeys[MAP_EARTHWALKER_TEMPLE] = 0x36a; //Shares Walled City's flag
    sMapObjGroupBitKeys[MAP_BOSS_KAMERIAN_DRAGON] = 0x5dc; //Shares the same flag as the rest of Dragon Rock (Bottom)
}

/** Checks if the Scarab collection cutscene has already played, and if so unlocks the Scarab UI
  * (NOTE: the Scarab object DLL has also been patched to set the Scarab UI bit upon collection, so no need to check on every update!
  *  This is mostly intended for players who start out playing unmodded, since it'll put the Scarab UI into its correct state) 
  */
RECOMP_HOOK_RETURN_DLL(gplay_start_game) void dll_gplay_hook_enable_scarabs_if_already_collected() {
    if (main_get_bits(0x910) && main_get_bits(0x919) == 0){
        main_set_bits(0x919, 1);
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
        sGameOptions->volumeMusic = 0x7f;
        sGameOptions->volumeAudio = 0x7f;
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
