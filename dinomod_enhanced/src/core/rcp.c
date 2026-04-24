#include "modding.h"

#include "sys/gfx/texture.h"

/** 
 * rcp_screen_write erroneously does not clear the texture DL cache like the other rcp functions.
 *
 * This causes the scarab counter text in the shop minigame to not display, as the font code will
 * not switch to the correct texture since it thinks the DL already has the font texture set (the
 * cache is desynced from the DL's actual state). 
 */
RECOMP_HOOK_RETURN("rcp_screen_write") void rcp_screen_write_return_hook(void) {
    tex_render_reset();
}
