#include "modding.h"
#include "recompconfig.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "PR/os.h"
#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/fonts.h"
#include "sys/gfx/gx.h"
#include "sys/gfx/texture.h"
#include "sys/main.h"
#include "sys/dll.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "sys/rcp.h"
#include "dll.h"
#include "dll_util.h"
#include "functions.h"
#include "types.h"

#include "recomp/dlls/engine/62_mainmenu_recomp.h"

extern u8 showDPLogo;
extern s8 sExitTransitionTimer;
extern s8 nextMenuID;
extern Texture* logoDinosaurPlanet;

#define MENU_TRANSITION_THRESHOLD 12

RECOMP_PATCH void mainmenu_draw(Gfx** gfx, Mtx** mtx, Vertex** vtx) {
    if (!nextMenuID || sExitTransitionTimer >= (MENU_TRANSITION_THRESHOLD - 1)) {
        font_window_set_coords(1, 0, 0, GET_VIDEO_WIDTH(vi_get_current_size()), GET_VIDEO_HEIGHT(vi_get_current_size()));
        font_window_flush_strings(1);
        gDLL_74_Picmenu->vtbl->draw(gfx);
        //@recomp: use the showDPLogo flag to actually show the DP logo
        if (main_demo_finished() || showDPLogo) {
            func_8003825C(gfx, logoDinosaurPlanet, 50, 50, 0, 0, 0xFF, 0);
        }
        font_window_draw(gfx, NULL, NULL, 1);
    }
}
