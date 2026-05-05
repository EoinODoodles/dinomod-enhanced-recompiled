#include "modding.h"

#include "common.h"
#include "sys/objanim.h"
#include "sys/objtype.h"

#include "recomp/dlls/objects/615_crawler_recomp.h"

//Removes an if condition that prevents memory being freed (originally by MusicalProgrammer)
RECOMP_PATCH void dll_615_func_1AE8(Object* self) {
    //@recomp: remove ID condition and always free object
    func_800267A4(self);
    obj_destroy_object(self);
    
    obj_free_tick(self);
    func_800267A4(self);
    obj_free_object_type(self, 4);    
    self->srt.flags |= OBJFLAG_INVISIBLE;
}
