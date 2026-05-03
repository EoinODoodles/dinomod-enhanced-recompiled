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

#define INFO_POPUP_DURATION 300
