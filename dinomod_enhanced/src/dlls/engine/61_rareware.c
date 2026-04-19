#include "modding.h"
#include "recompconfig.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "PR/os.h"
#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/joypad.h"
#include "sys/fonts.h"
#include "sys/gfx/gx.h"
#include "sys/gfx/texture.h"
#include "sys/main.h"
#include "sys/dll.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "sys/rcp.h"
#include "dll.h"
#include "dll_util.h"
#include "functions.h"
#include "types.h"

#include "recomp/dlls/engine/61_rareware_recomp.h"

#define END_FADE_DURATION 60           //End of Rareware screen: duration for fade-out
#define END_WAIT_DURATION 20            //End of Rareware screen: how long to hold on black at end of fade-out
#define MAIN_MENU_FADE_IN_DURATION 120 //Start of Title Screen: duration for fade-in

#define SKIP_FADE_DURATION 10 //When player skips Rareware screen: duration for fade-out
#define SKIP_WAIT_DURATION 20 //When player skips Rareware screen: how long to hold on black at end of fade-out

#define END_FADE_BEGIN_TIME (620 - (END_FADE_DURATION + END_WAIT_DURATION))
#define END_FADE_TRANSITION_TIME (END_FADE_BEGIN_TIME + (END_FADE_DURATION + END_WAIT_DURATION))

static int sDoSkip;
static s32 sCutToTitleTimer;

extern s32 data_0;
extern s8 data_4;
extern u32 data_8;

extern s8 bss_0;
extern s8 bss_1;
extern s8 bss_2;
extern f32 bss_4;
extern f32 bss_8;
extern Texture *bss_C;
extern Texture *bss_10;
extern Texture *bss_14;
extern Texture *bss_18;

static s32 update1_hijack(void);

RECOMP_HOOK_DLL(dll_61_ctor) void rareware_ctor_hook(DLLFile *dll) {
    dinomod_hijack_dll_export(dll, 0, update1_hijack);
}

static void recomp_main_menu(void) {
    extern void func_8001440C(s32 arg0);

    font_load(FONT_FUN_FONT);
    font_load(FONT_DINO_MEDIUM_FONT_IN);
    font_load(FONT_DINO_MEDIUM_FONT_OUT);
    main_load_frontend();

    vi_init(1, get_ossched(), FALSE);
    func_80041D20(1);
    func_80041C6C(1);

    //Load game options (makes sure languageID is initialised/remembered)
    gDLL_29_Gplay->vtbl->load_game_options();

    main_demo_reset();
    main_start_game(12457.1f, -1474.875f, -6690.398f, PLAYER_KRYSTAL);
    menu_set(MENU_TITLE_SCREEN);

    func_8001440C(1);

    //Title Screen: fade in
    gDLL_28_ScreenFade->vtbl->fade_reversed(MAIN_MENU_FADE_IN_DURATION, SCREEN_FADE_BLACK);
}

static s32 update1_hijack(void) {
    s32 delay;

    delay = gUpdateRate;
    if (delay > 3) {
        delay = 3;
    }

    if (bss_1 > 0) {
        bss_1 -= delay;
    }

    if (bss_2) {
        main_set_bits(BIT_44F, 0);
        //@recomp: title screen after Rareware
        recomp_main_menu();
    }

    // @recomp: Allow skipping
    if (bss_0 == FALSE) { //Make sure this isn't clashing with the shot's own end-of-sequence fade-out
        if (sDoSkip == FALSE) {
            //Fade to black
            if ((joy_get_pressed_raw(0) & (A_BUTTON | START_BUTTON))) {
                sDoSkip = TRUE;
                sCutToTitleTimer = (SKIP_FADE_DURATION + SKIP_WAIT_DURATION);
                gDLL_28_ScreenFade->vtbl->fade(SKIP_FADE_DURATION, SCREEN_FADE_BLACK);            
            }
        } else if (bss_0 == FALSE) {
            //Cut to next shot after fade-out
            sCutToTitleTimer -= gUpdateRate;
            if (sCutToTitleTimer <= 0) {
                bss_2 = TRUE;
            }
        }
    }

    //@recomp: fade out automatically when the Rareware sequence is nearly finished
    data_0 += gUpdateRate;
    if ((bss_0 == FALSE) && (data_0 >= END_FADE_BEGIN_TIME)) {
        bss_0 = TRUE; //do fade-out
        gDLL_28_ScreenFade->vtbl->fade(30, SCREEN_FADE_BLACK);
    }
    //Cut to next shot after fade-out finished
    if (bss_0 && data_0 >= END_FADE_TRANSITION_TIME) {
        bss_2 = TRUE;
    }

    if (bss_0) {
        bss_1 = 45;
    }
    
    if (data_4 > 0) {
        bss_4 -= gUpdateRateF;
    }
    if (data_4 > 2) {
        bss_8 -= gUpdateRateF;
    }

    return 0;
}

// Repositions the Rareware logo and text (originally by MusicalProgrammer)
RECOMP_PATCH void dll_61_draw(Gfx **gdl, Mtx **mtxs, Vertex **vtxs) {
    f32 var1;
    u8 _stackPad[4];

    if (bss_2 != 0 && bss_1 < 11) {
        return;
    }

    func_80037A14(gdl, mtxs, 1);
    gDLL_76->vtbl->func2(gdl, mtxs);

    if (data_0 > 40 && data_4 == 0) {
        data_4 = 1;
        bss_4 = 500.0f;
    }

    if (data_0 > 50 && data_4 == 1) {
        data_4 = 2;
    }

    if (data_0 > 285 && data_4 == 2) {
        data_4 = 3;
        bss_8 = 145.0f;
    }

    if (data_4 > 2) {
        if (bss_8 <= 72.0f) {
            var1 = 1.0f - ((72.0f - bss_8) / 72.0f);
        } else {
            var1 = 1.0f - ((bss_8 - 72.0f) / 72.0f);
        }

        if (var1 < 0.0f) {
            var1 = 0.0f;
        } else if (var1 > 1.0f) {
            var1 = 1.0f;
        }

        //@recomp: reposition logo
        func_8003825C(gdl, bss_10, 42, 175, 0, 0, (s16)(255.0f * var1), 0);
        func_8003825C(gdl, bss_18, 130, 208, 0, 0, (s16)(255.0f * var1), 0);
    }

    if (data_4 >= 1) {
        if (bss_4 <= 125.0f) {
            var1 = 1.0f - ((125.0f - bss_4) / 125.0f);
        } else {
            var1 = 1.0f - ((bss_4 - 375.0f) / 125.0f);
        }

        if (var1 < 0.0f) {
            var1 = 0.0f;
        } else if (var1 > 1.0f) {
            var1 = 1.0f;
        }

        //@recomp: reposition logo
        func_8003825C(gdl, bss_C, 42, 175, 0, 0, (u8)(255.0f * var1), 0);
        func_8003825C(gdl, bss_14, 130, 208, 0, 0, (u8)(255.0f * var1), 0);
    }
}
