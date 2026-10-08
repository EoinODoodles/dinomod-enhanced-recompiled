#include "modding.h"

#include "game/objects/object.h"
#include "dll.h"

#include "recomp/dlls/objects/406_CFCheapGalleon_recomp.h"

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 fullGalleonObjGroup;      //The map objectGroup for the full-detail Galleon and its objects
    s16 unk1A;
    s16 unk1C;
    s16 gamebitForceLowDetail;   //The cheap Galleon stays at max opacity and exits control early when this gamebit is set
} CFCheapGalleon_Setup;

/** Always show the high-detail Galleon. The low-detail version looks awful in recomp. */
RECOMP_PATCH void CFCheapGalleon_obj_Setup(Object* self, CFCheapGalleon_Setup* objSetup, s32 reset) {}

RECOMP_PATCH void CFCheapGalleon_obj_Control(Object* self) {
    CFCheapGalleon_Setup* objSetup = (CFCheapGalleon_Setup*)self->setup;

    self->opacity = 0;
    if (!gDLL_29_Gplay->vtbl->get_obj_group_status(self->mapID, objSetup->fullGalleonObjGroup)) {
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, objSetup->fullGalleonObjGroup, 1);
    }
}
RECOMP_PATCH void CFCheapGalleon_obj_Update(Object* self) {}

/** Ensure high-detail Galleon isn't loaded more often than it needs to be. */
RECOMP_HOOK_DLL(CFCheapGalleon_obj_Free) void CFCheapGalleon_obj_Free(Object* self) {
    CFCheapGalleon_Setup* objSetup = (CFCheapGalleon_Setup*)self->setup;

    gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, objSetup->fullGalleonObjGroup, 0);
}
