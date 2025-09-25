#include "modding.h"
#include "recomputils.h"
#include "sidekick_util.h"

#include "sys/dll.h"

#include "recomp/dlls/_asm/212_recomp.h"

#define DLL_EXPORT(num) (num + 1)

typedef void (*ObjUpdateFunc)(Object *obj);

static ObjUpdateFunc kyte_update_func; 
static void kyte_update_hijack(Object *self);

RECOMP_HOOK_DLL(dll_212_ctor) void kyte_ctor_hook(DLLFile *dll) {
    u32 *vtbl = DLL_FILE_TO_EXPORTS(dll);
    kyte_update_func = (ObjUpdateFunc)vtbl[DLL_EXPORT(1)];
    vtbl[DLL_EXPORT(1)] = (u32)&kyte_update_hijack;
}

static void kyte_update_hijack(Object *self) {
    // @recomp: If the map Kyte is on unloads, unload her and skip the update function.
    if (!dinomod_unload_sidekick_if_map_unloaded(self)) {
        recomp_printf("[dinomod] Unloading Kyte due to map unload!\n");
        return;
    }

    kyte_update_func(self);
}
