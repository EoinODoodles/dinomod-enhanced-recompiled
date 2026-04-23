#pragma once

#define DINOMOD_ROM_PATCH FALSE

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
