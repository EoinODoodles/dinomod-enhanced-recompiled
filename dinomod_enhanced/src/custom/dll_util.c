#include "PR/ultratypes.h"
#include "sys/dll.h"

void *dinomod_hijack_dll_export(DLLFile *dll, s32 exportIdx, void *hijack) {
    u32 *vtbl = DLL_FILE_TO_EXPORTS(dll);

    // +1 to get skip the initial null pointer. Export indices do not include the null at the start of the vtable.
    void **exportPtr = (void**)&vtbl[exportIdx + 1];
    void *original = *exportPtr;
    *exportPtr = hijack;

    return original;
}
