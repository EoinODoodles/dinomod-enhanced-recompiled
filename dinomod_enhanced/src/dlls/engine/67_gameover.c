#include "modding.h"

#include "macros.h"
#include "sys/fonts.h"
#include "sys/menu.h"
#include "sys/objects.h"
#include "dlls/engine/21_gametext.h"
#include "dlls/engine/28_screen_fade.h"
#include "dlls/engine/74_picmenu.h"
#include "dlls/objects/210_player.h"
#include "dll.h"

#include "recomp/dlls/engine/67_gameover_recomp.h"

typedef enum {
    STATE_0_Use_Duster,             //"Use Duster?" choice (initial screen if the player had any upon dying). Can exit to gameplay or advance to save choice screen.
    STATE_1_Continue_Playing,       //Shown when player isn't revived. Choice between reloading save or returning to boot screen.
    STATE_2_Do_You_Wish_to_Save,    //Shown when player isn't revived. Advances to "Continue Playing?" choice.
    STATE_3_Game_Saved              //"Game Saved" message displayed. Advances to "Continue Playing?" choice.
} GameOver_ScreenStates;

typedef enum {
    ITEM_0_Game_Over,
    ITEM_1_Yes,
    ITEM_2_No,
    ITEM_3_Current_Choice       //Overridden with ITEM_1_Yes/ITEM_2_No
} GameOver_PicMenuItems;

typedef enum {
    EXIT_TO_RELOADED_SAVE = 1,  //The player didn't use a Duster, but decided to reload their save and continue playing
    EXIT_TO_BOOT_SCREEN = 2,    //The player didn't use a Duster, and decided to stop playing
    EXIT_TO_GAMEPLAY = 3        //The player used a Duster, gameplay continues immediately
} GameOver_Destinations;

extern PicMenuItem dPicmenuItems[4];

/*0x8*/ extern s16 sOpacity;
/*0xC*/ extern f32 sFadeInDelayTimer;
/*0x10*/ extern f32 sGameSavedTimer;
/*0x14*/ extern u8 sGameOverScreenState;
/*0x15*/ extern u8 sDestination;
/*0x16*/ extern s8 sTransitionTimer;

RECOMP_PATCH s32 gameover_Update1(void) {
    s32 action;
    s32 delay;
    PlayerStats* stats;
    Object* player;
    s8 prevTimer;
    s32 selectedIdx;
    u32 index;

    player = objGetPlayer();

    //Handle leaving the Game Over screen 
    {
        prevTimer = sTransitionTimer;
        
        delay = gUpdateRate;
        if (delay > 3) {
            delay = 3;
        }
        
        if (sTransitionTimer > 0) {
            sTransitionTimer -= delay;
        }    
        
        if (sDestination == EXIT_TO_RELOADED_SAVE) {
            if (gDLL_28_ScreenFade->vtbl->is_complete()) {
                gDLL_29_Gplay->vtbl->start_loaded_game();
                // @recomp: Prevent the rest of the game tick from running after we reload the save,
                //          otherwise we'll run in a mix of old state and new state. Notably, this can
                //          result in bits and saved object positions being incorrect.
                return 1;
            }
            return 0;
        }
        
        if (sDestination == EXIT_TO_GAMEPLAY) {
            return 0;
        }
        
        if (sDestination == EXIT_TO_BOOT_SCREEN) {
            if ((prevTimer >= 13) && (sTransitionTimer < 13)) {
                main_func_80013FB4();
            } else if (sTransitionTimer <= 0) {
                menuSet(MENU_POST);
            }
            
            if (sTransitionTimer < 13) {
                return 1;
            } else {
                return 0;
            }
        }
    }
    
    //Handle player's choices
    action = gDLL_74_Picmenu->vtbl->update();
    if (action != PICMENU_ACTION_NONE) {
        selectedIdx = gDLL_74_Picmenu->vtbl->get_selected_item();
        switch (sGameOverScreenState) {
        case STATE_1_Continue_Playing:
            if (action == PICMENU_ACTION_SELECT) {
                //Player chooses to reload their save and continue playing
                if (gDLL_74_Picmenu->vtbl->get_item_override(selectedIdx) == ITEM_1_Yes) {
                    gDLL_28_ScreenFade->vtbl->fade(30, SCREEN_FADE_BLACK);
                    sDestination = EXIT_TO_RELOADED_SAVE;

                //Player chooses to stop playing
                } else {
                    gDLL_28_ScreenFade->vtbl->fade(30, SCREEN_FADE_BLACK);
                    sDestination = EXIT_TO_BOOT_SCREEN;
                    sTransitionTimer = 45;
                }
            }
            break;
        case STATE_2_Do_You_Wish_to_Save:
            if (action == PICMENU_ACTION_SELECT) {
                //Player chooses to save
                if (gDLL_74_Picmenu->vtbl->get_item_override(selectedIdx) == ITEM_1_Yes) {
                    sGameSavedTimer = 0.0f;
                    sGameOverScreenState = STATE_3_Game_Saved;

                    //Hide all Picmenu items
                    for (index = 0; index < ARRAYCOUNT(dPicmenuItems); index++){
                        dPicmenuItems[index].flags |= PICMENU_INTANGIBLE;
                    }
                    
                    //Start with "Yes" option selected
                    gDLL_74_Picmenu->vtbl->set_item_override(ITEM_3_Current_Choice, ITEM_1_Yes);
                    gDLL_74_Picmenu->vtbl->update_flags(dPicmenuItems);

                //Player chooses not to save
                } else {
                    //Advance to "Continue Playing?" screen
                    sGameOverScreenState = STATE_1_Continue_Playing;

                    //Start with "Yes" option selected
                    gDLL_74_Picmenu->vtbl->set_item_override(ITEM_3_Current_Choice, ITEM_1_Yes);
                }
            }
            break;
        case STATE_0_Use_Duster:
            if (action == PICMENU_ACTION_SELECT) {
                //Player chooses to use a Duster
                if (gDLL_74_Picmenu->vtbl->get_item_override(selectedIdx) == ITEM_1_Yes) {
                    //Remove one Duster
                    stats = gDLL_29_Gplay->vtbl->get_player_stats();
                    stats->dusters--;

                    //Restore max health
                    ((DLL_210_Player*)player->dll)->vtbl->set_health(
                        player, 
                        ((DLL_210_Player*)player->dll)->vtbl->get_health_max(player)
                    );

                    //Restore max magic
                    ((DLL_210_Player*)player->dll)->vtbl->set_magic(
                        player, 
                        ((DLL_210_Player*)player->dll)->vtbl->get_magic_max(player)
                    );

                    //Revive and continue with gameplay immediately
                    sDestination = EXIT_TO_GAMEPLAY;

                //Player chooses not to be revived
                } else {
                    //Advance to save option screen
                    sGameOverScreenState = STATE_2_Do_You_Wish_to_Save;

                    //Unhide "GAME OVER" Picmenu item
                    dPicmenuItems[ITEM_0_Game_Over].flags &= ~PICMENU_INTANGIBLE;

                    //Start with "Yes" option selected
                    gDLL_74_Picmenu->vtbl->set_item_override(ITEM_3_Current_Choice, ITEM_1_Yes);
                    gDLL_74_Picmenu->vtbl->update_flags(dPicmenuItems);
                }
            }
            break;
        }
    }
    
    //Handle displaying a "Game Saved" message
    if (sGameOverScreenState == STATE_3_Game_Saved) {
        //Save the game
        if (sGameSavedTimer == 0.0f) {
            gDLL_29_Gplay->vtbl->save_game();
        }
        
        //Advance to the "Continue Playing?" screen after 2 seconds
        sGameSavedTimer += gUpdateRateF;
        if (sGameSavedTimer >= 120.0f) {
            //Advance to "Continue Playing?" screen
            sGameOverScreenState = STATE_1_Continue_Playing;
            
            //Unhide all Picmenu items
            for (index = 0; index < ARRAYCOUNT(dPicmenuItems); index++){
                dPicmenuItems[index].flags &= ~PICMENU_INTANGIBLE;
            }
            
            gDLL_74_Picmenu->vtbl->update_flags(dPicmenuItems);
        }
    }
    
    //Start fading in the Game Over screen after a 1 second delay
    sFadeInDelayTimer += gUpdateRateF;
    if (sFadeInDelayTimer >= 60.0f) {
        sOpacity += gUpdateRate * 4;
        if (sOpacity > 140) {
            sOpacity = 140;
        }
        sFadeInDelayTimer = 60.0f;
    }

    return 0;
}
