#include "modding.h"
#include "recompconfig.h"
#include "configs.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "PR/os.h"
#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/joypad.h"
#include "sys/fonts.h"
#include "sys/gfx/texture.h"
#include "sys/main.h"
#include "sys/dll.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "sys/rcp.h"
#include "dll.h"
#include "dll_util.h"
#include "types.h"

#include "recomp/dlls/engine/61_rareware_recomp.h"

#define KEYFRAME_40 40      //Logo fade-in start
#define KEYFRAME_50 50      //Logo faded in
#define KEYFRAME_285 285    //Logo glow start
#define KEYFRAME_620 620    //Screen fade to black

#define LOGO_FADE_DURATION 125.0f
#define LOGO_HOLD_DURATION 250.0f

#define GLOW_FADE_DURATION 72.0f

typedef enum {
    Rareware_STATE_Initial = 0,
    Rareware_STATE_Fading_In = 1,
    Rareware_STATE_Visible = 2,
    Rareware_STATE_Glowing = 3
} RarewareStates;

#define END_FADE_DURATION 60           //End of Rareware screen: duration for fade-out
#define END_WAIT_DURATION 20            //End of Rareware screen: how long to hold on black at end of fade-out
#define MAIN_MENU_FADE_IN_DURATION 120 //Start of Title Screen: duration for fade-in

#define SKIP_FADE_DURATION 10 //When player skips Rareware screen: duration for fade-out
#define SKIP_WAIT_DURATION 20 //When player skips Rareware screen: how long to hold on black at end of fade-out

#define END_FADE_BEGIN_TIME (620 - (END_FADE_DURATION + END_WAIT_DURATION))
#define END_FADE_TRANSITION_TIME (END_FADE_BEGIN_TIME + (END_FADE_DURATION + END_WAIT_DURATION))

static int rsDoSkip;
static s32 rsCutToTitleTimer;

extern s32 dFrame;
extern s8 dState;

extern s8 sFadeOutStarted;
extern s8 sFadeOutTimer;
extern s8 sCutToNextScreen;
extern f32 sLogoTimer;
extern f32 sGlowTimer;
extern Texture *sTexRareLogo;
extern Texture *sTexRareLogoGlow;
extern Texture *sTexRareware;
extern Texture *sTexRarewareGlow;

static s32 update1_hijack(void);

RECOMP_HOOK_DLL(rareware_ctor) void rareware_ctor_hook(DLLFile *dll) {
    dinomod_hijack_dll_export(dll, 0, update1_hijack);
}

static void recomp_main_menu(void) {
    extern void func_8001440C(s32 arg0);

    font_load(FONT_FUN_FONT);
    font_load(FONT_DINO_MEDIUM_FONT_IN);
    font_load(FONT_DINO_MEDIUM_FONT_OUT);
    main_load_frontend();

    vi_init(1, get_ossched(), FALSE);
    track_set_z_buffer_on(1);
    track_set_sky_on(1);

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

    //Get gUpdateRate, clamped at maximum of 3
    delay = gUpdateRate;
    if (delay > 3) {
        delay = 3;
    }

    //Decrement fadeout timer, if it's started
    if (sFadeOutTimer > 0) {
        sFadeOutTimer -= delay;
    }

    //End of shot
    if (sCutToNextScreen) {
        main_set_bits(BIT_44F, 0);

        //@recomp: title screen after Rareware
        if (recomp_get_config_u32("rolling_demo") == BOOTCONFIG_Restore_Rolling_Demo) {
            recomp_main_menu();
        } else {
            menu_set(MENU_GAME_SELECT);
        }
    }

    // @recomp: Allow skipping
    if (sFadeOutStarted == FALSE) { //Make sure this isn't clashing with the shot's own end-of-sequence fade-out
        if (rsDoSkip == FALSE) {
            //Fade to black
            if ((joy_get_pressed_raw(0) & (A_BUTTON | START_BUTTON))) {
                rsDoSkip = TRUE;
                rsCutToTitleTimer = (SKIP_FADE_DURATION + SKIP_WAIT_DURATION);
                gDLL_28_ScreenFade->vtbl->fade(SKIP_FADE_DURATION, SCREEN_FADE_BLACK);            
            }
        } else if (sFadeOutStarted == FALSE) {
            //Cut to next shot after fade-out
            rsCutToTitleTimer -= gUpdateRate;
            if (rsCutToTitleTimer <= 0) {
                sCutToNextScreen = TRUE;
            }
        }
    }

    //@recomp: fade out automatically when the Rareware sequence is nearly finished
    dFrame += gUpdateRate;
    if ((sFadeOutStarted == FALSE) && (dFrame >= END_FADE_BEGIN_TIME)) {
        sFadeOutStarted = TRUE; //do fade-out
        gDLL_28_ScreenFade->vtbl->fade(30, SCREEN_FADE_BLACK);
    }
    //Cut to next shot after fade-out finished
    if (sFadeOutStarted && dFrame >= END_FADE_TRANSITION_TIME) {
        sCutToNextScreen = TRUE;
    }

    if (sFadeOutStarted) {
        sFadeOutTimer = 45;
    }
    
    if (dState >= Rareware_STATE_Fading_In) {
        sLogoTimer -= gUpdateRateF;
    }

    if (dState >= Rareware_STATE_Glowing) {
        sGlowTimer -= gUpdateRateF;
    }

    return 0;
}

// Repositions the Rareware logo and text (originally by MusicalProgrammer)
RECOMP_PATCH void rareware_draw(Gfx **gdl, Mtx **mtxs, Vertex **vtxs) {
    f32 A;
    s32 pad;

    //Stop drawing near the end of the screen's fade-out
    if (sCutToNextScreen != 0 && sFadeOutTimer < 11) {
        return;
    }

    rcp_clear_screen(gdl, mtxs, 1);
    gDLL_76->vtbl->func2(gdl, mtxs);

    //Handle key frames
    {
        if (dFrame > KEYFRAME_40 && dState == Rareware_STATE_Initial) {
            dState = Rareware_STATE_Fading_In;
            sLogoTimer = 2*LOGO_FADE_DURATION + LOGO_HOLD_DURATION;
        }

        if (dFrame > KEYFRAME_50 && dState == Rareware_STATE_Fading_In) {
            dState = Rareware_STATE_Visible;
        }

        if (dFrame > KEYFRAME_285 && dState == Rareware_STATE_Visible) {
            dState = Rareware_STATE_Glowing;
            sGlowTimer = 2*GLOW_FADE_DURATION + 1;
        }
    }

    //Draw glows around Rare logo and Rareware text (fade in, then out)
    if (dState >= Rareware_STATE_Glowing) {
        //Handle opacity
        if (sGlowTimer <= GLOW_FADE_DURATION) {
            A = 1.0f - ((GLOW_FADE_DURATION - sGlowTimer) / GLOW_FADE_DURATION); //fading out
        } else {
            A = 1.0f - ((sGlowTimer - GLOW_FADE_DURATION) / GLOW_FADE_DURATION); //fading in
        }

        //Clamp opacity coefficient (0-1)
        if (A < 0.0f) {
            A = 0.0f;
        } else if (A > 1.0f) {
            A = 1.0f;
        }

        //@recomp: reposition logo
        rcp_screen_full_write(gdl, sTexRareLogoGlow, 42, 175, 0, 0, (s16)(255.0f * A), SCREEN_WRITE_TRANSLUCENT);
        rcp_screen_full_write(gdl, sTexRarewareGlow, 130, 208, 0, 0, (s16)(255.0f * A), SCREEN_WRITE_TRANSLUCENT);
    }

    //Draw Rare logo and Rareware text (fade in, hold for a while, then fade out)
    if (dState >= Rareware_STATE_Fading_In) {
        //Handle opacity
        if (sLogoTimer <= LOGO_FADE_DURATION) {
            A = 1.0f - ((LOGO_FADE_DURATION - sLogoTimer) / LOGO_FADE_DURATION); //out
        } else {
            A = 1.0f - ((sLogoTimer - (LOGO_FADE_DURATION + LOGO_HOLD_DURATION)) / LOGO_FADE_DURATION); //in + hold
        }

        //Clamp opacity coefficient (0-1)
        if (A < 0.0f) {
            A = 0.0f;
        } else if (A > 1.0f) {
            A = 1.0f;
        }

        //@recomp: reposition logo
        rcp_screen_full_write(gdl, sTexRareLogo, 42, 175, 0, 0, (u8)(255.0f * A), SCREEN_WRITE_TRANSLUCENT);
        rcp_screen_full_write(gdl, sTexRareware, 130, 208, 0, 0, (u8)(255.0f * A), SCREEN_WRITE_TRANSLUCENT);
    }
}
