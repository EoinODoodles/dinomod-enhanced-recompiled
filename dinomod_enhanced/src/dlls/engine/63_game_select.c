#include "dll_util.h"
#include "modding.h"
#include "recomputils.h"
#include "rt64_extended_gbi.h"

#include "recomp/dlls/engine/63_gameselect_recomp.h"
#include "dlls/engine/63_gameselect.h"

#include "PR/gbi.h"
#include "PR/os.h"
#include "PR/ultratypes.h"
#include "dlls/engine/21_gametext.h"
#include "dlls/engine/28_screen_fade.h"
#include "dlls/engine/29_gplay.h"
#include "dlls/engine/74_picmenu.h"
#include "game/gamebits.h"
#include "game/gametexts.h"
#include "sys/dll.h"
#include "sys/gfx/texture.h"
#include "sys/fonts.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/memory.h"
#include "sys/print.h"
#include "sys/rcp.h"
#include "dll.h"
#include "types.h"
#include "macros.h"

#include "core/fonts.h"
#include "player_stats.h"

// #define DEBUG_TASKS

extern GameSelectSubmenu sSubmenus[8];

extern PicMenuSounds sGameRecapMenuSounds;
extern GameTextChunk *sGameTextChunk;
extern s8 sSubmenuIdx;
extern s8 sSelectedSaveIdx;
extern s8 sCopyDstIdx;
extern s16 sSaveGameTextureIDs[4];
extern s16 sSaveGameBgTextureIDs[18];
extern s16 sSaveGameBgIndices[24];

extern GameSelectSaveInfo sSaveGameInfo[3];
extern s8 sCopyDstOptions[2];
extern s8 sExitTransitionTimer;
extern s8 sRedrawFrames;
extern s16 sSaveGameBoxX;
extern s16 sSaveGameBoxY;
extern char sSaveGameTimeStr[10];
extern char sSpiritCountStr[2];
extern char sSpellStoneCountStr[2];
extern u8 sExitToGame;
extern u8 sExitToMainMenu;
extern Texture *sBackgroundTexture;
extern Texture *sLogoTexture;
extern Texture *sLogoShadowTexture;
extern char sRecentTaskNumStrs[4][4];
extern Texture *sSaveGameTextures[4];
extern Texture *sSaveGameBgTextures[18];

extern void dll_63_draw_save_game_box(Gfx **gdl, s32 x, s32 y, GameSelectSaveInfo *saveInfo);
extern void dll_63_clean_up(s32 leavingMenus);
extern void dll_63_goto_game_select(s32 param1);
extern void dll_63_load_save_game_info();
extern void dll_63_goto_game_confirm();
extern void dll_63_init_submenu(GameSelectSubmenu *submenu);
extern void dll_63_goto_game_select(s32 param1);
extern void dll_63_goto_erase_select();
extern void dll_63_goto_copy_src_select();

/** If no name is entered when starting a save, display "Krystal"/"Sabre" instead of nothing */
static const char *dinomod_get_save_filename(const GameSelectSaveInfo *saveInfo) {
    if (saveInfo->filename[0] != '\0') {
        return saveInfo->filename;
    } else {
        return saveInfo->playerno == PLAYER_KRYSTAL ? "KRYSTAL" : "SABRE";
    }
}

/** Retains your save slot selection when backing out to the top-level menu page (i.e. exiting the menu page that shows info about the save slot) */
RECOMP_PATCH void dll_63_goto_game_select(s32 param1) {
    GameSelectSubmenu *submenu;
    // @recomp: Correct picmenu default selected item
    s8 selectedSlot = sSelectedSaveIdx;

    if (selectedSlot < 0 || selectedSlot >= 3){
        selectedSlot = 0;
    }

    if (sSubmenuIdx != -1) {
        gDLL_74_Picmenu->vtbl->clear_items();
    }

    sSubmenuIdx = SUBMENU_GAME_SELECT;
    sSelectedSaveIdx = -1;
    submenu = &sSubmenus[sSubmenuIdx];

    dll_63_load_save_game_info();

    dll_63_init_submenu(submenu);
    gDLL_74_Picmenu->vtbl->set_items(submenu->menuItems, submenu->count,
        /*defaultItem*/ selectedSlot, // @recomp: Don't hardcode to zero, use selected save
        /*sounds*/ NULL,
        /*param5*/ 5,
        /*param6*/ 4,
        /*textColor*/ 0, 0, 0,
        /*textHighlight*/ 0, 0, 0);
    
    sRedrawFrames = 2;
}

RECOMP_PATCH void dll_63_act_game_select(PicMenuAction action, s32 selected) {
    //@recomp: enable back navigation from game select to title screen
    if (action == PICMENU_ACTION_BACK) {
        sExitToMainMenu = TRUE;
        gDLL_28_ScreenFade->vtbl->fade(20, SCREEN_FADE_BLACK);
        // gDLL_5_AMSEQ->vtbl->stop(0);
        // gDLL_5_AMSEQ->vtbl->stop(1);
        // gDLL_5_AMSEQ->vtbl->stop(2);
        // gDLL_5_AMSEQ->vtbl->stop(3);
        sExitTransitionTimer = 35;
        return;
    }
    switch (selected) {
        case 4:
            if (action == PICMENU_ACTION_SELECT) {
                dll_63_goto_erase_select();
            }
            break;
        case 3:
            if (action == PICMENU_ACTION_SELECT) {
                dll_63_goto_copy_src_select();
            }
            break;
        default:
            // Selected a save file button
            if (action == PICMENU_ACTION_SELECT) {
                if (sSaveGameInfo[selected].isEmpty) {
                    // Go to name entry menu
                    dll_63_clean_up(0);
                    menuSetSaveGameIdx(selected);
                    menuSet(MENU_ENTER_NAME);
                } else {
                    sSelectedSaveIdx = selected;
                    sSaveGameBoxX = 56;
                    sSaveGameBoxY = 179;
                    dll_63_goto_game_confirm();
                }
            }
            break;
    }
}

/** Retains your save slot selection when backing out from the "Previously On" menu page to the save slot info page */
RECOMP_PATCH void dll_63_act_game_recap(PicMenuAction action, s32 selected) {
    if (action == PICMENU_ACTION_BACK) {
        // @recomp: Don't update selected save index here
        //sSelectedSaveIdx = selected;
        sSaveGameBoxX = 56;
        sSaveGameBoxY = 179;
        dll_63_goto_game_confirm();
    } else if (action == PICMENU_ACTION_SELECT) {
        sExitToGame = TRUE;
        gDLL_28_ScreenFade->vtbl->fade(20, SCREEN_FADE_BLACK);
        gDLL_5_AMSEQ->vtbl->stop(0);
        gDLL_5_AMSEQ->vtbl->stop(1);
        gDLL_5_AMSEQ->vtbl->stop(2);
        gDLL_5_AMSEQ->vtbl->stop(3);
        sExitTransitionTimer = 35;
    }
}

/** Read each slot's Spirit and SpellStone counts */
RECOMP_PATCH void dll_63_load_save_game_info() {
    s32 i;
    Savefile *saveFile;
    char *filenamePtr;

    for (i = 0; i < 3; i++) {
        if ((u8)gDLL_29_Gplay->vtbl->load_save(i, /*startGame*/FALSE) == 0) {
            // failed to load save?
            gDLL_29_Gplay->vtbl->erase_save(i);
            bzero(&sSaveGameInfo[i], sizeof(GameSelectSaveInfo));
            sSaveGameInfo[i].isEmpty = TRUE;
        } else {
            saveFile = &gDLL_29_Gplay->vtbl->get_state()->save.file;

            if (!saveFile->isEmpty) {
                sSaveGameInfo[i].playerno = saveFile->playerno;
                // sSaveGameInfo[i].spiritBits = get_gplay_bitstring(0x489);
                sSaveGameInfo[i].spiritBits = getCountSpirits(); //@recomp: Changing gamebit for consistency with Pause Menu, but may switch to 0x489 later
                sSaveGameInfo[i].unk3  = getCountSpellStones(); //@recomp: Store SpellStone count in unused field (which may have been intended for it!)

                filenamePtr = sSaveGameInfo[i].filename;

                gDLL_7_Newday->vtbl->convert_ticks_to_real_time(
                    saveFile->timePlayed,
                    &sSaveGameInfo[i].timeHours, &sSaveGameInfo[i].timeMinutes, &sSaveGameInfo[i].timeSeconds);

                sSaveGameInfo[i].unkA = 0;
                sSaveGameInfo[i].isEmpty = FALSE;

                bcopy(saveFile->name, filenamePtr, sizeof(saveFile->name) - 1); // 1 less to preserve null terminator
            } else {
                bzero(&sSaveGameInfo[i], sizeof(GameSelectSaveInfo));
                sSaveGameInfo[i].isEmpty = TRUE;
            }
        }
    }
}

/** Displays the player's SpellStone and Spirit count on the save info page */
/** Also, makes it so "Krystal"/"Sabre" appears as the save slot name if you don't set any name */
RECOMP_PATCH void dll_63_draw_save_game_box(Gfx **gdl, s32 x, s32 y, GameSelectSaveInfo *saveInfo) {
    s32 i;
    s32 x2;
    s32 y2;
    s32 len;

    // Draw background
    x2 = x;
    y2 = y;
    len = ARRAYCOUNT(sSaveGameBgIndices);

    for (i = 0; i < len; i++) {
        if (sSaveGameBgIndices[i] == -1) {
            x2 = x;
            y2 += 32;
        } else {
            rcpScreenFullWrite(gdl, sSaveGameBgTextures[sSaveGameBgIndices[i]], x2, y2, 0, 0, 0xFF, SCREEN_WRITE_TRANSLUCENT);
            x2 += 64;
        }
    }

    // Draw player icon
    rcpScreenFullWrite(gdl, sSaveGameTextures[saveInfo->playerno], x + 14, y + 8, 0, 0, 0xFF, SCREEN_WRITE_TRANSLUCENT);
    // Draw spirit icon
    rcpScreenFullWrite(gdl, sSaveGameTextures[2], x + 241, y + 71, 0, 0, 0xFF, SCREEN_WRITE_TRANSLUCENT);
    // Draw spell stone icon
    rcpScreenFullWrite(gdl, sSaveGameTextures[3], x2 + 14, y + 71, 0, 0, 0xFF, SCREEN_WRITE_TRANSLUCENT);

    // Draw text
    fontWindowUseFont(1, FONT_DINO_MEDIUM_FONT_IN);
    fontWindowSetTextColour(1, 255, 255, 255, 0, 255);

    // @recomp: Display default filename if selected save name is empty
    fontWindowAddStringXY(1, x + 64, y + 18, (char*)dinomod_get_save_filename(saveInfo), 1, ALIGN_TOP_LEFT);

    sprintf(sSaveGameTimeStr, "%3d:%02d:%02d", saveInfo->timeHours, saveInfo->timeMinutes, saveInfo->timeSeconds);
    fontWindowAddStringXY(1, x + 156, y + 49, sSaveGameTimeStr, 1, ALIGN_TOP_CENTER);

    // @recomp: Use Spirit count
    sprintf(sSpiritCountStr, "%1d", saveInfo->spiritBits);
    fontWindowAddStringXY(1, x + 234, y + 81, sSpiritCountStr, 1, ALIGN_TOP_CENTER);

    // @recomp: Use SpellStone count
    sprintf(sSpellStoneCountStr, "%1d", saveInfo->unk3);
    fontWindowAddStringXY(1, x + 84, y + 81, sSpellStoneCountStr, 1, ALIGN_TOP_CENTER);
}

/** Makes it so "Krystal"/"Sabre" appears as the save slot name if you don't set any name */
RECOMP_PATCH void dll_63_init_submenu(GameSelectSubmenu *submenu) {
    s32 i;
    s32 textID;
    GameSelectSaveInfo *saveGame;

    for (i = 0; i < submenu->count; i++) {
        textID = submenu->textIDs[i];

        if (textID >= 0) {
            submenu->menuItems[i].text = sGameTextChunk->strings[textID];
        } else {
            saveGame = &sSaveGameInfo[-textID - 1];

            if (saveGame->isEmpty) {
                submenu->menuItems[i].text = sGameTextChunk->strings[4];
                submenu->menuItems[i].flags &= ~1;
                submenu->menuItems[i].flags |= PICMENU_ALIGN_TEXT_CENTER;
                submenu->menuItems[i].texture.asID = -1;
            } else {
                // @recomp: Display default filename if selected save name is empty
                submenu->menuItems[i].text = (char*)dinomod_get_save_filename(saveGame);
                submenu->menuItems[i].flags &= ~PICMENU_ALIGN_TEXT_CENTER;
                submenu->menuItems[i].flags |= 1;
                submenu->menuItems[i].texture.asID = sSaveGameTextureIDs[saveGame->playerno];
            }
        }
    }
}

static char *recent_task_strs[3];
static s32 num_recent_task_strs = 0;

static void free_recent_task_strs() {
    for (s32 i = 0; i < num_recent_task_strs; i++) {
        mmFree(recent_task_strs[i]);
        recent_task_strs[i] = NULL;
    }

    num_recent_task_strs = 0;
}

/**
  * A copy of base recomp's patches for dll_63_draw, but with an extra patch fixing a bug where the "Previously On"
  * screen's task strings could overlap with each other when one wrapped onto 3 lines instead of the usual 2.
  *
  * The text might trail too far down the screen if there are multiple tasks that wrap onto 3 lines, 
  * but luckily there are no 3 adjacent gameplay tasks where that happens, at least playing through the
  * game without sequence breaks, with the language set to English. All languages need to be tested for this though!
  *
  * For fan translations: it's recommended to try to keep task descriptions to 2 lines on this screen.
  * Use 3 lines at most, and not on successive tasks if it can be avoided.
  */ 
static void dll_63_draw_custom(Gfx **gdl, Mtx **mtxs, Vertex **vtxs) {
    s32 numRecentTasks;
    s32 uly;
    s32 lry;
    s32 ulx;
    s32 lrx;
    s32 y;
    s32 i;
    GameSelectSubmenu *submenu;
    /* RECOMP */
    s32 lineCount;

    submenu = &sSubmenus[sSubmenuIdx];

    // @recomp: Always redraw
    sRedrawFrames = 100;

    if ((sExitToGame || sExitToMainMenu) && (sExitTransitionTimer <= 10)) {
        return;
    }
    
    fontWindowSetCoords(1, 0, 0,
        GET_VIDEO_WIDTH(viGetCurrentSize()) - 100,
        GET_VIDEO_HEIGHT(viGetCurrentSize()));
    fontWindowFlushStrings(1);

    fontWindowSetCoords(3, 105, 0,
        GET_VIDEO_WIDTH(viGetCurrentSize()) - 200,
        GET_VIDEO_HEIGHT(viGetCurrentSize()));
    fontWindowFlushStrings(3);

    //@recomp: set up a separate window for the dropshadow text, to ensure it wraps the same as the main text
    if (sSubmenuIdx == SUBMENU_GAME_RECAP) {
        fontWindowSetCoords(4, 105 - 2, 0,
            GET_VIDEO_WIDTH(viGetCurrentSize()) - 200 - 2,
            GET_VIDEO_HEIGHT(viGetCurrentSize()) - 2);
        fontWindowFlushStrings(4);
    }

    if (sRedrawFrames != 0) {
        // @recomp: Center background
        // TODO: the clear screen is only necessary because coming from the rolling demo, some 3d stuff still draws??
        rcpClearScreen(gdl, mtxs, CLEAR_COLOR);
        gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_CENTER, G_EX_ORIGIN_CENTER, (-640 / 2) * 4, 0, (640 / 2) * 4, 0);
        rcpScreenFullWrite(gdl, sBackgroundTexture, 0, 0, 0, 0, 0xFF, SCREEN_WRITE_CYC_COPY);
        gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);

        if (sSubmenuIdx == SUBMENU_GAME_RECAP) {
            rcpScreenFullWrite(gdl, sLogoShadowTexture, 119, 92, 0, 0, 0xFF, SCREEN_WRITE_TRANSLUCENT);
            rcpScreenFullWrite(gdl, sLogoTexture, 129, 100, 0, 0, 0xFF, SCREEN_WRITE_TRANSLUCENT);

#ifdef DEBUG_TASKS
            //Log tasks to the console
            static u8 tasksChanged = TRUE;
            if (tasksChanged) {
                tasksChanged = FALSE;

                for (s32 i = 0; i < 5; i++) {
                    recomp_printf("Task history #%d: %d (%s)\n", 
                        i, 
                        mainGetBits(BIT_Recent_Task_1 + i), 
                        gDLL_21_Gametext->vtbl->get_text(GAMETEXT_0F4_Task_Header + mainGetBits(BIT_Recent_Task_1 + i), 0)
                    );
                }
                recomp_printf("Next task: %d (%s)\n\n", 
                    mainGetBits(BIT_Recent_Task_1 + 5), 
                    gDLL_21_Gametext->vtbl->get_text(GAMETEXT_0F4_Task_Header + mainGetBits(BIT_Recent_Task_1 + 5), 0)
                );
            }

            //Use C-Down to move down the task list, allowing the full list to be checked quickly
            if (joyGetPressed(0) & D_CBUTTONS) {
                tasksChanged = TRUE;
                gDLL_30_Task->vtbl->mark_task_completed(mainGetBits(BIT_Furthest_Completed_Task));
                gDLL_30_Task->vtbl->load_recently_completed();
                sRedrawFrames = 2;
            }
#endif

            numRecentTasks = gDLL_30_Task->vtbl->get_num_recently_completed();
            if (numRecentTasks > 3) {
                numRecentTasks = 3;
            }

            fontWindowEnableWordwrap(3);
            fontWindowUseFont(1, FONT_FUN_FONT);
            fontWindowUseFont(3, FONT_FUN_FONT);
            fontWindowSetTextColour(1, 183, 139, 97, 255, 255);
            fontWindowSetTextColour(3, 183, 139, 97, 255, 255);

            // @recomp: Fix memory leak with task strings
            free_recent_task_strs();
            for (i = 0; i < numRecentTasks; i++) {
                recent_task_strs[i] = gDLL_30_Task->vtbl->get_recently_completed_task_text(i);
            }
            num_recent_task_strs = numRecentTasks;

            y = 232;
            for (i = 0; i < numRecentTasks; i++) {
                sprintf(sRecentTaskNumStrs[i], "%1d.", (int)(i + 1));
                fontWindowAddStringXY(1, 75, y, sRecentTaskNumStrs[i], 1, ALIGN_TOP_LEFT);
                fontWindowAddStringXY(3, 2, y, recent_task_strs[i], 1, ALIGN_TOP_LEFT);
                
                //@recomp: count how many lines the string will wrap onto, and insert extra vertical space if needed (to fix lines overlapping)
                lineCount = fontCountLinesWordwrap(3, recent_task_strs[i], 2);
                if (lineCount > 2) {
                    y += 40 + (13 * (lineCount - 2));
                } else {
                    y += 40;
                }
            }

            // @recomp: Use a separate window for the dropshadow, the same as window 3 but shifted by (-2, -2) 
            // This ensures the dropshadow text always wraps the same way as the main text
            y = 232 - 2;
            fontWindowSetTextColour(1, 0, 0, 0, 255, 255);
            fontWindowSetTextColour(4, 0, 0, 0, 255, 255);
            fontWindowEnableWordwrap(4);
            fontWindowUseFont(4, FONT_FUN_FONT);
            for (i = 0; i < numRecentTasks; i++) {
                sprintf(sRecentTaskNumStrs[i], "%1d.", (int)(i + 1));
                fontWindowAddStringXY(1, 75, y, sRecentTaskNumStrs[i], 1, ALIGN_TOP_LEFT);
                fontWindowAddStringXY(4, 2, y, recent_task_strs[i], 1, ALIGN_TOP_LEFT);
                
                //@recomp: count how many lines the string will wrap onto, and insert extra vertical space if needed (to fix lines overlapping)
                lineCount = fontCountLinesWordwrap(4, recent_task_strs[i], 2);
                if (lineCount > 2) {
                    y += 40 + (13 * (lineCount - 2));
                } else {
                    y += 40;
                }
            }
        } else {
            if (sSelectedSaveIdx != -1) {
                dll_63_draw_save_game_box(gdl, sSaveGameBoxX, sSaveGameBoxY, &sSaveGameInfo[sSelectedSaveIdx]);
            }

            fontWindowUseFont(1, FONT_FUN_FONT);
            fontWindowSetTextColour(1, 183, 139, 97, 255, 255);

            fontWindowAddStringXY(1, 320, 405, sGameTextChunk->strings[submenu->buttonLegendTextIdx], 1, ALIGN_TOP_CENTER);
            fontWindowSetTextColour(1, 0, 0, 0, 255, 255);
            fontWindowAddStringXY(1, 318, 403, sGameTextChunk->strings[submenu->buttonLegendTextIdx], 2, ALIGN_TOP_CENTER);
        }

        fontWindowSetCoords(2, 0, 0, 
            GET_VIDEO_WIDTH(viGetCurrentSize()) - 100, 
            GET_VIDEO_HEIGHT(viGetCurrentSize()));
        fontWindowFlushStrings(2);
        fontWindowUseFont(2, FONT_DINO_MEDIUM_FONT_IN);
        fontWindowEnableWordwrap(2);
        fontWindowSetTextColour(2, 255, 255, 255, 0, 255);

        fontWindowAddStringXY(2, 69, 61, sGameTextChunk->strings[submenu->titleTextIdx], 1, ALIGN_TOP_LEFT);
        fontWindowSetTextColour(2, 0, 0, 0, 255, 255);
        fontWindowAddStringXY(2, 64, 56, sGameTextChunk->strings[submenu->titleTextIdx], 2, ALIGN_TOP_LEFT);

        fontWindowDraw(gdl, NULL, NULL, 2);
    } else {
        // Always redraw background in case picmenu redraws
        menu_func_80010158(&ulx, &lrx, &uly, &lry);
        rcpScreenScrollWrite(gdl, sBackgroundTexture, 0, 0, uly, lry, 0xFF, SCREEN_WRITE_CYC_COPY);
    }

    // @recomp: Always redraw all
    gDLL_74_Picmenu->vtbl->redraw_all();
    gDLL_74_Picmenu->vtbl->draw(gdl);

    if (sSubmenuIdx == SUBMENU_GAME_RECAP) {
        fontWindowDraw(gdl, NULL, NULL, 4);
        fontWindowFlushStrings(4);
    }
    fontWindowDraw(gdl, NULL, NULL, 1);
    fontWindowDraw(gdl, NULL, NULL, 3);

    sRedrawFrames -= 1;
    if (sRedrawFrames < 0) {
        sRedrawFrames = 0;
    }
}

/** Hijack the print function, since base recomp already patches `dll_63_print` */
typedef s32 (*MenuPrint)(Gfx **gdl, Mtx **mtxs, Vertex **vtxs);
static MenuPrint gameselect_print_func; 

RECOMP_HOOK_DLL(dll_63_ctor) void gameselect_ctor_hook(DLLFile *dll) {
    gameselect_print_func = dinomod_hijack_dll_export(dll, 2, dll_63_draw_custom);
}

RECOMP_HOOK_RETURN_DLL(dll_63_dtor) void gameselect_dtor_hook() {
    gameselect_print_func = NULL;
}
