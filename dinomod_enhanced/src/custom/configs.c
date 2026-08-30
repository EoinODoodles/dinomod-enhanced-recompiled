#include "configs.h"
#include "custom_gamebits.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "PR/os.h"
#include "common.h"

/* 
    NOTE ON CONFIGS:
    All mod config accesses have been routed through here, so they can more readily
    be adapted for N64 in future! 
    
    For N64, maybe uses of `recomp_get_config_u32` could be removed with a `DINOMOD_ROM_PATCH` ifdef,
    and configs could then be read from an unused/extended portion of the game options save data.
*/

/* GENERAL */

/* Checks what menu the game should go to during startup. */
BootConfigs configs_GetBootUpMode(void) {
    return recomp_get_config_u32("rolling_demo");
}


/* TEXT */

/* Checks which variety of Gametext should be used. */
GametextFlavor configs_GetGametextFlavour(void) {
    return recomp_get_config_u32("gametext_flavor");
}

/* Checks whether custom object description text should be used. */
_Bool configs_GetGametextUseExtraDescriptions(void) {
    return recomp_get_config_u32("lamingaming_extra_description_text") != 0;
}

/* Checks whether upcoming subtitles should be hidden. */
_Bool configs_GetHideUpcomingSubtitles(void) {
    return recomp_get_config_u32("upcoming_subtitle_trans") != 0;
}


/* AUDIO */

/* Checks which item collection jingle should play */
RecompPickupJingle configs_GetPickupJingleMode(void) {
    return recomp_get_config_u32("pickup_jingle");
}


/* CONTROLS */

/* Checks which button tapping option is selected. */
ButtonTappingAssistModes configs_GetButtonTapMode(void) {
    return recomp_get_config_u32("button_tap_modes");
}

/* Checks whether the player shouldn't able to move the aiming reticle during the spell firing animation. */
_Bool configs_GetSpellAimFireLock(void) {
    return recomp_get_config_u32("spell_aim_fire_lock") != 0;
}

/* Checks whether the log vehicle can be paddled automatically holding the A button. */
_Bool configs_LogCanHoldA(void) {
    return recomp_get_config_u32("log_a_button") == RECOMP_LOG_ROWING_HOLD;
}

/* Checks whether or not the log vehicle's rolling is enabled. */
_Bool configs_LogCanRoll(void) {
    return recomp_get_config_u32("log_rolling") == RECOMP_LOG_ROLL_ENABLED;
}


/* GAMEPLAY / QUALITY-OF-LIFE */

/* Checks whether to change the Sidekick Toy's zooming-away behaviour as it rolls to a stop on sloped surfaces. */
_Bool configs_GetSidekickToyPreventZoomies(void) {
    return recomp_get_config_u32("sidekick_toy_zoomies") != 0;
}

/* Checks how many FrostWeeds are needed for Garunda Te's quest */
u32 configs_GetFrostWeedMax(void) {
    return recomp_get_config_u32("garunda_te_frostweeds_override");
}

/* Checks whether Garunda Te will accept damaged FrostWeeds that are carried over to him manually */
_Bool configs_GetFrostWeedTwigsConfig(void) {
    return recomp_get_config_u32("garunda_te_frostweeds_accept_twigs") != 0;
}

/* Checks whether Walled City's Pressure Switch quality-of-life edits are enabled. */
_Bool configs_GetWCPressureSwitchQOL(void) {
    return (recomp_get_config_u32("wc_pressure_switch_refill") != 0);
}

/* Checks whether to reduce the Ice Blast Spell's cost. */
_Bool configs_GetIceBlastCostReduced(void) {
    return recomp_get_config_u32("iceblast_cost") != 0;
}


/* UI */

/* Checks whether to use new control mode for the inventory (allowing scrolling up or down through items, not just down). */
_Bool configs_GetInventoryNewControls(void) {
    return recomp_get_config_u32("cmdmenu_new_controls") != 0;
}

/* Checks which D-pad inventory control option is selected. */
CmdmenuDPadModes configs_GetInventoryDPadMode(void) {
    return recomp_get_config_u32("cmdmenu_d_controls");
}

/* Checks whether to use updated Kiosk icons for DIM's Gold/Silver keys. */
_Bool configs_GetIconGoldSilverKeys(void) {
    return recomp_get_config_u32("cmdmenu_icons_gold_silver_keys") != 0;
}

/* Checks whether to use an updated Kiosk icon for the Lantern Fireflies */
_Bool configs_GetIconFirefly(void) {
    return recomp_get_config_u32("cmdmenu_icons_firefly") != 0;
}

/* Checks whether to use a custom icon for the Energy Eggs */
_Bool configs_GetIconEnergyEggs(void) {
    return recomp_get_config_u32("cmdmenu_icons_energy_eggs") != 0;
}

/* Checks whether to use a Kiosk character portrait icon of Fox */
_Bool configs_GetIconFox(void) {
    return recomp_get_config_u32("cmdmenu_icons_fox") != 0;
}

/* Checks whether Shiny Nuggets should be stacked (TODO: not implemented yet) */
_Bool configs_GetStackShinyNuggets(void) {
    return recomp_get_config_u32("cmdmenu_stack_shiny_nuggets") != 0;
}

/* Checks how to handle the overlap between the inventory and the icons for the active Spell/Sidekick command. */
ActiveIconModes configs_GetUIActiveIconOverlapMode(void) {
    return recomp_get_config_u32("cmdmenu_active_icon_overlap");
}

/* Checks whether to animate a fade in/out on the icons for the active Spell/Sidekick command. */
_Bool configs_GetUIActiveIconFade(void) {
    return recomp_get_config_u32("cmdmenu_active_icon_fade") != 0;
}

/* Checks whether to show the Sidekick energy meter while Blue Mushrooms are highlighted in the inventory. */
_Bool configs_GetSidekickMeterShowOverFood(void) {
    return recomp_get_config_u32("cmdmenu_sidekick_meter_show_over_food") != 0;
}

/* Checks whether to hide red food from the Sidekick energy meter. */
_Bool configs_GetSidekickMeterHideRedFood(void) {
    return recomp_get_config_u32("cmdmenu_sidekick_meter_hide_red") != 0;
}

/* Checks whether the UI's item info pop-up should be fixed up visually. */
_Bool configs_GetItemInfoPopupFixVisuals(void) {
    return recomp_get_config_u32("cmdmenu_info_popup_fix") != 0;
}

/* Checks how the UI's item info pop-up should be handled, expanding the pool of collectables that display it, etc. */
InfoPopupModes configs_GetItemInfoPopupMode(void) {
    return recomp_get_config_u32("cmdmenu_info_popup_expand");
}

/* Checks whether Menu timers should show 60ths of a second too 
  (Rare had them coded in already, but printing invisibly outside the box) */
_Bool configs_GetMenuTimerFractionConfig(void) {
    return recomp_get_config_u32("timer_fractions") != 0;
}


/* GRAPHICS */

/* Checks whether 60fps mode is enabled. */
_Bool configs_GetSixtyFPS(void) {
    return recomp_get_config_u32("sixty_fps_mode") != 0;
}

/* Checks whether to hide default FXEmits (misconfigured ones that flash up for a frame and then disappear). */
_Bool configs_GetHideDefaultFXEmits(void) {
    return recomp_get_config_u32("fxemit_hide_default") != 0;
}


/* MISC */

/* Checks whether to use the Magic Gems' unused fancy bounce calculations. */
_Bool configs_GetMagicGemReflectBounce(void) {
    return recomp_get_config_u32("magic_dust_reflect_bounce") != 0;
}

/* Checks the likelihood of showing Blue/White Mushrooms' unused dance animation. */
u32 configs_GetMushroomDanceChance(void) {
    return recomp_get_config_u32("shmushroom_dance_chance");
}

/* Checks whether to use Red Mushrooms' hidden/unused states. */
_Bool configs_GetRedMushroomsEnhanced(void) {
    return recomp_get_config_u32("shkillermushroom_show_hidden_states") != 0;
}

/* Checks whether to replace Bomb Spore plants with purple Rocket Mushrooms. */
_Bool configs_GetPurpleMushrooms(void) {
    return recomp_get_config_u32("shrocketmushroom_purple") != 0;
}

/* Checks how Vampire Bats should behave. */
VampireBat_BattleMode configs_GetVampireBatMode(void) {
    return recomp_get_config_u32("vampirebat_config");
}

/* Checks what option to use for DIMCannon's custom sound design. */
DIMCannonSounds configs_GetDIMCannonSoundMode(void) {
    return recomp_get_config_u32("dim_cannon_sounds");
}

/* Checks how DIMTent's cinders should be handled. */
DIMTentModes configs_GetDIMTentMode(void) {
    return recomp_get_config_u32("dim_tent_cinders");
}

/* Checks whether Walled City's Pressure Switches should ignore projectiles. */
_Bool configs_GetWCPressureSwitchIgnoreProjectiles(void) {
    return (recomp_get_config_u32("wc_pressure_switch_ignore_projectiles") != 0);
}

/* Checks whether Walled City's Beacons should create flame effects. */
_Bool configs_GetWCBeaconFlames(void) {
    return (recomp_get_config_u32("wc_beacon_flames") != 0);
}

/* Checks whether to use King RedEye's "Enhanced" mode */
_Bool configs_GetKTEnhanced(void) {
    return recomp_get_config_u32("kt_mode") != KT_RECOMP_VANILLA;
}

/* Checks what option to use for playing as Fox. */
PlayAsFoxModes configs_GetPlayAsFoxMode(void) {
    return recomp_get_config_u32("play_as_fox");
}
