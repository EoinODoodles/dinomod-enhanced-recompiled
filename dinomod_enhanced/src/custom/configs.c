#include "configs.h"
#include "custom_gamebits.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "PR/os.h"
#include "common.h"

//TODO: maybe add functions for checking all the configs' values here, so they can more readily be adapted for N64 in future!

/* Checks whether Walled City's Pressure Switch quality-of-life edits are enabled. */
_Bool configs_GetWCPressureSwitchQOL(void) {
    return (recomp_get_config_u32("wc_pressure_switch_refill") != 0);
}

/* Checks whether Menu timers should show 60ths of a second too 
  (Rare had them coded in already, but printing invisibly outside the box) */
_Bool configs_GetMenuTimerFractionConfig(void) {
    return recomp_get_config_u32("timer_fractions");
}
