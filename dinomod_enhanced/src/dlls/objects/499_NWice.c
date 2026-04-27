#include "modding.h"

#include "game/objects/object.h"
#include "sys/objtype.h"

#include "recomp/dlls/objects/499_NWice_recomp.h"

RECOMP_PATCH void dll_499_setup(Object* self, ObjSetup* setup, s32 arg2) {
    // @recomp: Don't mark self as invisible, otherwise the player will not draw while standing on the ice
    //self->srt.flags |= OBJFLAG_INVISIBLE;
    self->unkB0 |= 0x2000;
    obj_add_object_type(self, OBJTYPE_61);
}
