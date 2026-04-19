#include "modding.h"
#include "recomputils.h"

#include "recomp/dlls/engine/63_gameselect_recomp.h"
#include "dlls/engine/63_gameselect.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "dlls/engine/21_gametext.h"
#include "dlls/engine/28_screen_fade.h"
#include "dlls/engine/29_gplay.h"
#include "dlls/engine/74_picmenu.h"
#include "sys/gfx/texture.h"
#include "sys/fonts.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/memory.h"
#include "sys/print.h"
#include "sys/rcp.h"
#include "dll.h"
#include "types.h"
#include "macros.h"

#include "player_stats.h"

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
        gDLL_5_AMSEQ->vtbl->stop(0);
        gDLL_5_AMSEQ->vtbl->stop(1);
        gDLL_5_AMSEQ->vtbl->stop(2);
        gDLL_5_AMSEQ->vtbl->stop(3);
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
                    set_save_game_idx(selected);
                    menu_set(MENU_ENTER_NAME);
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
                sSaveGameInfo[i].spiritBits = getCountSpirits(); //@recomp: Changing flag for consistency with Pause Menu, but may switch to 0x489 later
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
            rcp_screen_full_write(gdl, sSaveGameBgTextures[sSaveGameBgIndices[i]], x2, y2, 0, 0, 0xFF, 0);
            x2 += 64;
        }
    }

    // Draw player icon
    rcp_screen_full_write(gdl, sSaveGameTextures[saveInfo->playerno], x + 14, y + 8, 0, 0, 0xFF, 0);
    // Draw spirit icon
    rcp_screen_full_write(gdl, sSaveGameTextures[2], x + 241, y + 71, 0, 0, 0xFF, 0);
    // Draw spell stone icon
    rcp_screen_full_write(gdl, sSaveGameTextures[3], x2 + 14, y + 71, 0, 0, 0xFF, 0);

    // Draw text
    font_window_use_font(1, FONT_DINO_MEDIUM_FONT_IN);
    font_window_set_text_colour(1, 255, 255, 255, 0, 255);

    // @recomp: Display default filename if selected save name is empty
    font_window_add_string_xy(1, x + 64, y + 18, (char*)dinomod_get_save_filename(saveInfo), 1, ALIGN_TOP_LEFT);

    sprintf(sSaveGameTimeStr, "%3d:%02d:%02d", saveInfo->timeHours, saveInfo->timeMinutes, saveInfo->timeSeconds);
    font_window_add_string_xy(1, x + 156, y + 49, sSaveGameTimeStr, 1, ALIGN_TOP_CENTER);

    // @recomp: Use Spirit count
    sprintf(sSpiritCountStr, "%1d", saveInfo->spiritBits);
    font_window_add_string_xy(1, x + 234, y + 81, sSpiritCountStr, 1, ALIGN_TOP_CENTER);

    // @recomp: Use SpellStone count
    sprintf(sSpellStoneCountStr, "%1d", saveInfo->unk3);
    font_window_add_string_xy(1, x + 84, y + 81, sSpellStoneCountStr, 1, ALIGN_TOP_CENTER);
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
