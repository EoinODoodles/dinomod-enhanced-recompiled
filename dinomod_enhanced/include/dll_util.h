#pragma once

#include "PR/ultratypes.h"
#include "sys/dll.h"

// IObject export indices
enum {
    OBJEXPORT_SETUP = 0,
    OBJEXPORT_CONTROL = 1,
    OBJEXPORT_UPDATE = 2,
    OBJEXPORT_PRINT = 3,
    OBJEXPORT_FREE = 4,
    OBJEXPORT_GET_MODEL_FLAGS = 5,
    OBJEXPORT_GET_DATA_SIZE = 6
};

/**
 * Replaces a DLL's vtable pointer for an export with the given hijack function.
 * Returns a pointer to the original function so that the original code can still be ran if needed.
 *
 * This should be called from a DLL ctor hook. Do not use a return hook, the vtable should be updated
 * before the constructor runs! 
 */
void *dinomod_hijack_dll_export(DLLFile *dll, s32 exportIdx, void *hijack);
