#include "modding.h"
#include "recomputils.h"
#include "sidekick_util.h"

#include "sys/dll.h"
#include "sys/objects.h"

#include "recomp/dlls/_asm/211_recomp.h"

#define DLL_EXPORT(num) (num + 1)

typedef void (*ObjUpdateFunc)(Object *obj);

static ObjUpdateFunc tricky_update_func; 
static void tricky_update_hijack(Object *self);

RECOMP_HOOK_DLL(dll_211_ctor) void tricky_ctor_hook(DLLFile *dll) {
    u32 *vtbl = DLL_FILE_TO_EXPORTS(dll);
    tricky_update_func = (ObjUpdateFunc)vtbl[DLL_EXPORT(1)];
    vtbl[DLL_EXPORT(1)] = (u32)&tricky_update_hijack;
}

RECOMP_HOOK_RETURN_DLL(dll_211_dtor) void tricky_dtor_hook() {
    tricky_update_func = NULL;
}

static void tricky_update_hijack(Object *self) {
    // @recomp: If the map Tricky is on unloads, unload him and skip the update function.
    if (!dinomod_unload_sidekick_if_map_unloaded(self)) {
        recomp_printf("[dinomod] Unloading Tricky due to map unload!\n");
        return;
    }

    tricky_update_func(self);
}

// TODO: replace with a real decomp of this function
RECOMP_PATCH void dll_211_func_940C(Object *self, void *state) {
    // Note: This function is probably only meant to be called when state+4C has the 0x800 bit set...
    //       It won't necessarily when unloading tricky manually, since destroy calls this without checking the state.
    u32 *unk4c = (u32*)((u32)state + 0x4c);
    *unk4c &= ~0x800;
    *unk4c |= 0x1000;

    // @recomp: Do null checks before unloading stuff, also reset pointers to null after.
    //          obj_destroy_object will crash if given a null/invalid pointer.
    void **unk0 = (void**)((u32)state + 0x0);
    if (*unk0 != NULL) {
        dll_unload(*unk0);
        *unk0 = NULL;
    }

    Object **unk5f0 = (Object**)((u32)state + 0x5f0);
    if (*unk5f0 != NULL) {
        obj_destroy_object(*unk5f0);
        *unk5f0 = NULL;
    }

    for (s32 i = 0; i < 3; i++) {
        Object **ptr = (Object**)((u32)state + (0x5e4 + (i * 4)));
        if (*ptr != NULL) {
            obj_destroy_object(*ptr);
            *ptr = NULL;
        }

    }
}
