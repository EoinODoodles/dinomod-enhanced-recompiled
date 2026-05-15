#include "modding.h"

#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "dll.h"

#include "recomp/dlls/objects/512_SHLevelControl_recomp.h"
#include "sys/map_enums.h"

RECOMP_HOOK_DLL(SHLevelControl_control) void SHLevelControl_control_hook(Object *self) {
    _Bool riverUnblocked = main_get_bits(0x123);
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
