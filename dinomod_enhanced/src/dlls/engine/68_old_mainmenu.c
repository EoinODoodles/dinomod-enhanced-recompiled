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

#include "recomp/dlls/engine/68_old_mainmenu_recomp.h"

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
            dTexTiles[i].tex = texLoadTexture(TEXTABLE_254);
            continue;
        }
        
        dTexTiles[i].tex = texLoadTexture(TEXTABLE_C5_DinosaurPlanetLogo + i);
    }    
}

static void dinomod_unload_logo_textures(void) {
    for (s32 i = 0; dTexTiles[i].animProgress != -1; i++) {
        texFreeTexture(dTexTiles[i].tex);
    }    
}

// offset: 0x0 | ctor
RECOMP_PATCH void old_mainmenu_ctor(void* dll) {
    sGametext = gDLL_21_Gametext->vtbl->get_chunk(GAMETEXT_005_Title_Menu);
    sIndexSelected = 0;
    sTextTimer = 0;
    sButtonsEnabled = FALSE;
    sTimer = 0;
    menu_func_80010018(2);

    //@recomp: load DLL 73 as a static, to fix this menu's broken function calls
    //(TODO: load the DLL in a more persistent way, as part of main? Or maybe this is fine?)
    if (gDLL_73_PicmenuOld == NULL) {
        gDLL_73_PicmenuOld = dllLoad(DLL_ID_OLD_PICMENU, 8);
    }

    //@recomp: load unused logo textures
    dinomod_load_logo_textures();
}

// offset: 0x84 | dtor
RECOMP_PATCH void old_mainmenu_dtor(void* dll) {
    mmFree(sGametext);

    //@recomp: unload DLL 73 when finished
    if (gDLL_73_PicmenuOld) {
        dllFree(gDLL_73_PicmenuOld);
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

    camViewportGetFullRect(&ulx, &uly, &lrx, &lry);

    gDPSetScissor((*gdl)++, G_SC_NON_INTERLACE, ulx, uly, lrx, lry);

    gDPSetCombineMode(*gdl, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    dlApplyCombine(gdl);

    gDPSetOtherMode(*gdl,
                    G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP |
                        G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE,
                    G_AC_NONE | G_ZS_PIXEL | G_RM_CLD_SURF | G_RM_CLD_SURF2);
    dlApplyOtherMode(gdl);

    dlSetPrimColor(gdl, 0, 0, 0, 0xFF);

    gDPFillRectangle((*gdl)++, ulx, uly, lrx, lry);

    gDLBuilder->needsPipeSync = TRUE;
    camApplyScissor(gdl);
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
    rcpTileWrite(gdl, dTexTiles, 160, 40, 0xFF, 0xFF, 0xFF, 0xFF);

    main_func_80014508(20);

    //@recomp: fix function calls
    gDLL_73_PicmenuOld->vtbl->enable_joy_buttons(sButtonsEnabled);
    gDLL_73_PicmenuOld->vtbl->init_text_window(180);
    gDLL_73_PicmenuOld->vtbl->add_string(0, sGametext->strings[0], 20, sIndexSelected); //Note: old fonts were overwritten/updated, so the text is larger here than originally intended
    
    // Advance to old Level Select
    if (gDLL_73_PicmenuOld->vtbl->handle_joystick_and_buttons(&sIndexSelected) == FALSE) {
        menuSet(MENU_OLD_LEVEL_SELECT);
    }
    
    // Blinking "PRESS START" text
    {
        if (sTextTimer > 20) {
            fontWindowDraw(gdl, NULL, NULL, 1);
        }
        sTextTimer = (gUpdateRate + sTextTimer) & 0x3F;
    }

    sButtonsEnabled = TRUE;
}
