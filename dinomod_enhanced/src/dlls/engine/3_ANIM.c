#include "modding.h"
#include "dll_util.h"

#include "game/objects/object.h"

#include "recomp/dlls/engine/3_ANIM_recomp.h"

typedef void (*Export17)(s32 objectSeqIndex, Object* object, s32 arg2);
static Export17 export17_func; 
static void export17_hijack(s32 objectSeqIndex, Object* object, s32 arg2);

RECOMP_HOOK_DLL(dll_3_ctor) void anim_ctor_hook(DLLFile *dll) {
    export17_func = dinomod_hijack_dll_export(dll, 17, export17_hijack);
}

RECOMP_HOOK_RETURN_DLL(dll_3_dtor) void anim_dtor_hook() {
    export17_func = NULL;
}

static void export17_hijack(s32 objectSeqIndex, Object* object, s32 arg2) {
    // @recomp: Ignore if the ObjDef sequence ID is -1
    if (object->def != NULL && object->def->pSeq != NULL) {
        if (object->def->pSeq[objectSeqIndex] == -1) {
            return;
        }
    }

    export17_func(objectSeqIndex, object, arg2);
}
