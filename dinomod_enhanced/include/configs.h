#pragma once

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
    DPAD_OFF,
    DPAD_ON_CBUTTONS_ON,
    DPAD_ON_CBUTTONS_OFF
} CmdmenuDPadModes;

typedef enum {
    POPUP_CONFIG_DEFAULT,                       //Unedited behaviour, only show pop-up when collecting Kyte's grubs
    POPUP_CONFIG_EXPANDED,                      //Show pop-up when collecting other items
    POPUP_CONFIG_OVERRIDE_TUTORIAL_ON_REPEAT    //Also show pop-up for Bridge Gears/Shiny Nuggets' tutorial box after the first time they're collected
} InfoPopupModes;

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
#define DIMCannonSounds_Config "dim_cannon_sounds"

// Configs for VampireBat's battle modes
typedef enum {
    VAMPIREBAT_BATTLE_OFF_IGNORE,   //Bats can't be harmed, and they ignore the player
    VAMPIREBAT_BATTLE_OFF_FOLLOW,   //Bats can't be harmed, and they flutter harmlessly around the player
    VAMPIREBAT_BATTLE_ON            //Bats can be battled
} VampireBat_BattleMode;
#define VampireBat_Config "vampirebat_config"

typedef enum {
    PLAY_AS_SABRE_WITH_FOX_AS_ILLUSION,     //Play as Sabre, aside from Illusion Spell (which shows Fox as a little swapped homage to the prototype's behaviour)
    PLAY_AS_FOX_ALWAYS                      //Play as Fox regardless of Illusion Spell
} PlayAsFoxModes;

_Bool configs_GetMenuTimerFractionConfig(void);
_Bool configs_GetWCPressureSwitchQOL(void);
_Bool configs_GetWCBeaconFlames(void);

#define INFO_POPUP_DURATION 300
