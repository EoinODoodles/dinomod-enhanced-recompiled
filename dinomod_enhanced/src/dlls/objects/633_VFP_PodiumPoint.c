#include "modding.h"

#include "PR/ultratypes.h"
#include "game/objects/object.h"
#include "recomputils.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "dll.h"

#include "recomp/dlls/objects/633_VFP_PodiumPoint_recomp.h"

extern s32 data_0;

extern void VFP_PodiumPoint_func_1B8(Object* self);

RECOMP_PATCH void VFP_PodiumPoint_control(Object* self) {
    u8 mapSetupID;

    mapSetupID = gDLL_29_Gplay->vtbl->get_map_setup(self->mapID);
    switch (mapSetupID) {
    default:
        data_0 = 0x123;
        break;
    case 1:
        data_0 = 0x22B; // @recomp: Accept DIM's activated SpellStone instead of the unactivated one
        break;
    case 2:
        data_0 = 0x83B;
        break;
    case 3:
        data_0 = 0x83C;
        break;
    }
    VFP_PodiumPoint_func_1B8(self);
}
