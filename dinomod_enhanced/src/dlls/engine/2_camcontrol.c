#include "modding.h"
#include "recomputils.h"
#include "dll_util.h"
#include "sidekick_util.h"

#include "game/objects/object_id.h"
#include "sys/dll.h"
#include "sys/objects.h"
#include "sys/objtype.h"

#include "dlls/engine/2_camcontrol.h"

#include "recomp/dlls/engine/2_camcontrol_recomp.h"

static CamControl_Data* sCamData;

RECOMP_PATCH void CamControl_set_target_flag_2(s32 enable) {
    // @recomp: Return early if in Galadon fight (removes forced z-targeting)
    {
        Object **objectList;
        s32 count;

        objectList = obj_get_all_of_type(OBJTYPE_4, &count);

        for (s32 i = 0; i < count; i++) {
            if (objectList[i]->id == OBJ_DIM_Boss) {
                return;
            }
        }
    }

    if (enable) {
        sCamData->targetFlags |= 2;
    } else {
        sCamData->targetFlags &= ~2;
    }
}
