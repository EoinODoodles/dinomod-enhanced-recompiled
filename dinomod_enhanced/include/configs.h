#pragma once

#include "PR/ultratypes.h"

typedef enum {
    BOOTCONFIG_Default_Skip_Rolling_Demo,
    BOOTCONFIG_Restore_Rolling_Demo,
    BOOTCONFIG_Skip_to_Game_Select
} BootConfigs;

typedef enum {
    GAMETEXT_VANILLA,
    GAMETEXT_COSMETIC,
} GametextFlavor;

typedef enum {
    UPCOMING_SUBTITLE_TRANS_VANILLA,
    UPCOMING_SUBTITLE_TRANS_INVISIBLE,
} UpcomingSubtitleTransparencyOption;

typedef enum {
    DPAD_OFF,
    DPAD_ON_CBUTTONS_ON,
    DPAD_ON_CBUTTONS_OFF
} CmdmenuDPadModes;

typedef enum {
    POPUP_CONFIG_DEFAULT,                       //Unedited behaviour, only show pop-up when collecting Kyte's grubs
    POPUP_CONFIG_EXPANDED,                      //Show pop-up when collecting other items
    POPUP_CONFIG_OVERRIDE_TUTORIAL_ON_REPEAT    //Also show pop-up for Bridge Gears/Shiny Nuggets' tutorial box after the first time they're collected
} InfoPopupModes;

#define INFO_POPUP_DURATION 300

typedef enum {
    ACTIVEICON_DEFAULT, //Unedited behaviour, Sidekick Command icon clashes with inventory when open
    ACTIVEICON_HIDE,    //Sidekick Command icon hidden when inventory open
    ACTIVEICON_MOVE     //Sidekick Command icon moves down when inventory opens (and Spell icon moves left if needed)
} ActiveIconModes;

/* 
 * Accessibility options for gameplay involving rapid button tapping,
 * like the Test of Strength Krazoa Shrine, or the LightFoot Strength Trial.
 */
typedef enum {
    BUTTON_TAPPING_ASSIST_OFF,       //Default difficulty, but with FPS dependency fixes
    BUTTON_TAPPING_ASSIST_ON,        //Don't need to tap as rapidly
    BUTTON_TAPPING_ASSIST_HOLD,      //Character pushes while holding A
    BUTTON_TAPPING_ASSIST_AUTO,      //Character pushes automatically
} ButtonTappingAssistModes;

typedef enum  {
    RECOMP_SPELL_AIM_FIRE_LOCK_OFF,
    RECOMP_SPELL_AIM_FIRE_LOCK_ON
} RecompSpellAimFireLockOption;

typedef enum {
    RECOMP_LOG_ROWING_TAP,
    RECOMP_LOG_ROWING_HOLD
} RecompLogAButtonMode;

typedef enum { 
    RECOMP_LOG_ROLL_DISABLED,
    RECOMP_LOG_ROLL_ENABLED
} RecompLogCanRoll;

typedef enum {
    ROPE_MOVE_DEFAULT,                      //The original tank-style rope controls.
    ROPE_MOVE_CAMERA_RELATIVE_INITIAL,      //Camera-relative rope controls, based around the moment you first tilt the stick to move. Moving the stick to a significantly different position will update the camera-based direction.
    ROPE_MOVE_CAMERA_RELATIVE_CURRENT       //Camera-relative rope controls, always relative to the current viewing angle.
} RopeMoveModes;

typedef enum {
    RECOMP_PICKUP_JINGLE_OLD_A,
    RECOMP_PICKUP_JINGLE_OLD_B,
    RECOMP_PICKUP_JINGLE_NEW
} RecompPickupJingle;

typedef enum {
    KT_RECOMP_VANILLA,
    KT_RECOMP_ENHANCED
} KTRecompMode;

// Configs for the DIMTents' unused burnt model
typedef enum {
    DIM_TENT_CINDERS_OFF,           //DIMTents just disappear as usual
    DIM_TENT_CINDERS_ON_FADEOUT,    //DIMTents leave behind a charred frame, which eventually fades away
    DIM_TENT_CINDERS_ON_PERSIST     //DIMTents leave behind a charred frame, which stays around
} DIMTentModes;

// Configs for DIMCannon's sound design
typedef enum {
    DIM_CANNON_SFX_OFF,        //The cannon is silent (unedited)
    DIM_CANNON_SFX_ON_BASIC,   //A cannon blast is played when firing
    DIM_CANNON_SFX_ON_FULL     //Lots more machinery sounds and CannonClaw voice clips are added too
} DIMCannonSounds;

// Configs for VampireBat's battle modes
typedef enum {
    VAMPIREBAT_BATTLE_OFF_IGNORE,   //Bats can't be harmed, and they ignore the player
    VAMPIREBAT_BATTLE_OFF_FOLLOW,   //Bats can't be harmed, and they flutter harmlessly around the player
    VAMPIREBAT_BATTLE_ON            //Bats can be battled
} VampireBat_BattleMode;

typedef enum {
    PLAY_AS_SABRE_WITH_FOX_AS_ILLUSION,     //Play as Sabre, aside from Illusion Spell (which shows Fox as a little swapped homage to the prototype's behaviour)
    PLAY_AS_FOX_ALWAYS                      //Play as Fox regardless of Illusion Spell
} PlayAsFoxModes;

/* GENERAL */
BootConfigs configs_GetBootUpMode(void);

/* TEXT */
GametextFlavor configs_GetGametextFlavour(void);
_Bool configs_GetGametextUseExtraDescriptions(void);
_Bool configs_GetHideUpcomingSubtitles(void);

/* AUDIO */
RecompPickupJingle configs_GetPickupJingleMode(void);

/* CONTROLS */
ButtonTappingAssistModes configs_GetButtonTapMode(void);
_Bool configs_GetSpellAimFireLock(void);
_Bool configs_LogCanHoldA(void);
_Bool configs_LogCanRoll(void);
_Bool configs_RopesTurnOnce(void);
RopeMoveModes configs_RopeMoveMode(void);

/* GAMEPLAY / QUALITY-OF-LIFE */
_Bool configs_GetSidekickToyPreventZoomies(void);
u32 configs_GetFrostWeedMax(void);
_Bool configs_GetFrostWeedTwigsConfig(void);
_Bool configs_GetWCPressureSwitchQOL(void);
_Bool configs_GetIceBlastCostReduced(void);

/* UI */
_Bool configs_GetInventoryNewControls(void);
CmdmenuDPadModes configs_GetInventoryDPadMode(void);
_Bool configs_GetIconGoldSilverKeys(void);
_Bool configs_GetIconFirefly(void);
_Bool configs_GetIconEnergyEggs(void);
_Bool configs_GetIconFox(void);
_Bool configs_GetStackShinyNuggets(void);

ActiveIconModes configs_GetUIActiveIconOverlapMode(void);
_Bool configs_GetUIActiveIconFade(void);

_Bool configs_GetSidekickMeterShowOverFood(void);
_Bool configs_GetSidekickMeterHideRedFood(void);

_Bool configs_GetItemInfoPopupFixVisuals(void);
InfoPopupModes configs_GetItemInfoPopupMode(void);

_Bool configs_GetMenuTimerFractionConfig(void);

/* GRAPHICS */
_Bool configs_GetSixtyFPS(void);
_Bool configs_GetHideDefaultFXEmits(void);

/* MISC */
_Bool configs_GetMagicGemReflectBounce(void);
u32 configs_GetMushroomDanceChance(void);
_Bool configs_GetRedMushroomsEnhanced(void);
_Bool configs_GetPurpleMushrooms(void);
VampireBat_BattleMode configs_GetVampireBatMode(void);
DIMCannonSounds configs_GetDIMCannonSoundMode(void);
DIMTentModes configs_GetDIMTentMode(void);
_Bool configs_GetWCPressureSwitchIgnoreProjectiles(void);
_Bool configs_GetWCBeaconFlames(void);
_Bool configs_GetWCSunTempleLiftRailDesign(void);
_Bool configs_GetKTEnhanced(void);
PlayAsFoxModes configs_GetPlayAsFoxMode(void);
