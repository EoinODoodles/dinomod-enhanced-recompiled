#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "sys/map_enums.h"
#include "sys/main.h"

#include "recomp/dlls/engine/29_gplay_recomp.h"

extern u16 sMapObjGroupBitKeys[];

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
    if (get_gplay_bitstring(0x910) && get_gplay_bitstring(0x919) == 0){
        set_gplay_bitstring(0x919, 1);
    }
}
