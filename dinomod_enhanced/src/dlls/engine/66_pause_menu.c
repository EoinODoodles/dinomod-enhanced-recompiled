#include "modding.h"
#include "recomputils.h"

#include "recomp/dlls/engine/66_pausemenu_recomp.h"
#include "dlls/engine/66_pausemenu.h"

#include "PR/ultratypes.h"
#include "functions.h"
#include "dll.h"
#include "sys/joypad.h"
#include "sys/fonts.h"
#include "sys/gfx/map.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "sys/rcp.h"

#include "player_stats.h"

extern const char formatCompletionPercentage[];
extern const char formatGameplayTime[];
extern const char formatSpellStoneCount[];
extern const char formatDusterCount[];
extern const char formatSpiritCount[];

extern PicMenuItem pauseMenuItems[2];
extern PicMenuSounds pauseMenuSounds;

extern GameTextChunk* gametext;
extern Texture* textureSpellStone;
extern Texture* textureDuster;
extern Texture* textureSpirit;
extern char gameplayTime[0xa];
extern char spiritCount[0x2];
extern char spellStoneCount[0x2];
extern char dusterCount[0x2];
extern char completionPercentage[4];
extern u8 pauseScreenState;
extern s8 gameSavedMessageTimer;
extern s16 pauseMenuOpacity;

static void getPlayerStats(void){
    s8 dusters = getCountDusters();
    s8 spellStones = getCountSpellStones();
    s8 spirits = getCountSpirits();

    sprintf(dusterCount, formatDusterCount, dusters);
    sprintf(spellStoneCount, formatSpellStoneCount, spellStones);
    sprintf(spiritCount, formatSpiritCount, spirits);
}

static void printWithDropshadow(char message[], s16 x, s16 y, s32 colour_main, s32 colour_shadow, s8 opacity, s8 alignment){ 
    //Main text
    font_window_set_text_colour(1, 
        (colour_main >> 24) & 0xFF, 
        (colour_main >> 16) & 0xFF, 
        (colour_main >> 8) & 0xFF,  
        colour_main & 0xFF, 
        opacity);

    font_window_add_string_xy(1, x, y, message, 1, alignment);

    //Drop-shadow
    font_window_set_text_colour(1, 
        (colour_shadow >> 24) & 0xFF, 
        (colour_shadow >> 16) & 0xFF, 
        (colour_shadow >> 8) & 0xFF,  
        colour_shadow & 0xFF, 
        ((u8)opacity * DROP_SHADOW_MULTIPLIER) >> 8);

    font_window_add_string_xy(1, x - 1, y - 1, message, 2, alignment);
}

/** Allows the SpellStone/Spirit/Duster counters show your actual counts, and adds missing drop-shadow to completion percentage text */
RECOMP_PATCH void pausemenu_draw(Gfx** gfx, Mtx** mtx, Vertex** vtx) {
    s32 ulx;
    s32 uly;
    s32 lrx;
    s32 lry;
    s16 hours;
    s16 minutes;
    s16 seconds;
    f32 opacity_main;
    s32 opacity_drop_shadow;
    s32 screen_dimensions;
    s32 screen_width;
    s32 screen_height;
    s32 colour_main = 0xB78B61FF;
    s32 colour_shadow = 0x000000FF;

    screen_dimensions = vi_get_current_size();
    screen_width = screen_dimensions & 0xFFFF;
    screen_height = screen_dimensions >> 0x10;

    //Draw background and dimming overlay
    viewport_get_full_rect(&ulx, &uly, &lrx, &lry);
    gDPSetCombineMode(*gfx, G_CC_PRIMITIVE, G_CC_PRIMITIVE);    
    dl_apply_combine(gfx);
    gDPSetOtherMode(*gfx, 
        G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | 
        G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | 
        G_PM_NPRIMITIVE, G_AC_NONE | G_ZS_PIXEL | G_RM_CLD_SURF | G_RM_CLD_SURF2);
    dl_apply_other_mode(gfx);
    dl_set_prim_color(gfx, 0, 0, 0, pauseMenuOpacity);
    gDPFillRectangle((*gfx)++, ulx, uly, lrx, lry);
    gDLBuilder->needsPipeSync = TRUE;
    
    font_window_set_coords(1, 0, 0, screen_width, screen_height);
    font_window_flush_strings(1);
    opacity_main = ((f32) pauseMenuOpacity / BG_OVERLAY_MAX_OPACITY) * 255.0f;

    gDLL_74_Picmenu->vtbl->set_opacity((u8)opacity_main);
    gDLL_74_Picmenu->vtbl->draw(gfx);

    //Draw icons and text
    font_window_set_text_colour(1, 0xB7, 0x8B, 0x61, 0xFF, opacity_main);
    
    // font_window_use_font(1, FONT_DINO_SUBTITLE_FONT_1);
    // printWithDropshadow("Custom pause draw function!", screen_width/2, screen_height - 20,  colour_main, colour_shadow, opacity_main, ALIGN_MIDDLE_CENTER);

    font_window_use_font(1, FONT_FUN_FONT);

    switch (pauseScreenState){
        case PAUSE_MENU_GAME_SAVED:
            //Draw "Game Saved" message
            printWithDropshadow(gametext->strings[4], 160, 115, colour_main, colour_shadow, opacity_main, ALIGN_TOP_CENTER);
            break;
        default:   
            //Draw icons
            func_8003825C(gfx, textureSpellStone, 44, 136, 0, 0, opacity_main, 0);
            func_8003825C(gfx, textureDuster, 127, 162, 0, 0, opacity_main, 0);
            func_8003825C(gfx, textureSpirit, 216, 137, 0, 0, opacity_main, 0);
            
            //Draw completion percentage
            sprintf(completionPercentage, formatCompletionPercentage, gDLL_30_Task->vtbl->get_completion_percentage());
            printWithDropshadow(completionPercentage, 264, 36, colour_main, colour_shadow, opacity_main, ALIGN_TOP_CENTER);
    
            //Draw gameplay time
            gDLL_7_Newday->vtbl->convert_ticks_to_real_time(gDLL_29_Gplay->vtbl->get_time_played(), &hours, &minutes, &seconds);
            sprintf(gameplayTime, formatGameplayTime, hours, minutes, seconds);
            printWithDropshadow(gameplayTime, 74, 36, colour_main, colour_shadow, opacity_main, ALIGN_TOP_CENTER);
    
            //Change font
            font_window_use_font(1, FONT_DINO_SUBTITLE_FONT_1);
    
            //Update stats
            getPlayerStats();

            //Draw SpellStone count
            font_window_add_string_xy(1, 85, 156, spellStoneCount, 1, ALIGN_TOP_LEFT);
    
            //Draw Duster count
            font_window_add_string_xy(1, 182, 184, dusterCount, 1, ALIGN_TOP_RIGHT);
    
            //Draw Spirit count
            font_window_add_string_xy(1, 232, 156, spiritCount, 1, ALIGN_TOP_LEFT);
    
            //Draw "Game Time" and "Complete" strings
            printWithDropshadow(gametext->strings[2], 74, 21, colour_main, colour_shadow, opacity_main, ALIGN_TOP_CENTER);
            printWithDropshadow(gametext->strings[3], 264, 21, colour_main, colour_shadow, opacity_main, ALIGN_TOP_CENTER);
            break;
    }

    font_window_draw(gfx, 0, 0, 1);
}

RECOMP_PATCH s32 pausemenu_update(void) {
    s32 action;
    s32 index;
    s32 selectedItem;
    
    if (pauseScreenState == PAUSE_MENU_MAIN) {

        action = gDLL_74_Picmenu->vtbl->update();
        selectedItem = gDLL_74_Picmenu->vtbl->get_selected_item();
        
        if (action == PICMENU_ACTION_SELECT) {
            if (!selectedItem){
                gDLL_6_AMSFX->vtbl->play_sound(0, 2931, 0x7F, 0, 0, 0, 0);
                menu_set(MENU_GAMEPLAY);
                unpause();
                joy_set_button_mask(0, A_BUTTON | B_BUTTON);
            } else {
                gDLL_6_AMSFX->vtbl->play_sound(0, 2930, 0x7F, 0, 0, 0, 0);
                gameSavedMessageTimer = 0;
                pauseScreenState = 1;
                
                for (index = 0; index < 2; index++){
                    pauseMenuItems[index].flags |= 0x1000;
                }
                
                gDLL_74_Picmenu->vtbl->update_flags(pauseMenuItems);
            }
        } else if (action == PICMENU_ACTION_BACK) {
            menu_set(MENU_GAMEPLAY);
            unpause();
            joy_set_button_mask(0, A_BUTTON | B_BUTTON);
        }

    } else if (pauseScreenState == PAUSE_MENU_GAME_SAVED) {

        if (gameSavedMessageTimer == 0) {
            gDLL_29_Gplay->vtbl->save_game();
        }

        gameSavedMessageTimer += gUpdateRateF;
        if (gameSavedMessageTimer >= 120.0f) {
            pauseScreenState = 0;

            for (index = 0; index < 2; index++){
                pauseMenuItems[index].flags &= ~0x1000;
            }
            
            gDLL_74_Picmenu->vtbl->update_flags(pauseMenuItems);
            gDLL_74_Picmenu->vtbl->set_selected_item(0);
        }
    }
    
    //Gradually fade in BG overlay (and the UI elements, which depend on this value)
    pauseMenuOpacity += gUpdateRate * 8;
    if (pauseMenuOpacity > BG_OVERLAY_MAX_OPACITY) {
        pauseMenuOpacity = BG_OVERLAY_MAX_OPACITY;
    }
    
    return 0;
}
