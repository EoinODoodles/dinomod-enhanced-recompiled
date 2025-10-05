#include "modding.h"
#include "recomputils.h"
#include "dll_util.h"
#include "sidekick_util.h"

#include "game/objects/object_id.h"
#include "sys/dll.h"
#include "sys/objects.h"
#include "sys/objtype.h"

#include "recomp/dlls/_asm/2_recomp.h"

typedef void (*func_1548)(s32);
static func_1548 camcontrol_func_1548; 
static void func_1548_hijack(s32);

RECOMP_HOOK_DLL(dll_2_ctor) void camcontrol_ctor_hook(DLLFile *dll) {
    camcontrol_func_1548 = dinomod_hijack_dll_export(dll, 16, func_1548_hijack);
}

RECOMP_HOOK_RETURN_DLL(dll_2_dtor) void camcontrol_dtor_hook() {
    camcontrol_func_1548 = NULL;
}

static void func_1548_hijack(s32 a0) {
    // @recomp: Ignore func_1548 calls if in Galadon fight (removes forced z-targeting)
    Object **objectList;
    s32 count;

    objectList = obj_get_all_of_type(4, &count);

    for (s32 i = 0; i < count; i++) {
        if (objectList[i]->id == OBJ_DIM_Boss) {
            return;
        }
    }

    camcontrol_func_1548(a0);
}
