#include "modding.h"
#include "recompconfig.h"
#include "dll_util.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "PR/os.h"
#include "types.h"
#include "game/objects/object.h"
#include "game/gamebits.h"
#include "game/gametexts.h"
#include "recomputils.h"
#include "sys/gfx/texture.h"
#include "sys/dll.h"
#include "sys/fonts.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "sys/rcp.h"
#include "dll.h"
#include "dlls/engine/74_picmenu.h"

#include "recomp/dlls/engine/62_mainmenu_recomp.h"

typedef enum {
    //Overrides for item7 (Language)
    MainMenu_ITEM_0_English = 0,
    MainMenu_ITEM_1_Francais = 1,
    MainMenu_ITEM_2_Deutsch = 2,
    MainMenu_ITEM_3_Espanol = 3,
    MainMenu_ITEM_5_Italiano = 4,

    MainMenu_ITEM_5_Start = 5,
    MainMenu_ITEM_6_Options = 6,
    MainMenu_ITEM_7_Language = 7 //Gets overridden with items 0-4
} MainMenuItemIndices;

#define MENU_TRANSITION_THRESHOLD 12
#define MENU_TRANSITION_DURATION 35

extern s32 D_8008C890;

extern PicMenuItem pressStartItem[];
extern PicMenuItem mainMenuItems[];
extern s16 gametextLineIndices[];

extern u8 showDPLogo;
extern GameTextChunk* gametext;
extern GplayOptions* options;
extern s8 sExitTransitionTimer;
extern s8 nextMenuID;
extern Texture* logoDinosaurPlanet;

extern void mainmenu_clean_up(void);

/** 
  * Start out with "Options" selected when coming back from the Options Menu
  */
RECOMP_PATCH void mainmenu_ctor(void *dll) {
    int total_strings;
    s32 index;
    s32 prevMenuID; //@recomp

    total_strings = 8;
    
    logoDinosaurPlanet = tex_load_deferred(0xC5);
    rcp_set_border_color(0, 0, 0);

    //Set language and get text
    options = gDLL_29_Gplay->vtbl->get_game_options();
    gDLL_21_Gametext->vtbl->set_bank(options->languageID);
    gametext = gDLL_21_Gametext->vtbl->get_chunk(GAMETEXT_0EE_Menu_Title_Screen);

    //Set "Press Start" text
    pressStartItem->text = gametext->strings[6];

    for (index = 0; index < total_strings; index++){
        mainMenuItems[index].text = gametext->strings[gametextLineIndices[index]];
    }
    
    //Hide the DP logo at first when entering from the Rareware screen
    prevMenuID = menu_get_previous();
    if (prevMenuID == MENU_RAREWARE) {
        gDLL_74_Picmenu->vtbl->set_items(pressStartItem, 1, 0, 0, 0, 0, 0xB7, 0x8B, 0x61, 0xFF, 0xD7, 0x3D);
        showDPLogo = FALSE;
    } else {
        mainMenuItems[MainMenu_ITEM_7_Language].overrideWith = options->languageID; //Set language index
        gDLL_74_Picmenu->vtbl->set_items(mainMenuItems, 8, 5, 0, 0, 0, 0xB7, 0x8B, 0x61, 0xFF, 0xD7, 0x3D);
        showDPLogo = TRUE;

        //@recomp: start out with "Options" selected when coming back from the Options Menu
        if (prevMenuID == MENU_OPTIONS) {
            gDLL_74_Picmenu->vtbl->set_selected_item(6);
        }
    }
    
    nextMenuID = NULL;
    sExitTransitionTimer = 0;
}

/** 
  * Small fixes for the Main Menu's interactions:
  *
  * - When only the "Press Start" text is displayed, don't play a "backing out" sound when pressing B (since it doesn't go anywhere).
  * - Save chosen language setting when selecting it with A, or when entering Game Select/Options.
  * - Allow backing out to the "Press Start" text from the "Start/Options/Language" selection, by pressing B.
  */
RECOMP_PATCH s32 mainmenu_update1(void) {
    s32 temp;
    s32 index;
    s32 action;
    s32 lineIndex;
    s8 prevExitTransitionTimer;
    s32 delay;

    prevExitTransitionTimer = sExitTransitionTimer;

    delay = gUpdateRate;
    if (delay > 3) {
        delay = 3;
    }

    if (sExitTransitionTimer > 0) {
        sExitTransitionTimer -= delay;
    }

    //@recomp: when only the "Press Start" text is displayed, don't play a sound when pressing B (since it doesn't do anything) 
    if (showDPLogo == FALSE) {
        joy_set_button_mask(0, B_BUTTON);
    }

    //Transitioning to different page once timer runs out
    if (nextMenuID) {
        if (prevExitTransitionTimer > MENU_TRANSITION_THRESHOLD && sExitTransitionTimer <= MENU_TRANSITION_THRESHOLD) {
            //Change resolution for game select
            vi_init(14, get_ossched(), FALSE);
            mainmenu_clean_up();
            func_80041D20(0);
            func_80041C6C(0);
            if (nextMenuID == MENU_GAME_SELECT) {
                gDLL_29_Gplay->vtbl->save_game_options();
            }
        } else if (sExitTransitionTimer < 1) {
            gDLL_29_Gplay->vtbl->save_game_options(); //@recomp: save options
            func_800141A4(1, 0, PLAYER_KRYSTAL, nextMenuID);
        }

        if (sExitTransitionTimer <= MENU_TRANSITION_THRESHOLD) {
            return 1;
        } else {
            return 0;
        }
    } else {
        action = gDLL_74_Picmenu->vtbl->update();

        //When DP logo not visible and button pressed
        if (showDPLogo == FALSE){
            if (action == PICMENU_ACTION_SELECT) {
                gDLL_74_Picmenu->vtbl->clear_items();
                mainMenuItems[MainMenu_ITEM_7_Language].overrideWith = options->languageID;
                gDLL_74_Picmenu->vtbl->set_items(mainMenuItems, 8, 5, NULL, 0, 0, 0xB7, 0x8B, 0x61, 0xFF, 0xD7, 0x3D);
                showDPLogo = TRUE;
            }
        } else {
            if (action == PICMENU_ACTION_SELECT) {
    
                temp = 1;
                switch (gDLL_74_Picmenu->vtbl->get_selected_item()) {
                    case 5:
                        gDLL_28_ScreenFade->vtbl->fade(20, temp);
                        nextMenuID = MENU_GAME_SELECT;
                        sExitTransitionTimer = MENU_TRANSITION_DURATION;
                        return 0;
                    case 6:
                        gDLL_28_ScreenFade->vtbl->fade(20, temp);
                        nextMenuID = MENU_OPTIONS;
                        sExitTransitionTimer = MENU_TRANSITION_DURATION;
                        return 0;
                    //@recomp: save language option when selecting it with A
                    case 7:
                        gDLL_29_Gplay->vtbl->save_game_options();
                        break;
                }

            //@recomp: Allow backing out to just the "Press Start" text
            } else if (action == PICMENU_ACTION_BACK) {
                if (showDPLogo) {
                    gDLL_74_Picmenu->vtbl->clear_items();
                    gDLL_74_Picmenu->vtbl->set_items(pressStartItem, 1, 0, 0, 0, 0, 0xB7, 0x8B, 0x61, 0xFF, 0xD7, 0x3D);
                    showDPLogo = FALSE;
                }
            }
    
            //Changing language
            options->languageID = gDLL_74_Picmenu->vtbl->get_item_override(7);
            if (gDLL_21_Gametext->vtbl->curr_bank() != options->languageID) {
                gDLL_21_Gametext->vtbl->set_bank(options->languageID);
                mmFree(gametext);
                gametext = gDLL_21_Gametext->vtbl->get_chunk(GAMETEXT_0EE_Menu_Title_Screen);
    
                temp = 8;
                for (index = 0; index < temp; index++){
                    mainMenuItems[index].text = gametext->strings[gametextLineIndices[index]];
                }
                
                gDLL_74_Picmenu->vtbl->update_text(mainMenuItems);
            }
        }
        
        return 0;
    }
}

/** 
  * Show the Dinosaur Planet logo when entering "Start/Options/Language" selection
  */
RECOMP_PATCH void mainmenu_draw(Gfx** gfx, Mtx** mtx, Vertex** vtx) {
    if (!nextMenuID || sExitTransitionTimer >= (MENU_TRANSITION_THRESHOLD - 1)) {
        font_window_set_coords(1, 0, 0, GET_VIDEO_WIDTH(vi_get_current_size()), GET_VIDEO_HEIGHT(vi_get_current_size()));
        font_window_flush_strings(1);
        gDLL_74_Picmenu->vtbl->draw(gfx);
        
        //@recomp: use the showDPLogo flag to actually show the DP logo
        if (main_demo_finished() || showDPLogo) {
            //@rom-patch: centre logo in widescreen
            #if DINOMOD_ROM_PATCH
                if (D_8008C890) { 
                    //Widescreen aspect
                    rcp_screen_full_write(gfx, logoDinosaurPlanet, 71, 50, 0, 0, 0xFF, 0);
                } else { 
                    //Standard aspect
                    rcp_screen_full_write(gfx, logoDinosaurPlanet, 50, 50, 0, 0, 0xFF, 0);
                }
            #else
            rcp_screen_full_write(gfx, logoDinosaurPlanet, 50, 50, 0, 0, 0xFF, 0);
            #endif
        }

        font_window_draw(gfx, NULL, NULL, 1);
    }
}
