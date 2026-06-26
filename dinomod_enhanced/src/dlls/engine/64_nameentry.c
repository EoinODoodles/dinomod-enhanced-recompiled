#include "modding.h"

#include "recomp/dlls/engine/64_nameentry_recomp.h"

#include "PR/os.h"
#include "PR/ultratypes.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/29_gplay.h"
#include "dlls/engine/74_picmenu.h"
#include "sys/joypad.h"
#include "sys/menu.h"
#include "dll.h"

extern PicMenuItem sMenuItems[];;

extern GameTextChunk *sGameTextChunk;

extern Texture *sLetterBgBoxTexture;
extern u8 sNumNameLetters;
extern char sNameLetters[10]; // 5 strings 1 character long + null terminator
extern s8 sMainRedrawFrames;
extern s8 sNameLettersRedrawFrames;
extern Texture *sBackgroundTexture;

extern void dll_64_clean_up();
extern void dll_64_draw_letters(Gfx **gdl, s32 x, s32 y);

#define BACKSPACE_BUTTON_IDX 28
#define END_BUTTON_IDX 29

//Move the selection onto the name entry screen's end/confirm button when pressing Start
RECOMP_PATCH s32 dll_64_update1() {
    s32 action;
    s32 selected;
    char name[10];
    s32 i;

    //@recomp: jump to end button when pressing Start
    if ((joy_get_pressed(0) & START_BUTTON) && 
        (gDLL_74_Picmenu->vtbl->get_selected_item() != END_BUTTON_IDX)
    ) {
        joy_disable_buttons(0, START_BUTTON);

        gDLL_74_Picmenu->vtbl->set_selected_item(END_BUTTON_IDX);
        sNameLettersRedrawFrames = 2;

        gDLL_6_AMSFX->vtbl->play(NULL, SOUND_PICMENU_MOVE, MAX_VOLUME, NULL, NULL, 0, NULL);
    }

    action = gDLL_74_Picmenu->vtbl->update();

    if (action != PICMENU_ACTION_NONE) {
        selected = gDLL_74_Picmenu->vtbl->get_selected_item();
        if (action == PICMENU_ACTION_SELECT) {
            if (selected < BACKSPACE_BUTTON_IDX && sNumNameLetters < 5) {
                bcopy(sMenuItems[selected].text, &sNameLetters[sNumNameLetters << 1], 2);
                sNumNameLetters += 1;
                sNameLettersRedrawFrames = 2;
            } else if (selected == BACKSPACE_BUTTON_IDX && sNumNameLetters != 0) {
                if (!sNumNameLetters){} // fakematch
                sNumNameLetters -= 1;
                sNameLettersRedrawFrames = 2;
            } else if (selected == END_BUTTON_IDX) {
                for (i = 0; i < sNumNameLetters; i++) {
                    name[i] = sNameLetters[i << 1];
                }
                name[sNumNameLetters] = '\0';

                gDLL_29_Gplay->vtbl->init_save(get_save_game_idx(), name);
                menu_set(MENU_GAME_SELECT);
                sNameLettersRedrawFrames = 2;
            }
        } else {
            if (sNumNameLetters != 0) {
                if (!sNumNameLetters){} // fakematch
                sNumNameLetters -= 1;
                sNameLettersRedrawFrames = 2;
            }
        }
    }

    return 0;
}
