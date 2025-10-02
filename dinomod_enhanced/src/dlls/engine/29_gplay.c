#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "sys/map_enums.h"

#include "recomp/dlls/engine/29_gplay_recomp.h"

extern u16 sMapObjGroupBitKeys[];

/** Modifies the flagIDs used to track maps' objectGroup load states (originally by MusicalProgrammer) */
RECOMP_HOOK_DLL(gplay_ctor) void gplay_patch_map_object_group_flags() {
    sMapObjGroupBitKeys[MAP_EARTHWALKER_TEMPLE] = 0x36a; //Shares Walled City's flag
    sMapObjGroupBitKeys[MAP_BOSS_KAMERIAN_DRAGON] = 0x5dc; //Shares the same flag as the rest of Dragon Rock (Bottom)
}
