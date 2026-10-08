#include "modding.h"

#include "dlls/engine/29_gplay.h"
#include "dlls/engine/53_movelib.h"
#include "game/gamebits.h"
#include "recomputils.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "dll.h"

#include "recomp/dlls/objects/400_CFLevelControl_recomp.h"

static _Bool recomp_sWaterDrainSeqStarted = FALSE;

RECOMP_HOOK_DLL(CFLevelControl_ctor) void CFLevelControl_ctor_hook(void) {
    // Reset custom statics
    recomp_sWaterDrainSeqStarted = FALSE;
}

RECOMP_HOOK_RETURN_DLL(CFLevelControl_obj_Control) void recomp_CFLevelControl_guardianSeqObjGroupToggle(void) {
    // Enable CFGuardian objgroup while the water draining seq plays, otherwise he will not show up.
    // This must run after CFLevelControl_doDistBasedObjGroupToggling to work correctly.
    if (mainGetBits(BIT_CRF_Throne_Room_Quest_Complete) != 0 && mainGetBits(BIT_Played_Seq_02B2_CF_Courtyard_Water_Drains) == 0) {
        dll_gplay->set_obj_group_status(MAP_CLOUDRUNNER_DUNGEON, 3, 1);
    }

    // Set a savepoint after the courtyard water draining seq completes
    if (mainGetBits(BIT_CRF_Throne_Room_Quest_Complete) && !mainGetBits(BIT_Played_Seq_02B2_CF_Courtyard_Water_Drains)) {
        recomp_sWaterDrainSeqStarted = TRUE;
    } else if (recomp_sWaterDrainSeqStarted && mainGetBits(BIT_Played_Seq_02B2_CF_Courtyard_Water_Drains) && !mainGetBits(BIT_CRF_Activate_Kyte_Gold_Perch)) {
        recomp_sWaterDrainSeqStarted = FALSE;

        SRT transform;
        if (((DLL_53_movelib*)(gTempDLLInsts[1]))->vtbl->func7(27, &transform)) {
            dll_gplay->savepoint(&transform.transl, transform.yaw, 0, mapGetLayer());
        }
    }
}
