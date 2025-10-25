#include "modding.h"

#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/map.h"

RECOMP_HOOK("func_80027934") void galleon_staff_collision_hack(Object* obj, Object* otherObj) {
    // @recomp: Turn off collision between Krystal's staff and the Galleon (original patch by MusicalProgrammer)
    if (otherObj->id == OBJ_staff && D_800B4A50 == MAP_FRONT_END) {
        // Note: we technically only need to set this once, maybe there's a better way to do this?
        otherObj->objhitInfo->unk5A = 0;
    }
}
