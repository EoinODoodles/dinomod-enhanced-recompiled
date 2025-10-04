#pragma once

#include "PR/ultratypes.h"
#include "sys/dll.h"

/**
 * Replaces a DLL's vtable pointer for an export with the given hijack function.
 * Returns a pointer to the original function so that the original code can still be ran if needed.
 *
 * This should be called from a DLL ctor hook. Do not use a return hook, the vtable should be updated
 * before the constructor runs! 
 */
void *dinomod_hijack_dll_export(DLLFile *dll, s32 exportIdx, void *hijack);
