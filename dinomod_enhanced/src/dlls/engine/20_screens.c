#include "modding.h"
#include "recomputils.h"
#include "recomp/dlls/engine/20_screens_recomp.h"
#include "engine/20_screens.h"

#include "PR/ultratypes.h"
#include "sys/memory.h"

extern s32 sLoadedScreenNo;
extern s32 sRenderScreen;
extern s32 sLoadedScreenByteLength;
extern u16 *sLoadedScreen;

/** This DLL is never unloaded (by default), but it should free sLoadedScreen */
RECOMP_PATCH void screens_dtor(void *self) {
    //@recomp: free screen
    if (sLoadedScreen) {
        mmFree(sLoadedScreen);
    }
}

/** Check if a screen is currently visible */
int screens_is_screen_visible() {
    return (sLoadedScreenNo >= 0);
}
