#include "modding.h"
#include "dll_util.h"

#include "game/objects/object.h"

#include "recomp/dlls/engine/3_ANIM_recomp.h"

typedef s32 (*Export17)(s32 objectSeqIndex, Object* object, s32 arg2);
static Export17 export17_func; 
static s32 export17_hijack(s32 objectSeqIndex, Object* object, s32 arg2);

RECOMP_HOOK_DLL(dll_3_ctor) void anim_ctor_hook(DLLFile *dll) {
    export17_func = dinomod_hijack_dll_export(dll, 17, export17_hijack);
}

RECOMP_HOOK_RETURN_DLL(dll_3_dtor) void anim_dtor_hook() {
    export17_func = NULL;
}

static s32 export17_hijack(s32 objectSeqIndex, Object* object, s32 arg2) {
    if (object->def != NULL) {
        // @recomp: Ignore if the given index is out of bounds. The actual function has a bug in this
        //          specific case where it will acquire a sequence slot but never free it when it sees
        //          that the index is out of bounds. Over time, this will make it impossible for any
        //          object sequence to play for the rest of the session.
        if (objectSeqIndex < 0 || objectSeqIndex >= object->def->numSequences || object->def->pSeq == NULL) {
            return -1;
        }
        // @recomp: Ignore if the ObjDef sequence ID is -1. This is new behavior required by dinomod.
        if (object->def->pSeq[objectSeqIndex] == -1) {
            return -1;
        }
    }

    return export17_func(objectSeqIndex, object, arg2);
}
