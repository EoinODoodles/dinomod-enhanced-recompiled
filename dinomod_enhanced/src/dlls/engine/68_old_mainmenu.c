#include "dll.h"
#include "game/gametexts.h"
#include "modding.h"
#include "recompconfig.h"
#include "configs.h"

// #include "dlls/engine/73.h"
#include "sys/dll.h"
#include "sys/fonts.h"
#include "sys/gfx/textable.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/menu.h"

#include "recomp/dlls/engine/68_old_mainmenu_recomp.h"

//TODO: remove after decomp update
#define old_mainmenu_draw old_mainmenu_func_D8

#define dTexTiles data_0 

#define sIndexSelected bss_4 
#define sTextTimer bss_8 
#define sButtonsEnabled bss_C 
#define sTimer bss_10 

#define DLL_ID_OLD_PICMENU 73
#define MENU_OLD_LEVEL_SELECT 13

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

static DLL_73* gDLL_73_PicmenuOld;

extern DLL_20_screens *gDLL_20_Screens;
extern DLL_21_gametext *gDLL_21_Gametext;
// extern DLL_73* dll_throw_fault; //NOTE: BROKEN! This is a function now, not DLL 73.

extern s8 D_8008C8B4;

#define END {NULL, -1, 0, 0}

/* TextureTiles for the old Dinosaur Planet logo, unused! */
/*0x0*/ extern TextureTile dTexTiles[];

/*0x0*/ extern GameTextChunk* sGametext;
/*0x4*/ extern s32 sIndexSelected;
/*0x8*/ extern s32 sTextTimer;
/*0xC*/ extern s32 sButtonsEnabled;
/*0x10*/ extern s32 sTimer;

static void dinomod_load_logo_textures(void) {
    for (s32 i = 0; dTexTiles[i].animProgress != -1; i++) {
        //Special case: handle first texture tile of old DP logo (since it was replaced by a full image of the newer DP logo)
        if (i == 0) {
            dTexTiles[i].tex = tex_load_deferred(TEXTABLE_254);
            continue;
        }
        
        dTexTiles[i].tex = tex_load_deferred(TEXTABLE_C5_DinosaurPlanetLogo + i);
    }    
}

static void dinomod_unload_logo_textures(void) {
    for (s32 i = 0; dTexTiles[i].animProgress != -1; i++) {
        tex_free(dTexTiles[i].tex);
    }    
}

// offset: 0x0 | ctor
RECOMP_PATCH void old_mainmenu_ctor(void* dll) {
    sGametext = gDLL_21_Gametext->vtbl->get_chunk(GAMETEXT_005_Title_Menu);
    sIndexSelected = 0;
    sTextTimer = 0;
    sButtonsEnabled = FALSE;
    sTimer = 0;
    func_80010018(2);

    //@recomp: load DLL 73 as a static, to fix this menu's broken function calls
    //(TODO: load the DLL in a more persistent way, as part of main? Or maybe this is fine?)
    if (gDLL_73_PicmenuOld == NULL) {
        gDLL_73_PicmenuOld = dll_load_deferred(DLL_ID_OLD_PICMENU, 8);
    }

    //@recomp: load unused logo textures
    dinomod_load_logo_textures();
}

// offset: 0x84 | dtor
RECOMP_PATCH void old_mainmenu_dtor(void* dll) {
    mmFree(sGametext);

    //@recomp: unload DLL 73 when finished
    if (gDLL_73_PicmenuOld) {
        dll_unload(gDLL_73_PicmenuOld);
        gDLL_73_PicmenuOld = NULL;
    }

    //@recomp: unload logo textures
    dinomod_unload_logo_textures();
}

static void recomp_draw_fill(Gfx **gdl, Mtx **mtxs, Vertex **vtxs) {
    s32 ulx;
    s32 uly;
    s32 lrx;
    s32 lry;

    viewport_get_full_rect(&ulx, &uly, &lrx, &lry);

    gDPSetScissor((*gdl)++, G_SC_NON_INTERLACE, ulx, uly, lrx, lry);

    gDPSetCombineMode(*gdl, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    dl_apply_combine(gdl);

    gDPSetOtherMode(*gdl,
                    G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP |
                        G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE,
                    G_AC_NONE | G_ZS_PIXEL | G_RM_CLD_SURF | G_RM_CLD_SURF2);
    dl_apply_other_mode(gdl);

    dl_set_prim_color(gdl, 0, 0, 0, 0xFF);

    gDPFillRectangle((*gdl)++, ulx, uly, lrx, lry);

    gDLBuilder->needsPipeSync = TRUE;
    camera_apply_scissor(gdl);
}

// offset: 0xD8 | func: 2 | export: 2
RECOMP_PATCH void old_mainmenu_draw(Gfx** gdl, Mtx** mtx, Vertex** vtx) {   
    //@recomp: fade in (like in One Hour Footage)
    {
        static u8 rsFadeStarted = FALSE;
        static u32 rsTimer = 0;

        if (rsFadeStarted == FALSE) {
            rsTimer += gUpdateRate;

            if (rsTimer > 1) {
                gDLL_28_ScreenFade->vtbl->fade_reversed(60, SCREEN_FADE_BLACK);
                rsFadeStarted = TRUE;
            } else {
                recomp_draw_fill(gdl, mtx, vtx);
            }
        }
    }

    D_8008C8B4 = 1;
    
    sTimer += gUpdateRate;    
    if (sTimer >= 41) {
        sTimer = 41;
    }

    // There were more than 2 SCREENS.bin images at one stage!
    // The old Dinosaur Planet logo (and likely the background gradient too, unless SCREENS supported opacity at an earlier stage) must've been a SCREENS image!
    // gDLL_20_Screens->vtbl->show_screen(2); //@recomp: comment out, since the image is missing

    // @recomp: put the unused TextureTiles to use, drawing the old Dinosaur Planet logo (slightly smaller than the missing SCREENS.bin version seen in the One Hour Footage)
    rcp_tile_write(gdl, dTexTiles, 160, 40, 0xFF, 0xFF, 0xFF, 0xFF);

    func_80014508(20);

    //@recomp: fix function calls
    gDLL_73_PicmenuOld->vtbl->enable_joy_buttons(sButtonsEnabled);
    gDLL_73_PicmenuOld->vtbl->init_text_window(180);
    gDLL_73_PicmenuOld->vtbl->add_string(0, sGametext->strings[0], 20, sIndexSelected); //Note: old fonts were overwritten/updated, so the text is larger here than originally intended
    
    // Advance to old Level Select
    if (gDLL_73_PicmenuOld->vtbl->handle_joystick_and_buttons(&sIndexSelected) == FALSE) {
        menu_set(MENU_OLD_LEVEL_SELECT);
    }
    
    // Blinking "PRESS START" text
    {
        if (sTextTimer > 20) {
            font_window_draw(gdl, NULL, NULL, 1);
        }
        sTextTimer = (gUpdateRate + sTextTimer) & 0x3F;
    }

    sButtonsEnabled = TRUE;
}
