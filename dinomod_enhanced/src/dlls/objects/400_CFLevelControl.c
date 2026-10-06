#include "modding.h"

#include "dlls/engine/29_gplay.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "dll.h"

#include "recomp/dlls/objects/400_CFLevelControl_recomp.h"

RECOMP_HOOK_RETURN_DLL(CFLevelControl_obj_Control) void recomp_CFLevelControl_guardianSeqObjGroupToggle(void) {
    // Enable CFGuardian objgroup while the water draining seq plays, otherwise he will not show up.
    // This must run after CFLevelControl_doDistBasedObjGroupToggling to work correctly.
    if (mainGetBits(BIT_CRF_Throne_Room_Quest_Complete) != 0 && mainGetBits(BIT_Played_Seq_02B2_CF_Courtyard_Water_Drains) == 0) {
        dll_gplay->set_obj_group_status(MAP_CLOUDRUNNER_DUNGEON, 3, 1);
    }
}
