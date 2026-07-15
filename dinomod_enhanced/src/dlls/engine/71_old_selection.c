//Seems to be an old version of the SwapStone choice menu (DLL 71 selection)

#include "dll.h"
#include "modding.h"
#include "recompconfig.h"
#include "configs.h"

#include "dlls/engine/29_gplay.h"
// #include "dlls/engine/73.h"
#include "sys/dll.h"
#include "sys/fonts.h"
#include "sys/map.h"
#include "sys/menu.h"
#include "sys/objmsg.h"

#include "recomp/dlls/_asm/71_recomp.h"

#define dll_71_draw dll_71_func_48

#define DLL_ID_OLD_PICMENU 73

typedef enum {
    DLL73_ACTION_None = -1
} DLL73_Actions;

DLL_INTERFACE(DLL_73) {
/*:*/ DLL_INTERFACE_BASE(DLL);
/*0*/ void (*init_text_window)(s32 y);
/*1*/ void (*init_text_window_with_margin)(s32 marginX, s32 y);
/*2*/ void (*add_string)(s32 valueEnter, char* text, s32 lineHeight, s32 selectedIndex);
/*3*/ void (*add_string_x)(s32 valueEnter, char* text, s32 x, s32 lineHeight, s32 selectedIndex);
/*4*/ void (*set_exit_value)(s32 value);
/*5*/ s16 (*handle_joystick_and_buttons)(s32* idx);
/*6*/ void (*set_font_and_colour)(s32 dimmed);
/*7*/ void (*enable_joy_buttons)(s32 enabled);
/*8*/ s8 (*get_total_items)(void);
};

extern DLL_29_gplay *gDLL_29_Gplay;
// extern DLL_73* dll_throw_fault; //NOTE: BROKEN! This is a function now, not DLL 73.

static DLL_73* gDLL_73_PicmenuOld;

extern s8 D_8008C8B4;

typedef enum {
    SwapStone_Old_Continue,
    SwapStone_Old_Swap,
    SwapStone_Old_To_Warlock
} SwapStone_Old_Options;

#define sIndexSelected bss_0
#define sButtonsEnabled bss_4

/*0x0*/ static s32 sIndexSelected;
/*0x4*/ static s32 sButtonsEnabled;

// offset: 0x0 | ctor
RECOMP_PATCH void dll_71_ctor(void *dll) {
    sIndexSelected = 0;
    sButtonsEnabled = FALSE;

    //@recomp: load DLL 73 as a static, to fix this menu's broken function calls
    if (gDLL_73_PicmenuOld == NULL) {
        gDLL_73_PicmenuOld = dll_load_deferred(DLL_ID_OLD_PICMENU, 8);
    }
}

// offset: 0x28 | dtor
RECOMP_PATCH void dll_71_dtor(void *dll) {
    //@recomp: unload DLL 73 when finished
    if (gDLL_73_PicmenuOld) {
        dll_unload(gDLL_73_PicmenuOld);
        gDLL_73_PicmenuOld = NULL;
    }
}

// offset: 0x48 | func: 2 | export: 2
RECOMP_PATCH void dll_71_draw(Gfx** gdl, Mtx** mtx, Vertex** vtx) {
    s32 warpID;
    s32 playerNumber;

    playerNumber = gDLL_29_Gplay->vtbl->get_playerno();
    D_8008C8B4 = FALSE;
    
    //@recomp: restore these function calls
    gDLL_73_PicmenuOld->vtbl->enable_joy_buttons(sButtonsEnabled);
    gDLL_73_PicmenuOld->vtbl->init_text_window(60);
    gDLL_73_PicmenuOld->vtbl->add_string(0, "CONTINUE",   20, sIndexSelected);
    gDLL_73_PicmenuOld->vtbl->add_string(1, "SWAP",       20, sIndexSelected);
    gDLL_73_PicmenuOld->vtbl->add_string(2, "TO WARLOCK", 20, sIndexSelected);
    gDLL_73_PicmenuOld->vtbl->set_exit_value(0);
    
    switch (gDLL_73_PicmenuOld->vtbl->handle_joystick_and_buttons(&sIndexSelected)) {
    default:
        D_8008C8B4 = TRUE;
        break;
    case SwapStone_Old_Continue:
        obj_send_mesg_many(/* OBJ_16 (deleted) */16, OBJMSG_SEND_FILTER_ID, NULL, 0, 0);
        menu_set(MENU_GAMEPLAY);
        break;
    case SwapStone_Old_Swap:
        warpID = (playerNumber == PLAYER_SABRE) ? WARP_SC_RUBBLE_PODIUM : WARP_SH_ROCKY_PODIUM;
        gDLL_29_Gplay->vtbl->set_playerno((1 - playerNumber));
        warpPlayer(warpID, TRUE);
        menu_set(MENU_GAMEPLAY);
        break;
    case SwapStone_Old_To_Warlock:
        warpID = (playerNumber == PLAYER_SABRE) ? WARP_WM_SABRE_SIDE : WARP_WM_KRYSTAL_SIDE;
        warpPlayer(warpID, TRUE);
        break;
    }
    
    font_window_set_coords(3, 100, 25, 220, 140);
    font_window_set_bg_colour(3, 0, 0, 0, 0x80);
    font_window_flush_strings(3);
    font_window_draw(gdl, 0, 0, 3);
    font_window_draw(gdl, 0, 0, 1);
    sButtonsEnabled = TRUE;
}
