#include "modding.h"

#include "custom_gamebits.h"

#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "dll.h"

#include "recomp/dlls/objects/512_SHLevelControl_recomp.h"

static void dinomod_river_control(void) {
    _Bool boulderBlownUp = main_get_bits(DINOMOD_BIT_920_SH_BoulderBlownUp);
    // TODO: temporary, link boulder bit to river bit. we can separate these if a seq is made for fixing the river
    main_set_bits(DINOMOD_BIT_921_SH_RiverUnblocked, boulderBlownUp);

    _Bool riverUnblocked = main_get_bits(DINOMOD_BIT_921_SH_RiverUnblocked);
    _Bool dimSpellStoneActivated = main_get_bits(BIT_SpellStone_DIM_Activated);

    // Link custom object groups to the normal groups used to cull objects around the river.
    // We'll show the custom groups only if at least one of these are active.
    _Bool riverObjGroupActive = gDLL_29_Gplay->vtbl->get_obj_group_status(MAP_SWAPSTONE_HOLLOW, 6) || 
        gDLL_29_Gplay->vtbl->get_obj_group_status(MAP_SWAPSTONE_HOLLOW, 7);

    // Enable obj group for river related objects that should only load if the river is unblocked
    gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_HOLLOW, 11, 
        (riverObjGroupActive && riverUnblocked) ? 1 : 0);
    
    // Enable obj group for river related objects that should only load if the player has access to Diamond Bay
    gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_HOLLOW, 12, 
        (riverObjGroupActive && riverUnblocked && dimSpellStoneActivated) ? 1 : 0);
}

RECOMP_HOOK_DLL(SHLevelControl_control) void SHLevelControl_control_hook(Object *self) {
    // Drive AMSFX's waterfall sfx logic. This is necessary to stop the waterfallspray sfx that plays
    // in the DB river when coming back to SH.
    gDLL_6_AMSFX->vtbl->water_falls_control();

    // Handle river related stuff
    dinomod_river_control();
}
