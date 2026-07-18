#include "configs.h"
#include "modding.h"
#include "recompconfig.h"

#include "dll.h"
#include "dlls/engine/73.h"
#include "game/gametexts.h"
#include "sys/dll.h"
#include "sys/fonts.h"
#include "sys/gfx/textable.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/menu.h"

#include "engine/73_old_picmenu.h"

#include "recomp/dlls/engine/73_old_picmenu_recomp.h"

/*0x0*/ extern s8 dButtonsEnabled;  //Whether to check button presses (for backing out/advancing)

/*0x0*/ extern s8 sValueEnter; //Value returned when selecting a menu item
/*0x1*/ extern s8 sValueExit;  //Value returned when backing out from the menu
/*0x2*/ extern s16 sTextY;
/*0x4*/ extern s8 sTotalItems;
/*0x8*/ extern f32 sTimer;

//@recomp: use statics for the fontIDs, so they can be changed
static FontID rsFontLight = FONT_DINO_MEDIUM_FONT_IN;
static FontID rsFontDark = FONT_DINO_MEDIUM_FONT_OUT;

//@recomp: allow font colours to be edited too
static u8 rsFontColourLight[4] = {0xFF, 0xFF, 0xFF, 0};
static u8 rsFontColourDark[4] = {0xFF, 0xFF, 0xFF, 0};
static u8 rsFontColourShadow[4] = {0, 0, 0, 0xFF};

static u8 rsUseDropShadow = FALSE;
static s8 rsDropShadowCoords[2] = {1, 1};

void dll_73_set_fontIDs(FontID light, FontID dark) {
    rsFontLight = light;
    rsFontDark = dark;

    font_load(light);
    if (light != dark) {
        font_load(dark);
    }
}

FontID dll_73_get_fontID_light(void) {
    return rsFontLight;
}

FontID dll_73_get_fontID_dark(void) {
    return rsFontLight;
}

void dll_73_set_font_colours(u32 light, u32 dark) {
    rsFontColourLight[0] = (light >> (8*3)) & 0xFF;
    rsFontColourLight[1] = (light >> (8*2)) & 0xFF;
    rsFontColourLight[2] = (light >> 8) & 0xFF;
    rsFontColourLight[3] = light & 0xFF;
    
    rsFontColourDark[0] = (dark >> (8*3)) & 0xFF;
    rsFontColourDark[1] = (dark >> (8*2)) & 0xFF;
    rsFontColourDark[2] = (dark >> (8)) & 0xFF;
    rsFontColourDark[3] = dark & 0xFF;
}

void dll_73_enable_drop_shadow(u8 enable) {
    rsUseDropShadow = (enable != 0);
}

void dll_73_set_drop_shadow_colour(u32 colour) {
    rsFontColourShadow[0] = (colour >> (8*3)) & 0xFF;
    rsFontColourShadow[1] = (colour >> (8*2)) & 0xFF;
    rsFontColourShadow[2] = (colour >> (8)) & 0xFF;
    rsFontColourShadow[3] = colour & 0xFF;
}

void dll_73_set_drop_shadow_position(s8 x, s8 y) {
    rsDropShadowCoords[0] = x;
    rsDropShadowCoords[1] = y;
}

extern void dll_73_set_font_and_colour(s32 dimmed);

// offset: 0x118 | func: 1 | export: 1
RECOMP_PATCH void dll_73_init_text_window_with_margin(s32 marginX, s32 y) {
    u32 halfWidth;

    halfWidth = (GET_VIDEO_WIDTH(vi_get_current_size()) - marginX) / 2;
    
    font_window_set_coords(1, halfWidth, 0, 
        GET_VIDEO_WIDTH(vi_get_current_size()) - halfWidth, 
        GET_VIDEO_HEIGHT(vi_get_current_size())
    );
    
    font_window_use_font(1, rsFontLight);
    font_window_flush_strings(1);
    sTextY = y;
    sTotalItems = 0;
    sValueExit = DLL73_ACTION_None;
}

// offset: 0x204 | func: 2 | export: 2
/**
  * Adds a string to the font window, centred horizontally.
  *
  * The selected line has an animation, fading between the Dino Medium In/Out fonts.
  */
RECOMP_PATCH void dll_73_add_string(s32 valueEnter, char* text, s32 lineHeight, s32 selectedIndex) {
    f32 tValue;
    f32 opacity;
    f32 opacityRemainder;

    if (selectedIndex == sTotalItems) {
        sValueEnter = valueEnter;
        if (sTimer <= 100.0f) {
            tValue = sTimer / 100.0f;
            opacity = (s32)(tValue * 255.0f);
            opacityRemainder = (s32) ((1.0f - tValue) * 255.0f);
        } else {
            tValue = (sTimer - 100.0f) / 100.0f;
            opacityRemainder = (s32) (tValue * 255.0f);
            opacity = (s32) ((1.0f - tValue) * 255.0f);
        }
        
        //Drop-shadows
        if (rsUseDropShadow) {
            font_window_use_font(1, rsFontLight);
            font_window_set_text_colour(1, rsFontColourShadow[0], rsFontColourShadow[1], rsFontColourShadow[2], rsFontColourShadow[3], (s32)opacity);
            font_window_add_string_xy(1, -0x8000, sTextY + rsDropShadowCoords[1], text, 2, ALIGN_TOP_CENTER);
            
            font_window_use_font(1, rsFontDark);
            font_window_set_text_colour(1, rsFontColourShadow[0], rsFontColourShadow[1], rsFontColourShadow[2], rsFontColourShadow[3], (s32)opacityRemainder);
            font_window_add_string_xy(1, -0x8000, sTextY + rsDropShadowCoords[1], text, 2, ALIGN_TOP_CENTER);
        }
        
        //Light
        font_window_use_font(1, rsFontLight);
        font_window_set_text_colour(1, rsFontColourLight[0], rsFontColourLight[1], rsFontColourLight[2], rsFontColourLight[3], (s32)opacity);
        font_window_add_string_xy(1, -0x8000, sTextY, text, 1, ALIGN_TOP_CENTER);
        
        //Dark
        font_window_set_text_colour(1, rsFontColourDark[0], rsFontColourDark[1], rsFontColourDark[2], rsFontColourLight[3], (s32)opacityRemainder);
        font_window_use_font(1, rsFontDark);
    } else {
        //Drop-shadow
        if (rsUseDropShadow) {
            font_window_use_font(1, rsFontDark);
            font_window_set_text_colour(1, rsFontColourShadow[0], rsFontColourShadow[1], rsFontColourShadow[2], rsFontColourShadow[3], 0xFF);
            font_window_add_string_xy(1, -0x8000, sTextY + rsDropShadowCoords[1], text, 2, ALIGN_TOP_CENTER);
        }

        //Dark
        font_window_set_text_colour(1, rsFontColourDark[0], rsFontColourDark[1], rsFontColourDark[2], rsFontColourLight[3], 0xFF);
    }
    font_window_add_string_xy(1, -0x8000, sTextY, text, 1, ALIGN_TOP_CENTER);
    
    sTextY += lineHeight;
    sTotalItems++;
}

// offset: 0x6D0 | func: 6 | export: 6
RECOMP_PATCH void dll_73_set_font_and_colour(s32 dimmed) {
    if (dimmed == FALSE) {
        font_window_set_text_colour(1, rsFontColourLight[0], rsFontColourLight[1], rsFontColourLight[2], rsFontColourLight[3], 0xFF);
        font_window_use_font(1, rsFontLight);
    } else {
        font_window_set_text_colour(1, rsFontColourDark[0], rsFontColourDark[1], rsFontColourDark[2], rsFontColourLight[3], 0xFF);
        font_window_use_font(1, rsFontDark);
    }
}
