#include "sidekick_util.h"

#include "game/objects/object_id.h"
#include "recomputils.h"
#include "sys/objects.h"
#include "sys/map.h"

extern MapHeader* gLoadedMapsDataTable[120];

// C port of MusicalProgrammer's function to unload sidekicks when the map they are on(?) unloads.
s32 dinomod_unload_sidekick_if_map_unloaded(Object *obj) {
    MapHeader *mapHeader;
    void *var_a2;
    s32 i;

    if (obj->id == OBJ_Tricky) {
        var_a2 = *((void**)((u32)obj->data + 0x46C));
    } else if (obj->id == OBJ_Kyte) {
        var_a2 = *((void**)((u32)obj->data + 0xA4));

        if (((u32)obj->data + 0x1F4) == (u32)var_a2) {
            return 1;
        }
    } else {
        return 1;
    }

    if (var_a2 == NULL) {
        return 1;
    }

    // note(shinx): Changed the loop count from 97 to 120 to match the actual length of gLoadedMapsDataTable
    for (i = 0; i < 120; i++) {
        mapHeader = gLoadedMapsDataTable[i];

        if (mapHeader != NULL) {
            if (((u8*)var_a2 >= (u8*)mapHeader->objectInstanceFile_ptr) && (u8*)var_a2 < (u8*)mapHeader->grid_B1_ptr) {
                // Found in loaded map objects
                return 1;
            }
        }
    }

    obj_destroy_object(obj);
    return 0;
}
