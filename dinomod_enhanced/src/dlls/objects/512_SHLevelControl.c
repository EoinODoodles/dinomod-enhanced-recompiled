#include "modding.h"
#include "recomputils.h"

#include "custom_gamebits.h"

#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "dll.h"
#include "dlls/objects/common/sidekick.h"

#include "recomp/dlls/objects/512_SHLevelControl_recomp.h"

static _Bool player_has_spellstone(void) {
    return (main_get_bits(BIT_SpellStone_DIM_Activated) && !main_get_bits(BIT_877))
        || (main_get_bits(BIT_SpellStone_WC) && !main_get_bits(BIT_DB_Unlock_Act_Three))
        || (main_get_bits(BIT_7CC)); // Note: DR SpellStone has no hide bit, instead it gets unset after VFP act 3
}

static _Bool player_is_postgame(void) {
    return main_get_bits(BIT_7AF); // Drakor defeated bit
}

static void dinomod_river_control(void) {
    _Bool boulderBlownUp = main_get_bits(DINOMOD_BIT_920_SH_BoulderBlownUp);
    // TODO: temporary, link boulder bit to river bit. we can separate these if a seq is made for fixing the river
    main_set_bits(DINOMOD_BIT_921_SH_RiverUnblocked, boulderBlownUp);

    _Bool riverUnblocked = main_get_bits(DINOMOD_BIT_921_SH_RiverUnblocked);

    // Link custom object groups to the normal groups used to cull objects around the river.
    // We'll show the custom groups only if at least one of these are active.
    _Bool riverObjGroupActive = gDLL_29_Gplay->vtbl->get_obj_group_status(MAP_SWAPSTONE_HOLLOW, 6) || 
        gDLL_29_Gplay->vtbl->get_obj_group_status(MAP_SWAPSTONE_HOLLOW, 7);

    // Enable obj group for river related objects that should only load if the river is unblocked
    gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_HOLLOW, 11, 
        (riverObjGroupActive && riverUnblocked) ? 1 : 0);
    
    // Enable obj group for river related objects that should only load if the player has access to Diamond Bay
    gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_HOLLOW, 12, 
        (riverObjGroupActive && riverUnblocked && (player_has_spellstone() || player_is_postgame())) ? 1 : 0);
}

#define VINE_AREA_MIN_X -7744
#define VINE_AREA_MIN_Z -2284
#define VINE_AREA_MAX_X -7038
#define VINE_AREA_MAX_Z -1556

static void dinomod_well_control(void) {
    Object* player = get_player();
    
    //Don't run if the player isn't in the well
    if (!player || player->srt.transl.y > -845.0f) {
        return;
    }
    
    // Temporary: set gamebit if the player issued Flame command near the lily pond vines
    // (can be removed in future if we create a curve network for Tricky to follow in the well)    
    _Bool lilyPondVinesGone = main_get_bits(DINOMOD_BIT_922_SH_Well_LilyPondVinesUnblocked);
    if (lilyPondVinesGone == FALSE) {

        //Check if Flame was used
        if (gDLL_1_cmdmenu->vtbl->was_this_item_used(Sidekick_Command_INDEX_4_Flame)){

            //Check that player's roughly in the area of the vines
            if (
                ((VINE_AREA_MIN_X < player->srt.transl.x) && (player->srt.transl.x < VINE_AREA_MAX_X)) &&
                ((VINE_AREA_MIN_Z < player->srt.transl.z) && (player->srt.transl.z < VINE_AREA_MAX_Z))
            ) {
                main_set_bits(DINOMOD_BIT_922_SH_Well_LilyPondVinesUnblocked, TRUE);
            }
        }
    }
}

RECOMP_HOOK_DLL(SHLevelControl_setup) void SHLevelControl_setup_hook(Object *self, ObjSetup *setup, s32 arg2) {
    // Wake up trader Thorntail automatically if DIM SpellStone was obtained as a hint to the player
    // that they should explore the burrows and (hopefully) find the explosive barrel.
    if (main_get_bits(BIT_SpellStone_DIM_Activated)) {
        main_set_bits(BIT_14, 1);
    }
}

RECOMP_HOOK_DLL(SHLevelControl_control) void SHLevelControl_control_hook(Object *self) {
    // Drive AMSFX's waterfall sfx logic. This is necessary to stop the waterfallspray sfx that plays
    // in the DB river when coming back to SH.
    gDLL_6_AMSFX->vtbl->water_falls_control();

    // Handle river related stuff
    dinomod_river_control();

    // Handle well related stuff
    dinomod_well_control();
}
