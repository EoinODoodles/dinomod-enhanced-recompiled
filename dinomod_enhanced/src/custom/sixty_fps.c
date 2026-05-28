#include "modding.h"
#include "recompevents.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "PR/ultratypes.h"

enum Recomp60FPSMode {
    SIXTY_FPS_OFF,
    SIXTY_FPS_ON
};

RECOMP_ON_GAME_TICK_START_CALLBACK void dinomod_sixty_fps_toggle(void) {
    static s32 prevSixtyFPSOption = -1;
    s32 sixtyFPSOption = recomp_get_config_u32("sixty_fps_mode");
    
    if (prevSixtyFPSOption == -1 || sixtyFPSOption != prevSixtyFPSOption) {
        recomp_set_60fps_enabled(sixtyFPSOption == SIXTY_FPS_ON ? TRUE : FALSE);
        prevSixtyFPSOption = sixtyFPSOption;
    }
}
