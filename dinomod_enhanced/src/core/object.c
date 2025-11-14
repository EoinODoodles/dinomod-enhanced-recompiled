#include "modding.h"

#include "PR/ultratypes.h"

extern s16 *gFile_OBJINDEX;
extern int gObjIndexCount;

RECOMP_HOOK_RETURN("init_objects") void init_objects_return_hook(void) {
    // @recomp: Change all -1 OBJINDEX.bin mappings to DummyObject. Otherwise, attempting to load
    //          one of those object IDs will result in a crash since the object setup code will
    //          return null and pretty much no part of the game is coded to expect a null there.
    for (s32 i = 0; i < gObjIndexCount; i++) {
        if (gFile_OBJINDEX[i] == -1) {
            gFile_OBJINDEX[i] = 0;
        }
    }
}
