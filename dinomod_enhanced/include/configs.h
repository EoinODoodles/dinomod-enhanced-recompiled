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
    POPUP_CONFIG_DEFAULT,
    POPUP_CONFIG_EXPANDED,
    POPUP_CONFIG_OVERRIDE_TUTORIAL_ON_REPEAT
} InfoPopupModes;

#define INFO_POPUP_DURATION 300
