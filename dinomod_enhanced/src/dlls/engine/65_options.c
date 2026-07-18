#include "modding.h"
#include "recomputils.h"

#include "PR/gbi.h"
#include "PR/os.h"
#include "PR/ultratypes.h"
#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/21_gametext.h"
#include "dlls/engine/28_screen_fade.h"
#include "dlls/engine/29_gplay.h"
#include "dlls/engine/74_picmenu.h"
#include "dlls/engine/75_frontend.h"
#include "game/gamebits.h"
#include "macros.h"
#include "sys/camera.h"
#include "sys/gfx/textable.h"
#include "sys/gfx/texture.h"
#include "sys/fonts.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/menu.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "sys/rcp.h"
#include "types.h"

#include "core/joypad.h"
#include "button_code.h"

#include "recomp/dlls/engine/65_options_recomp.h"

static s32 rsDimOpacity = 0;
static s32 rsDimOpacityPrev = 0;
static s32 rsMessageOpacity = 0;
static s32 rsMessageOpacityPrev = 0;
static s32 rsShowMessage = 0;

#define TITLE_SHADOW(val) (val - 5)
#define DESC_SHADOW1(val) (val - 1)
#define DESC_SHADOW2(val) (val - 2)
#define DESC_SHADOW3(val) (val - 3)

#define FRONTEND_FLAG_40_Hidden 0x40      //CUSTOM FLAG: control hidden

extern void vi_set_modifiers(u8 updateViMode, s8 hStartMod, s8 vScaleMod);

#define NONE -1
#define MAX_CONTROLS_PER_PAGE 6
#define MAIN_PAGE_INDEX(pageID) (pageID - 1)
#define FADE_THRESHOLD 13

#define CHEATS_PER_SCREEN 4

//NOTE: (used to) cause a crash because the Gametext file only has 7 strings
#define TOTAL_CHEATS 32

//The index of the first cheat shown when scrolled all the way to the bottom of the cheats list
#define CHEATS_FIRST_IDX_LAST_GROUP (TOTAL_CHEATS - CHEATS_PER_SCREEN)

typedef struct {
    PicMenuItem *menuItems;
    s8 *textIDs;
    u8 count;               //Number of PicMenuItems on
    s8 boxLabel;            //LineID for the page title shown on pages that use a control box
    s8 navigationInfo;      //LineID for the navigation controls shown at the bottom of most pages
    u8 unkB;
    u8 unkC;
    u8 unkD;
} OptionsPage;

typedef enum {
    OPTIONS_PAGE_0_Main_Page,       //Top-level options page, navigating to Video/Audio/Display/Control/Cheats/Cinema pages
    OPTIONS_PAGE_1_Video,           //Choices for screen size/ratio, and navigating to Screen Position page
    OPTIONS_PAGE_2_Audio,           //Choice for setup (headphones/stereo/mono/surround), volume sliders for music/SFX
    OPTIONS_PAGE_3_Display,         //Toggles for subtitles/instruments (UI elements like the minimap)
    OPTIONS_PAGE_4_Control,         //Choice for Z-button lock (tap/hold/combo), and navigating to View Layout page
    OPTIONS_PAGE_5_Cheats,          //Scrollable box showing 4 cheats at a time
    OPTIONS_PAGE_6_Cinema,          //Inaccessible, but would display a control box like the other pages
    OPTIONS_PAGE_7_View_Layout,     //Diagram of control scheme
    OPTIONS_PAGE_8_Screen_Position  //Adjusting screen X/Y offset
} OptionsMenuPageIDs;

typedef enum {
    OPTIONS_VIDEO_0_Screen_Size,
    OPTIONS_VIDEO_1_Ratio,
    OPTIONS_VIDEO_2_Screen_Position,
    OPTIONS_VIDEO_3_Brightness //Unused
} OptionsMenu_VideoItems;

typedef enum {
    OPTIONS_AUDIO_0_Setup,
    OPTIONS_AUDIO_1_Music,
    OPTIONS_AUDIO_2_SFX,
    OPTIONS_AUDIO_3_Speech //Unused
} OptionsMenu_AudioItems;

typedef enum {
    OPTIONS_DISPLAY_0_Subtitles,
    OPTIONS_DISPLAY_1_Instruments
} OptionsMenu_DisplayItems;

typedef enum {
    OPTIONS_CONTROL_0_Z_Button,
    OPTIONS_CONTROL_1_View_Layout
} OptionsMenu_ControlItems;

typedef enum {
    OPTIONS_CHEATS_0_Up,
    OPTIONS_CHEATS_1_CheatShown1,
    OPTIONS_CHEATS_2_CheatShown2,
    OPTIONS_CHEATS_3_CheatShown3,
    OPTIONS_CHEATS_4_CheatShown4,
    OPTIONS_CHEATS_5_Down
} OptionsMenu_CheatsItems;

typedef enum {
    LINE_0_A_Select_B_Cancel,
    LINE_1_Options,

    //Main page
    LINE_2_Display,
    LINE_3_Control,
    LINE_4_Cheats,
    LINE_5_Video,
    LINE_6_Audio,
    LINE_7_Cinema,

    //Display page
    LINE_8_Subtitles,
        LINE_9_On,
        LINE_10_Off,
    LINE_11_Instruments,
        LINE_12_R_Button_Only,    //Unused, intended as a choice for "Instruments"?

    //Control page
    LINE_13_Z_Button,
        LINE_14_Combo,
        LINE_15_Tap,
        LINE_16_Hold,
    LINE_17_View_Layout,

    //Cheats page (unused, has a separate Gametext file instead)
    LINE_18_CheatA,
    LINE_19_CheatB,
    LINE_20_CheatC,

    //Video page
    LINE_21_Screen_Size,
        LINE_22_Full,
        LINE_23_Wide,
        LINE_24_Cinema,
    LINE_25_Ratio,
        LINE_26_Normal,
        LINE_27_16_by_9,
    LINE_28_Screen_Position,
    LINE_29_Brightness,     //Unused!

    //Audio page
    LINE_30_Setup,
        LINE_31_Stereo,
        LINE_32_Surround,
        LINE_33_Mono,
        LINE_34_Headphones,
    LINE_35_Music,
    LINE_36_SFX,
    LINE_37_Speech,         //Unused!

    //Cinema page (unused, has a separate Gametext file instead)
    LINE_38_Cinema0,
    LINE_39_Cinema1,

    LINE_40_OK
} MenuLines;

/*0x0*/ extern char dDownArrowChar;
/*0x4*/ extern char dUpArrowChar;
/*0x8*/ extern char dLeftArrowChar;
/*0xC*/ extern char dRightArrowChar;

/*0x10*/ extern PicMenuItem dItemsMain[];
/*0x178*/ extern s8 dTextIDsMain[];

/*0x180*/ extern PicMenuItem dItemsDisplay[];
/*0x1F8*/ extern s8 dTextIDsDisplay[];
/*0x1FC*/ extern char* dDisplayChoiceStrings[];

/*0x204*/ extern PicMenuItem dItemsControl[];
/*0x27C*/ extern s8 dTextIDsControl[];
/*0x280*/ extern char* dControlZButtonStrings[];

/*0x28C*/ extern PicMenuItem dItemsCheats[];

/*0x4E4*/ extern s8 dTextIDsVideo[];
/*0x4E8*/ extern char* dVideoSizeStrings[];
/*0x4F4*/ extern char* dVideoRatioStrings[];

/*0x4FC*/ extern PicMenuItem dItemsAudio[];
/*0x5EC*/ extern s8 dTextIDsAudio[];
/*0x5F0*/ extern char* dAudioSetupStrings[];

/*0x600*/ extern PicMenuItem dItemsCinema[];
/*0x678*/ extern u32 _data_678; //Unused, maybe texture pointers for showing 3 sequence thumbnails?
/*0x67C*/ extern u32 _data_67C;
/*0x680*/ extern u32 _data_680;

/*0x684*/ extern PicMenuItem dItemsViewLayout[];
/*0x6C0*/ extern s8 dTextIDsViewLayout[];
/*0x6C4*/ extern PicMenuItem dItemsScreenPosition[];
/*0x700*/ extern s8 dTextIDsScreenPosition[];
/*0x704*/ extern OptionsPage dMenus[];
/*0x794*/ extern GameTextChunk* dGametextMenu;
/*0x798*/ extern GameTextChunk* dGametextControls;
/*0x79C*/ extern s8 dMenuID;

/* Coords for the View Layout screen's heading labels */
/*0x7A0*/ extern s16 dLayoutCoordsHeadings[11][2];

/* Coords for the View Layout screen's description labels */
/*0x7CC*/ extern s16 dLayoutCoordsDescriptions[16][2];

/*0x80C*/ extern s16 dBoxTextureIDs[];

#define END_OF_ROW -1

typedef enum {
    BoxTexture_Edge_Bottom_1 = 0,
    BoxTexture_Edge_Bottom_2 = 1,
    BoxTexture_Corner_Bottom_Left = 2,
    BoxTexture_Corner_Bottom_Right = 3,
    BoxTexture_Corner_Top_Left = 4,
    BoxTexture_Corner_Top_Right = 5,
    BoxTexture_Edge_Left_1 = 6,
    BoxTexture_Edge_Left_2 = 7,
    BoxTexture_Edge_Left_3 = 8,
    BoxTexture_Centre_1 = 9,
    BoxTexture_Centre_2 = 10,
    BoxTexture_Centre_3 = 11,
    BoxTexture_Centre_4 = 12,
    BoxTexture_Edge_Right_1 = 13,
    BoxTexture_Edge_Right_2 = 14,
    BoxTexture_Edge_Right_3 = 15,
    BoxTexture_Edge_Top_1 = 16,
    BoxTexture_Edge_Top_2 = 17
} BoxTextureIndices;

/*0x830*/ extern s16 dBoxTextureIndices[];
/*0x88C*/ extern u8 dSpeakerModes[];

/*0x0*/ extern s8 sRedrawFrames;                    //The number of updates that should be drawn before idling
/*0x1*/ extern s8 sCheatsTopIdx;                    //The index of the first cheat shown in the scrollable box (4 cheats are shown at a time)
/*0x2*/ extern s8 sFadeOutActive;                   //For fading out and cutting back to the Title Screen
/*0x3*/ extern s8 sFadeOutTimer;                    //Timer for backing out to the Title Screen
/*0x8*/ extern char sCheatStrings[CHEATS_PER_SCREEN][50]; //A buffer for the displayed cheats' text
/*0xD0*/ extern  Texture* dBoxTextures[18];         //Textures for the multi-tile picmenu boxes, used to frame Front End UI controls
/*0x118*/ extern Texture* sBGTex;                   //The page's high-res background image
/*0x11C*/ extern Texture* sCropFrameVertical;       //A white frame shown while adjusting the screen position
/*0x120*/ extern Texture* sCropFrameHorizontal;     //A white frame shown while adjusting the screen position
/*0x124*/ extern GameTextChunk* sGametextCinema;    //Text for the cinema menu
/*0x128*/ extern GameTextChunk* sGametextCheats;    //Text for the cheats menu
/*0x12C*/ extern s8 sCtrlCount;                     //The number of Front End controllers currently being used (checkboxes/sliders/lists)
/*0x12D*/ extern s8 sTopLevelItemIdx;               //The selected item index on the main top-level page of the Options menu (used to restore selection when backing out of a submenu)
/*0x130*/ extern FrontEndControl* sCtrls[MAX_CONTROLS_PER_PAGE]; //Pointers to Front End controls (checkboxes/sliders/lists)
/*0x148*/ extern GplayOptions* sGameOptions;        //Player's saved game options

extern void options_goto_main_page(void);
extern void options_goto_control_page(s32 selectedItemIdx);
extern void options_goto_video_page(s32 selectedItemIdx);
extern int options_exit_main_page(s32 action, s32 selectedItemIdx);
extern void options_handle_action_display_page(s32 action, s32 selectedItemIdx);
extern void options_handle_action_control_page(s32 action, s32 selectedItemIdx);
extern void options_handle_action_cheats_page(s32 action, s32 selectedItemIdx);
extern void options_handle_action_video_page(s32 action, s32 selectedItemIdx);
extern void options_handle_action_audio_page(s32 action, s32 selectedItemIdx);
extern void options_handle_action_cinema_page(s32 action, s32 selectedItemIdx);
extern void options_clean_up(void);
extern void options_draw_box(Gfx** gdl, s32 initialX, s32 initialY, s32 endX, s32 endY);
extern void options_goto_screen_position_page(void);

/**
  * Checks when the player interacts with parts of the options menu that cause crashes in recomp
  * or aren't fully supported/finished - and sets up a message informing them about why the item is locked.
  */
void options_recomp_override_and_display_messages(void) {
    s32 selectedIdx = gDLL_74_Picmenu->vtbl->get_selected_item();
    s32 prevShowMessage = rsShowMessage;
    s32 buttons;
    s8 joyX;
    s8 joyY;

    //Bail if a message is already being shown
    if (rsShowMessage > 0) {
        return;
    }

    buttons = joyGetPressed(0);
    joyGetStickMenuXYSign(0, &joyX, &joyY);

    switch (dMenuID) {
#ifndef DINOMOD_ROM_PATCH //@recomp only: handle blocking certain options to avoid crashes
    case OPTIONS_PAGE_8_Screen_Position:
        //@recomp: don't allow movement on the screen position page, to avoid crashes
        if (joyX || joyY) {
            rsShowMessage = 1;
        }
        break;
    case OPTIONS_PAGE_1_Video:
        //@recomp: note that widescreen is handled separately in recomp
        if ((selectedIdx == OPTIONS_VIDEO_0_Screen_Size) && joyX) {
            rsShowMessage = 3;
            gDLL_75->vtbl->set_value(sCtrls[OPTIONS_VIDEO_0_Screen_Size], 0);
            break;
        }
        if ((selectedIdx == OPTIONS_VIDEO_1_Ratio) && joyX) {
            rsShowMessage = 3;
            gDLL_75->vtbl->set_value(sCtrls[OPTIONS_VIDEO_1_Ratio], 0);
            break;
        }
        break;
#endif
    case OPTIONS_PAGE_6_Cinema:
        //Note that this page doesn't do anything currently
        if (buttons & (A_BUTTON | START_BUTTON)) {
            rsShowMessage = 2;
        }
        break;
    }

    if ((rsShowMessage > 0) && (prevShowMessage == 0)) {
        dll_amSfx->Play(NULL, SOUND_912_Object_Refused, MAX_VOLUME, NULL, NULL, 0, NULL);
    }
}

/**
  * Queues draw updates for the next few frames.
  */
static void options_refresh_draws(void) {
    s32 i;

    gDLL_74_Picmenu->vtbl->redraw_all();
    sRedrawFrames = 2;

    //Redraw front-end controls as well
    for (i = 0; i < sCtrlCount; i++) {
        if (sCtrls[i] != NULL) {
            sCtrls[i]->redrawFrames = 4;
        }
    }
}

/**
  * Draws a black rectangle over the screen, with adjustable opacity.
  */
static void dim_screen(Gfx** gdl, Mtx **mtxs, Vertex **vtxs, u8 opacity) {
    s32 i;
    s32 i2;
    s32 uly;
    s32 lry;
    s32 ulx;
    s32 lrx;

    camViewportGetFullRect(&ulx, &uly, &lrx, &lry);

    gDPSetCombineMode(*gdl, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    dlApplyCombine(gdl);

    gDPSetOtherMode(*gdl,
                    G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP |
                        G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE,
                    G_AC_NONE | G_ZS_PIXEL | G_RM_CLD_SURF | G_RM_CLD_SURF2);
    dlApplyOtherMode(gdl);

    dlSetPrimColor(gdl, 0, 0, 0, opacity);

    gDPFillRectangle((*gdl)++, ulx, uly, lrx, lry);

    gDLBuilder->needsPipeSync = TRUE;
}

extern void main_func_8001440C(s32 arg0);

// offset: 0x1FC | func: 0 | export: 0
RECOMP_PATCH s32 options_update1(void) {
    s8 timeBefore;
    s32 selectedIdx;
    s32 i;
    s8 joyX;
    s8 joyY;
    s32 frames;
    s32 action;
    //@recomp
    s32 rCustomMessageExitEarly = FALSE;
    s32 skipPicmenuUpdate = FALSE;
    u16 buttons;

    //@recomp: make sure pause menu doesn't accidentally load when pressing start
    //(can happen sometimes when Picmenu doesn't block START_BUTTON)
    main_func_8001440C(1);

    timeBefore = sFadeOutTimer;

    frames = gUpdateRate;
    if (frames > 3) {
        frames = 3;
    }

    if (sFadeOutTimer > 0) {
        sFadeOutTimer -= frames;
    }

    //@recomp: handle blocking certain options to avoid crashes (only in recomp), 
    //         and displaying info about unfinished sections (on N64 or recomp)
    options_recomp_override_and_display_messages();
    if (rsShowMessage > 0) {
        rCustomMessageExitEarly = TRUE;
    }

    //@recomp: handle custom message
    if (rsShowMessage > 0) {
        //Dismiss message when a button is pressed
        if ((rsDimOpacity > 50) && (joyGetPressed(0) & (A_BUTTON | B_BUTTON | START_BUTTON))) {
            rsShowMessage = 0;
            dll_amSfx->Play(NULL, SOUND_PICMENU_MOVE, MAX_VOLUME, NULL, NULL, 0, NULL);
        }

        //Quit out of `options_update1` early when a message is being displayed
        rCustomMessageExitEarly = TRUE;
    }

    //Redraw all elements when needed
    if ((gDLL_28_ScreenFade->vtbl->is_complete() == FALSE) || 
        //@recomp: also redraw when a custom message is being displayed
        (rsDimOpacity > 0) || (rsMessageOpacity > 0) || (rsShowMessage > 0)
    ) {
        options_refresh_draws();
    }

    //@recomp: don't update anything else when the custom message is being displayed
    if (rCustomMessageExitEarly) {
        return 0;
    }

    //Handle leaving the Options menu and returning to the Title Screen
    if (sFadeOutActive) {
        if ((timeBefore >= FADE_THRESHOLD) && (sFadeOutTimer < FADE_THRESHOLD)) {
            viInit(1, mainGetScheduler(), 0);
            options_clean_up();
            trackSetZBufferOn(1);
            trackSetSkyOn(1);
        } else if (sFadeOutTimer <= 0) {
            mainDemoReset();
            mainStartGame(12457.1f, -1474.875f, -6690.398f, PLAYER_KRYSTAL);
            menuSet(MENU_TITLE_SCREEN);
        }

        if (sFadeOutTimer < FADE_THRESHOLD) {
            return 1;
        } else {
            return 0;
        }
    }

    /* @recomp: check whether a frontend checkbox control was selected
        This decides whether the picmenu update should be skipped 
        (to avoid a bug where the picmenu items consume the frontend checkboxes' controls) */
    if (dMenuID != OPTIONS_PAGE_0_Main_Page) {
        buttons = joyGetPressed(0);
        if (buttons & (A_BUTTON | START_BUTTON)) {

            //Iterate over frontend controls
            for (i = 0; i < sCtrlCount; i++) {
                //Only consider checkboxes
                if ((sCtrls[i] == NULL) || (sCtrls[i]->type != FRONTEND_CONTROL_Checkbox)) {
                    continue;
                }

                //Skip picmenu update when a frontend checkbox is selected and toggled with A
                if (gDLL_75->vtbl->get_highlight_state(sCtrls[i])) {
                    skipPicmenuUpdate = TRUE;
                    action = PICMENU_ACTION_NONE;
                    options_refresh_draws();

                    //Play a refusal sound when trying to toggle a checkbox that's locked
                    if ((sCtrls[i]->flags & FRONTEND_FLAG_20_Locked) && ((buttons & (A_BUTTON | START_BUTTON)) != START_BUTTON)) {
                        dll_amSfx->Play(NULL, SOUND_912_Object_Refused, MAX_VOLUME, NULL, NULL, 0, NULL);
                        break;
                    }
                    break;
                }
            }
        }
    }

    //@recomp: skip Picmenu update when needed
    if (skipPicmenuUpdate == FALSE) {
        action = gDLL_74_Picmenu->vtbl->update();
    }
    selectedIdx = gDLL_74_Picmenu->vtbl->get_selected_item();

    switch (dMenuID) {
    case OPTIONS_PAGE_0_Main_Page:
        sTopLevelItemIdx = selectedIdx;
        if (options_exit_main_page(action, selectedIdx)) {
            return 0;
        }
        break;
    case OPTIONS_PAGE_3_Display:
        options_handle_action_display_page(action, selectedIdx);
        if (action == PICMENU_ACTION_BACK) {
            sGameOptions->showSubtitles = gDLL_75->vtbl->get_value(sCtrls[OPTIONS_DISPLAY_0_Subtitles]);
            sGameOptions->showInstruments = gDLL_75->vtbl->get_value(sCtrls[OPTIONS_DISPLAY_1_Instruments]);
            options_goto_main_page();
        }
        break;
    case OPTIONS_PAGE_4_Control:
        options_handle_action_control_page(action, selectedIdx);
        if (action == PICMENU_ACTION_BACK) {
            sGameOptions->zTargetMode = gDLL_75->vtbl->get_value(sCtrls[OPTIONS_CONTROL_0_Z_Button]);
            options_goto_main_page();
        }
        break;
    case OPTIONS_PAGE_5_Cheats:
        options_handle_action_cheats_page(action, selectedIdx);
        if (action == PICMENU_ACTION_BACK) {
            for (i = 0; i < 4; i++) {
                gDLL_29_Gplay->vtbl->set_cheat_enabled(
                    (sCheatsTopIdx + i),
                    gDLL_75->vtbl->get_value(sCtrls[i + 1])
                );
            }
            options_goto_main_page();
        }
        break;
    case OPTIONS_PAGE_1_Video:
        options_handle_action_video_page(action, selectedIdx);
        if (action == PICMENU_ACTION_BACK) {
            sGameOptions->screenSizeAnamorphic = gDLL_75->vtbl->get_value(sCtrls[OPTIONS_VIDEO_0_Screen_Size]);
            sGameOptions->screenAspectRatio = gDLL_75->vtbl->get_value(sCtrls[OPTIONS_VIDEO_1_Ratio]);
            options_goto_main_page();
        }
        break;
    case OPTIONS_PAGE_2_Audio:
        options_handle_action_audio_page(action, selectedIdx);
        if (action == PICMENU_ACTION_BACK) {
            sGameOptions->audioMode = gDLL_75->vtbl->get_value(sCtrls[OPTIONS_AUDIO_0_Setup]);
            sGameOptions->volumeMusic = gDLL_75->vtbl->get_value(sCtrls[OPTIONS_AUDIO_1_Music]);
            sGameOptions->volumeAudio = gDLL_75->vtbl->get_value(sCtrls[OPTIONS_AUDIO_2_SFX]);
            options_goto_main_page();
        }
        break;
    case OPTIONS_PAGE_6_Cinema:
        options_handle_action_cinema_page(action, selectedIdx);
        if (action == PICMENU_ACTION_BACK) {
            options_goto_main_page();
        }
        break;
    case OPTIONS_PAGE_7_View_Layout:
        if (action != PICMENU_ACTION_NONE) {
            texFreeTexture(sBGTex);
            sBGTex = texLoadTexture(TEXTABLE_2DF_Paper_BG_Sabre_Cape);
            options_goto_control_page(OPTIONS_CONTROL_1_View_Layout);
            return 0;
        }
        break;
    case OPTIONS_PAGE_8_Screen_Position:
        //@recomp: avoid recomp-specific crash

#ifdef DINOMOD_ROM_PATCH
        joyGetStickMenuXYSign(0, &joyX, &joyY);
        if (joyX > 0) {
            dll_amSfx->Play(NULL, SOUND_PICMENU_MOVE, MAX_VOLUME, NULL, NULL, 0, NULL);
            sGameOptions->screenOffsetX += 1;
            if (sGameOptions->screenOffsetX > 7) {
                sGameOptions->screenOffsetX = 7;
                joyX = 0;
            }

        } else if (joyX < 0) {
            dll_amSfx->Play(NULL, SOUND_PICMENU_MOVE, MAX_VOLUME, NULL, NULL, 0, NULL);
            sGameOptions->screenOffsetX -= 1;
            if (sGameOptions->screenOffsetX < -7) {
                sGameOptions->screenOffsetX = -7;
                joyX = 0;
            }
        }

        if (joyY > 0) {
            dll_amSfx->Play(NULL, SOUND_PICMENU_MOVE, MAX_VOLUME, NULL, NULL, 0, NULL);
            sGameOptions->screenOffsetY -= 1;
            if (sGameOptions->screenOffsetY < -7) {
                sGameOptions->screenOffsetY = -7;
                joyY = 0;
            }
        } else if (joyY < 0) {
            dll_amSfx->Play(NULL, SOUND_PICMENU_MOVE, MAX_VOLUME, NULL, NULL, 0, NULL);
            sGameOptions->screenOffsetY += 1;
            if (sGameOptions->screenOffsetY > 7) {
                sGameOptions->screenOffsetY = 7;
                joyY = 0;
            }
        }

        if ((joyX != 0) || (joyY != 0)) {
            vi_set_modifiers(1, sGameOptions->screenOffsetX, sGameOptions->screenOffsetY);
        }
#endif

        if (action != PICMENU_ACTION_NONE) {
            joyResetMenuStickDelay();
            options_goto_video_page(OPTIONS_VIDEO_2_Screen_Position);
            return 0;
        }
        break;
    default:
        break;
    }

    //Update Front End UI controls
    if (dMenuID != OPTIONS_PAGE_0_Main_Page) {
        for (i = 0; i < sCtrlCount; i++) {
            if (sCtrls[i] != NULL) {
                if (i == selectedIdx) {
                    gDLL_75->vtbl->set_highlight_state(sCtrls[i], TRUE);
                } else {
                    gDLL_75->vtbl->set_highlight_state(sCtrls[i], FALSE);
                }
                gDLL_75->vtbl->update(sCtrls[i]);
            }
        }
    }

    return 0;
}

extern void options_goto_display_page(void);
extern void options_goto_cheats_page(void);
extern void options_goto_audio_page(void);

static void options_goto_cinema_page(void) {
    OptionsPage *submenu;
    s32 enabled;
    s32 unlocked;
    u32 i;
    // u32 y;

    if (dMenuID != NONE) {
        gDLL_74_Picmenu->vtbl->clear_items();
    }

    dMenuID = OPTIONS_PAGE_6_Cinema;
    submenu = &dMenus[dMenuID];

    sCtrlCount = 0;
    sCheatsTopIdx = 0;
    // y = 252;
    sCtrls[sCtrlCount] = NULL;

    //TODO: Set up three sequence thumbnails displayed in the box

    gDLL_74_Picmenu->vtbl->set_items(
        submenu->menuItems, submenu->count,
        1, NULL,
        submenu->unkC, submenu->unkD,
        0xB7, 0x8B, 0x61,
        0xFF, 0xD7, 0x3D
    );

    // gDLL_75->vtbl->set_selection_state(sCtrls[0], 1);

    sRedrawFrames = 2;
}

// offset: 0x2A1C | func: 12
RECOMP_PATCH int options_exit_main_page(s32 action, s32 selectedItemIdx) {
    if (action == PICMENU_ACTION_SELECT) {
        switch (selectedItemIdx) {
        case MAIN_PAGE_INDEX(OPTIONS_PAGE_3_Display):
            options_goto_display_page();
            return 1;
        case MAIN_PAGE_INDEX(OPTIONS_PAGE_4_Control):
            options_goto_control_page(OPTIONS_CONTROL_0_Z_Button);
            return 1;
        case MAIN_PAGE_INDEX(OPTIONS_PAGE_5_Cheats):
            options_goto_cheats_page();
            return 1;
        case MAIN_PAGE_INDEX(OPTIONS_PAGE_1_Video):
            options_goto_video_page(OPTIONS_VIDEO_0_Screen_Size);
            return 1;
        case MAIN_PAGE_INDEX(OPTIONS_PAGE_2_Audio):
            options_goto_audio_page();
            return 1;
        //@recomp: handle cinema page
        case MAIN_PAGE_INDEX(OPTIONS_PAGE_6_Cinema):
            options_goto_cinema_page();
            return 1;
        }
    } else if (action == PICMENU_ACTION_BACK) {
        //Start fadeout back to Title Screen
        gDLL_28_ScreenFade->vtbl->fade(20, 1);
        sFadeOutTimer = 35;
        sFadeOutActive = TRUE;
    }

    return 0;
}

#define DIM_OPACITY_MAX 0xC0
#define MESSAGE_OPACITY_MAX 0xFF
#define DIM_OPACITY_SPEED 8
#define MESSAGE_OPACITY_SPEED 8

typedef struct {
    s8 offsetY;
    char title[50];
    char description[500];
} OptionsErrorMessage;

static s32 strings_split_new_lines_to_array(char* source, u32 sourceLength, char* splitLinesBuffer, u32 splitLinesBufferLength, char* splitLines[], u32 splitLinesMaxCount) {
    s32 lineIdx = 0;
    
    if (sourceLength > splitLinesBufferLength) {
        recomp_eprintf("Error: source text is longer than split lines buffer");
        return 0;
    }
    
    bzero(splitLinesBuffer + sourceLength, splitLinesBufferLength - sourceLength);
    bcopy(source, splitLinesBuffer, sourceLength);
    bzero(splitLines, splitLinesMaxCount*sizeof(char*));

    splitLines[0] = splitLinesBuffer;

    //Split on new line delimiter
    for (s32 c = 0, lineIdx = 1; splitLines[0][c] != '\0'; c++) {
        if (splitLines[0][c] == '\n') {
            splitLines[0][c] = '\0';
            c++;
            splitLines[lineIdx] = &splitLines[0][c]; //store address of new line
            lineIdx++;
        }            
    }

    return lineIdx;
}

static void options_draw_dinomod_message(Gfx** gdl, Mtx **mtxs, Vertex **vtxs) {
    static s32 rsCurrentMessage;
    static s32 rsMessagePrev;

    rsDimOpacityPrev = rsDimOpacity;
    rsMessageOpacityPrev = rsMessageOpacity;

    //Store current message
    if (rsShowMessage > 0) {
        rsCurrentMessage = rsShowMessage;
    }

    //Handle dimming animation
    {
        if (rsShowMessage > 0) {
            if (rsDimOpacity < DIM_OPACITY_MAX) {
                rsDimOpacity += gUpdateRate * DIM_OPACITY_SPEED;
                
                if (rsDimOpacity > DIM_OPACITY_MAX) {
                    rsDimOpacity = DIM_OPACITY_MAX;
                }
            }

            if (rsMessageOpacity < MESSAGE_OPACITY_MAX) {
                rsMessageOpacity += gUpdateRate * MESSAGE_OPACITY_SPEED;
                
                if (rsMessageOpacity > MESSAGE_OPACITY_MAX) {
                    rsMessageOpacity = MESSAGE_OPACITY_MAX;
                }
            }
        } else {
            if (rsMessageOpacity > 0) {
                rsMessageOpacity -= gUpdateRate * MESSAGE_OPACITY_SPEED * 2;

                if (rsMessageOpacity < 0) {
                    rsMessageOpacity = 0;
                }
            }

            if (rsDimOpacity > 0) {
                rsDimOpacity -= gUpdateRate * DIM_OPACITY_SPEED;

                if (rsDimOpacity < 0) {
                    rsDimOpacity = 0;
                }
            }
        }
    }

    if (rsDimOpacity <= 0) {
        return;
    }

    dim_screen(gdl, mtxs, vtxs, (u8)rsDimOpacity);
    
    //Handle message
    {
        //Note: messages must be in uppercase, since the larger fonts don't have lowercase glyphs.
        static OptionsErrorMessage rsDinomodMessages[] = {
            {0, "NOT POSSIBLE IN RECOMP", "ADJUSTING THE SCREEN POSITION\nIS ONLY SUPPORTED ON CONSOLE."},
            {-15, "PAGE INCOMPLETE", "RARE HAD YET TO FINISH\nTHIS SECTION OF THE OPTIONS.\nIT CURRENTLY HAS NO FUNCTIONALITY."},
            {-30, "NO EFFECT IN RECOMP", "WIDESCREEN IS HANDLED DIFFERENTLY IN RECOMP.\nTHE SCREEN SIZE AND ASPECT RATIO OPTIONS\nON THIS PAGE ARE ONLY SUPPORTED ON CONSOLE.\nPLEASE CHECK THE GRAPHICS TAB IN THE\nRECOMP SETTINGS MENU TO CONFIGURE WIDESCREEN!"},
        };
        static char rsCharBuffer[1000];
        static char* rsDescriptionLines[20];
        s32 descriptionLineCount = 0;

        //Prepare message lines
        {
            //Get the description lines separately (so they can be centred with new-lines)
            if ((rsShowMessage > 0) && (rsMessagePrev != rsCurrentMessage)) {
                rsMessagePrev = rsCurrentMessage; 

                descriptionLineCount = strings_split_new_lines_to_array(
                    rsDinomodMessages[rsCurrentMessage - 1].description, ARRAYCOUNT(rsDinomodMessages[rsCurrentMessage - 1].description), 
                    rsCharBuffer, ARRAYCOUNT(rsCharBuffer), 
                    rsDescriptionLines, ARRAYCOUNT(rsDescriptionLines)
                );
            }
        }

        //Print text
        {
            #define LINE_HEIGHT_TITLE 40
            #define LINE_HEIGHT_DESCRIPTION 25
            s32 totalTextHeight = LINE_HEIGHT_TITLE + (LINE_HEIGHT_DESCRIPTION * descriptionLineCount);

            s32 resolution = viGetCurrentSize();
            s32 width = GET_VIDEO_WIDTH(resolution);
            s32 height = GET_VIDEO_HEIGHT(resolution);
            s32 x;
            s32 y;
            s32 textWindowID = 3;

            fontWindowSetCoords(textWindowID, 
                0, 0,
                width, height
            );

            fontWindowFlushStrings(textWindowID);

            x = width/2;
            y = (height/2) - (totalTextHeight/2) + rsDinomodMessages[rsCurrentMessage - 1].offsetY;

            //Print message title, with drop-shadow
            {
                fontWindowUseFont(textWindowID, FONT_DINO_MEDIUM_FONT_IN);
                
                fontWindowSetTextColour(textWindowID, 0xFF, 0xFF, 0xFF, 0, rsMessageOpacity);
                fontWindowAddStringXY(textWindowID, x, y, rsDinomodMessages[rsCurrentMessage - 1].title, 1, ALIGN_MIDDLE_CENTER);

                fontWindowSetTextColour(textWindowID, 0, 0, 0, 0xFF, rsMessageOpacity);
                fontWindowAddStringXY(textWindowID, TITLE_SHADOW(x), TITLE_SHADOW(y), rsDinomodMessages[rsCurrentMessage - 1].title, 2, ALIGN_MIDDLE_CENTER);
                
                y += LINE_HEIGHT_TITLE;
            }
            
            //Print message description, with drop-shadow
            {
                fontWindowUseFont(textWindowID, FONT_FUN_FONT);

                //Print the text lines
                for (u32 lineIdx = 0; (lineIdx <= ARRAYCOUNT(rsDescriptionLines) - 1); lineIdx++) {
                    if (rsDescriptionLines[lineIdx] == NULL) {
                        break;
                    }

                    fontWindowSetTextColour(textWindowID, 0xB7, 0x8B, 0x61, 0xFF, rsMessageOpacity);
                    fontWindowAddStringXY(textWindowID, x, y, rsDescriptionLines[lineIdx], 1, ALIGN_MIDDLE_CENTER);

                    fontWindowSetTextColour(textWindowID, 0, 0, 0, 0xFF, rsMessageOpacity);
                    fontWindowAddStringXY(textWindowID, DESC_SHADOW1(x), DESC_SHADOW1(y), rsDescriptionLines[lineIdx], 2, ALIGN_MIDDLE_CENTER);
                    fontWindowAddStringXY(textWindowID, DESC_SHADOW2(x), DESC_SHADOW2(y), rsDescriptionLines[lineIdx], 2, ALIGN_MIDDLE_CENTER);
                    fontWindowAddStringXY(textWindowID, DESC_SHADOW3(x), DESC_SHADOW3(y), rsDescriptionLines[lineIdx], 2, ALIGN_MIDDLE_CENTER);

                    y += LINE_HEIGHT_DESCRIPTION;
                }

                fontWindowDraw(gdl, NULL, NULL, textWindowID);
            }

            fontWindowFlushStrings(textWindowID);
        }
    }
}

/**
  * Prints info about the cheats page, useful while trying to fix its crashes/incomplete aspects.
  */
PRAGMA_IGNORE_PUSH("-Wunused")
static void options_debug_cheats(void) {
    if (dMenuID != OPTIONS_PAGE_5_Cheats) {
        return;
    }

    s32 totalCheats = sGametextCheats->count;
    s32 cheatsOnScreen = MIN(CHEATS_PER_SCREEN, totalCheats - sCheatsTopIdx);
    s32 firstIdxLastPage = totalCheats - (totalCheats % CHEATS_PER_SCREEN);
    s32 selectedCheatIdx = -1;
    s32 selectedPicmenuIdx = gDLL_74_Picmenu->vtbl->get_selected_item();

    for (s32 i = 0; i < sCtrlCount; i++) {
        if (sCtrls[i] == NULL) {
            continue;
        }
        if (gDLL_75->vtbl->get_highlight_state(sCtrls[i])) {
            selectedCheatIdx = sCheatsTopIdx + i - 1;
            break;
        }
    }

    options_refresh_draws();

    diPrintf("totalCheats: %d\n", totalCheats);
    diPrintf("cheatsOnScreen: %d\n", cheatsOnScreen);
    diPrintf("sCheatsTopIdx: %d\n", sCheatsTopIdx);
    diPrintf("selectedCheatIdx: %d\n", selectedCheatIdx);
    diPrintf("firstIdxLastPage: %d\n\n", firstIdxLastPage);

    diPrintf("sCtrlCount: %d\n", sCtrlCount);
    diPrintf("selectedPicmenuIdx: %d\n", selectedPicmenuIdx);
}
PRAGMA_IGNORE_POP()

RECOMP_PATCH void options_draw(Gfx** gdl, Mtx **mtxs, Vertex **vtxs) {
    s32 i;
    s32 i2;
    s32 uly;
    s32 lry;
    s32 ulx;
    s32 lrx;
    s32 end;
    OptionsPage *submenu;

    submenu = &dMenus[dMenuID];

    if (sFadeOutActive && (sFadeOutTimer <= (FADE_THRESHOLD - 3))) {
        return;
    }

    fontWindowSetCoords(1, 0, 0,
        GET_VIDEO_WIDTH(viGetCurrentSize()),
        GET_VIDEO_HEIGHT(viGetCurrentSize())
    );

    fontWindowFlushStrings(1);

    if (dMenuID == OPTIONS_PAGE_8_Screen_Position) {
        rcpScreenFullWrite(gdl, sBGTex,               0,   0,   0, 0, 0xFF, 2);

        rcpScreenFullWrite(gdl, sCropFrameVertical,   38,  36,  0, 0, 0xFF, 1);
        rcpScreenFullWrite(gdl, sCropFrameHorizontal, 38,  36,  0, 0, 0xFF, 1);
        rcpScreenFullWrite(gdl, sCropFrameVertical,   599, 36,  0, 0, 0xFF, 1);
        rcpScreenFullWrite(gdl, sCropFrameHorizontal, 38,  438, 0, 0, 0xFF, 1);

        fontWindowUseFont(1, FONT_DINO_MEDIUM_FONT_IN);
        fontWindowSetTextColour(1, 0xFF, 0xFF, 0xFF, 0, gDLL_74_Picmenu->vtbl->get_highlight_alpha());
        fontWindowAddStringXY(1, 320, 42,  &dDownArrowChar,  1, ALIGN_TOP_CENTER);
        fontWindowAddStringXY(1, 320, 400, &dUpArrowChar,    1, ALIGN_TOP_CENTER);
        fontWindowAddStringXY(1, 48,  235, &dLeftArrowChar,  1, ALIGN_TOP_LEFT);
        fontWindowAddStringXY(1, 578, 235, &dRightArrowChar, 1, ALIGN_TOP_LEFT);
    } else if (dMenuID == OPTIONS_PAGE_7_View_Layout) {
        rcpScreenFullWrite(gdl, sBGTex, 0, 0, 0, 0, 0xFF, 2);

        //Headings (and 4 C-button arrows)
        fontWindowUseFont(1, FONT_FUN_FONT);
        i2 = ARRAYCOUNT(dLayoutCoordsHeadings) + ARRAYCOUNT(dLayoutCoordsDescriptions);
        for (end = ARRAYCOUNT(dLayoutCoordsHeadings), i = 0; i < end;) {
            //Print text
            fontWindowSetTextColour(1, 0xE1, 0xAB, 0x61, 0xFF, 0xFF);
            fontWindowAddStringXY(1, dLayoutCoordsHeadings[i][0], dLayoutCoordsHeadings[i][1], dGametextControls->strings[i], 1, ALIGN_TOP_LEFT);

            //Print drop-shadow
            fontWindowSetTextColour(1, 0, 0, 0, 0xFF, 0x96);
            fontWindowAddStringXY(1, dLayoutCoordsHeadings[i][0] - 1, dLayoutCoordsHeadings[i][1] - 1, dGametextControls->strings[i], 2, ALIGN_TOP_LEFT);

            i++;
        }

        //Descriptions
        fontWindowUseFont(1, FONT_DINO_SUBTITLE_FONT_1);
        for (end = i2, i2 = 0; i < end;) {
            //Print text
            fontWindowSetTextColour(1, 0xB7, 0x8B, 0x61, 0xFF, 0xFF);
            fontWindowAddStringXY(1, dLayoutCoordsDescriptions[i2][0], dLayoutCoordsDescriptions[i2][1], dGametextControls->strings[i], 1, ALIGN_TOP_LEFT);

            //Print drop-shadow
            fontWindowSetTextColour(1, 0, 0, 0, 0xFF, 0x96);
            fontWindowAddStringXY(1, dLayoutCoordsDescriptions[i2][0] - 1, dLayoutCoordsDescriptions[i2][1] - 1, dGametextControls->strings[i], 2, ALIGN_TOP_LEFT);

            i++;
            i2++;
        }

    } else {
        if (sRedrawFrames) {
            rcpScreenFullWrite(gdl, sBGTex, 0, 0, 0, 0, 0xFF, 2);

            fontWindowUseFont(1, FONT_DINO_MEDIUM_FONT_IN);

            //Print "OPTIONS" title, with drop-shadow
            {
                fontWindowSetTextColour(1, 0xFF, 0xFF, 0xFF, 0, 0xFF);
                fontWindowAddStringXY(1, 568, 63, dGametextMenu->strings[LINE_1_Options], 1, ALIGN_TOP_RIGHT);

                fontWindowSetTextColour(1, 0, 0, 0, 0xFF, 0xFF);
                fontWindowAddStringXY(1, 563, 58, dGametextMenu->strings[LINE_1_Options], 2, ALIGN_TOP_RIGHT);
            }

            //Print a heading label for the page's box, if it uses one
            if (submenu->boxLabel != NONE) {
                fontWindowSetTextColour(1, 0xFF, 0xFF, 0xFF, 0, 0xFF);
                fontWindowAddStringXY(1, 83, 178, dGametextMenu->strings[submenu->boxLabel], 1, ALIGN_TOP_LEFT);

                fontWindowSetTextColour(1, 0, 0, 0, 0xFF, 0xFF);
                fontWindowAddStringXY(1, 78, 173, dGametextMenu->strings[submenu->boxLabel], 2, ALIGN_TOP_LEFT);
            }

            //Print the menu navigation info, if the page uses it (A-SELECT, B-CANCEL)
            if (submenu->navigationInfo != NONE) {
                fontWindowUseFont(1, FONT_FUN_FONT);

                fontWindowSetTextColour(1, 0xB7, 0x8B, 0x61, 0xFF, 0xFF);
                fontWindowAddStringXY(1, 320, 405, dGametextMenu->strings[submenu->navigationInfo], 1, ALIGN_TOP_CENTER);

                fontWindowSetTextColour(1, 0, 0, 0, 0xFF, 0xFF);
                fontWindowAddStringXY(1, 318, 403, dGametextMenu->strings[submenu->navigationInfo], 2, ALIGN_TOP_CENTER);
            }

            //Draw the box for the page's controls, if it uses one
            if (submenu->boxLabel != NONE) {
                options_draw_box(gdl, 56, 220, 0, 480);
            }
        } else {
            menu_func_80010158(&ulx, &lrx, &uly, &lry);
            rcpScreenScrollWrite(gdl, sBGTex, 0, 0, uly, lry, 0xFF, 2);

            if (submenu->boxLabel != NONE) {
                options_draw_box(gdl, 56, 220, uly, lry);
            }

            if ((dMenuID == OPTIONS_PAGE_0_Main_Page) && (uly < 100)) {
                fontWindowUseFont(1, FONT_DINO_MEDIUM_FONT_IN);

                fontWindowSetTextColour(1, 0xFF, 0xFF, 0xFF, 0, 0xFF);
                fontWindowAddStringXY(1, 568, 63, dGametextMenu->strings[LINE_1_Options], 1, ALIGN_TOP_RIGHT);

                fontWindowSetTextColour(1, 0, 0, 0, 0xFF, 0xFF);
                fontWindowAddStringXY(1, 563, 58, dGametextMenu->strings[LINE_1_Options], 2, ALIGN_TOP_RIGHT);
            }
        }

        for (i = 0; i < sCtrlCount; i++) {
            if (sCtrls[i] != NULL) {

                //@recomp: hide controls when needed (TODO: move into a patch file for the front-end controls?)
                if (sCtrls[i]->flags & FRONTEND_FLAG_40_Hidden) {
                    continue;
                }

                gDLL_75->vtbl->draw(sCtrls[i], gdl);
            }
        }
    }

    gDLL_74_Picmenu->vtbl->draw(gdl);
    fontWindowDraw(gdl, NULL, NULL, 1);

    sRedrawFrames--;
    if (sRedrawFrames < 0) {
        sRedrawFrames = 0;
    }

    //@recomp
    options_draw_dinomod_message(gdl, mtxs, vtxs);

#ifdef DEBUG_OPTIONS_CHEATS
    options_debug_cheats();
#endif
}

// offset: 0x1CF4 | func: 7
RECOMP_PATCH void options_goto_cheats_page(void) {
    OptionsPage *submenu;
    s32 enabled;
    s32 unlocked;
    u32 i;
    u32 y;
    //@recomp
    u32 totalCheats = sGametextCheats->count;
    u32 numCheatsShown;
    s32 firstIdxLastPage = (s32)(totalCheats - (totalCheats % CHEATS_PER_SCREEN));

    if (dMenuID != NONE) {
        gDLL_74_Picmenu->vtbl->clear_items();
    }

    dMenuID = OPTIONS_PAGE_5_Cheats;
    submenu = &dMenus[dMenuID];

    sCtrlCount = 0;
    sCheatsTopIdx = 0;
    y = 252;
    sCtrls[sCtrlCount] = NULL;
    sCtrlCount++;

    //@recomp: only show a cheat index if it has a string (avoids a crash)
    numCheatsShown = MIN(CHEATS_PER_SCREEN, totalCheats - sCheatsTopIdx);

    //Set up the four cheats displayed in the box
    for (i = 0; i < numCheatsShown; i++) {
        unlocked = gDLL_29_Gplay->vtbl->is_cheat_unlocked(sCheatsTopIdx + i);

        if (unlocked) {
            sprintf(sCheatStrings[i], "%2d: %s", (int) ((sCheatsTopIdx + i) + 1), sGametextCheats->strings[sCheatsTopIdx + i]);
            submenu->menuItems[i + 1].text = sCheatStrings[i];
            submenu->menuItems[i + 1].flags &= ~(PICMENU_TRANSPARENT | PICMENU_DISABLED);
        } else {
            sprintf(sCheatStrings[i], "%2d:", (int) ((sCheatsTopIdx + i) + 1));
            submenu->menuItems[i + 1].text = sCheatStrings[i];
            submenu->menuItems[i + 1].flags |= (PICMENU_TRANSPARENT | PICMENU_DISABLED);
        }

        //@recomp
        if (i < numCheatsShown - 1) {
            submenu->menuItems[i + 1].downLink = i + 2;
        } else {
            submenu->menuItems[i + 1].downLink = CHEATS_PER_SCREEN + 1;
            submenu->menuItems[CHEATS_PER_SCREEN + 1].upLink = numCheatsShown;
        }

        submenu->menuItems[i + 1].flags &= ~PICMENU_INTANGIBLE;

        enabled = gDLL_29_Gplay->vtbl->is_cheat_active(sCheatsTopIdx + i);
        sCtrls[sCtrlCount] = (FrontEndControl*)gDLL_75->vtbl->create_checkbox(
            513, (s16) y, 
            0, 1, 
            enabled ? 1 : 0
        );
        sCtrlCount++;
        gDLL_75->vtbl->set_unlock_state(sCtrls[sCtrlCount - 1], unlocked);

        y += 26;
    }

    sCtrls[sCtrlCount] = NULL;
    sCtrlCount++;

    //Don't allow scrolling up when starting at the top
    submenu->menuItems[0].flags |= PICMENU_INTANGIBLE;

    //@recomp
    if (sCheatsTopIdx < firstIdxLastPage) {
        submenu->menuItems[CHEATS_PER_SCREEN + 1].flags &= ~PICMENU_INTANGIBLE;
    }

    gDLL_74_Picmenu->vtbl->set_items(
        submenu->menuItems, submenu->count,
        1, NULL,
        submenu->unkC, submenu->unkD,
        0xB7, 0x8B, 0x61,
        0xFF, 0xD7, 0x3D
    );

    gDLL_75->vtbl->set_highlight_state(sCtrls[OPTIONS_CHEATS_1_CheatShown1], 1);

    sRedrawFrames = 2;
}

// #define ALLOW_UNLOCKING_CHEATS_WITH_C_LEFT_AND_RIGHT

// offset: 0x2D50 | func: 15
/* RECOMP_PATCH */ void options_handle_action_cheats_page(s32 action, s32 selectedItemIdx) {
    OptionsPage* submenu;
    s32 i;
    s32 cheatsHaveScrolled;
    s32 enabled;
    // u8 previousCheatsTopIdx;
    //@recomp
    u32 totalCheats = sGametextCheats->count;
    s32 numCheatsShown;
    s32 firstIdxLastPage = (s32)(totalCheats - (totalCheats % CHEATS_PER_SCREEN));
    s32 selectedCtrlIdx;
    FrontEndControl* selectedCtrl;

    submenu = &dMenus[dMenuID];

    //Debug unlocking cheats
#ifdef ALLOW_UNLOCKING_CHEATS_WITH_C_LEFT_AND_RIGHT
    {
        //@debug: Unlock the selected cheat with C-right! (Need to back out and re-enter menu before it shows up)
        if (joyGetPressed(0) & R_CBUTTONS) {
            gDLL_29_Gplay->vtbl->unlock_cheat(((sCheatsTopIdx + selectedItemIdx) - 1));
            
            //@recomp: unlock the control immediately
            gDLL_75->vtbl->set_unlock_state(sCtrls[selectedItemIdx], TRUE);
            submenu->menuItems[selectedItemIdx].flags &= ~(PICMENU_TRANSPARENT | PICMENU_DISABLED);
            sprintf(sCheatStrings[selectedItemIdx - 1], "%2d: %s", (int)(sCheatsTopIdx + selectedItemIdx), sGametextCheats->strings[sCheatsTopIdx + selectedItemIdx - 1]);
            submenu->menuItems[selectedItemIdx].text = sCheatStrings[selectedItemIdx - 1];
            gDLL_74_Picmenu->vtbl->update_text(submenu->menuItems);
            gDLL_74_Picmenu->vtbl->update_flags(submenu->menuItems);
            options_refresh_draws();
            
        //@recomp: add a way to re-lock cheat as well
        } else if (joyGetPressed(0) & L_CBUTTONS) {
            sGameOptions->cheatsUnlocked &= ~(1 << ((sCheatsTopIdx + selectedItemIdx) - 1));
            
            //@recomp: lock the control immediately
            gDLL_75->vtbl->set_unlock_state(sCtrls[selectedItemIdx], FALSE);
            submenu->menuItems[selectedItemIdx].flags |= (PICMENU_TRANSPARENT | PICMENU_DISABLED);
            sprintf(sCheatStrings[selectedItemIdx - 1], "%2d: ", (int)(sCheatsTopIdx + selectedItemIdx));
            submenu->menuItems[selectedItemIdx].text = sCheatStrings[selectedItemIdx - 1];
            gDLL_74_Picmenu->vtbl->update_text(submenu->menuItems);
            gDLL_74_Picmenu->vtbl->update_flags(submenu->menuItems);
            options_refresh_draws();
        }
    }
#endif

    //@recomp: update cheat value as soon as a checkbox is toggled
    {
        selectedCtrlIdx = gDLL_74_Picmenu->vtbl->get_selected_item();
        selectedCtrl = sCtrls[selectedCtrlIdx];

        if (selectedCtrl && (selectedCtrl->flags & FRONTEND_FLAG_10_Value_Changed)) {
#ifdef DEBUG_OPTIONS_CHEATS
            diPrintf("SET CHEAT %d: %d\n", 
                sCheatsTopIdx + selectedCtrlIdx - 1, 
                gDLL_75->vtbl->get_value(sCtrls[selectedCtrlIdx])
            );
#endif

            gDLL_29_Gplay->vtbl->set_cheat_enabled(
                sCheatsTopIdx + selectedCtrlIdx - 1, 
                gDLL_75->vtbl->get_value(sCtrls[selectedCtrlIdx])
            );
        }
    }

    if (action != PICMENU_ACTION_SELECT) {
        return;
    }

    // previousCheatsTopIdx = sCheatsTopIdx;

    //Check whether the cheats have scrolled
    cheatsHaveScrolled = FALSE;
    if (selectedItemIdx == 0) {
        //Scroll up arrow selected
        sCheatsTopIdx -= CHEATS_PER_SCREEN;
        if (sCheatsTopIdx < 0) {
            sCheatsTopIdx = 0;
        } else {
            cheatsHaveScrolled = TRUE;
        }
    } else if (selectedItemIdx == (CHEATS_PER_SCREEN + 1)) {
        //Scroll down arrow selected (NOTE: causes a crash because the Gametext file only has 7 strings)
        sCheatsTopIdx += CHEATS_PER_SCREEN;
        
        //@recomp: handle last index based on strings available
        if (sCheatsTopIdx > firstIdxLastPage) {
            sCheatsTopIdx = firstIdxLastPage;
        } else {
            cheatsHaveScrolled = TRUE;
        }
    }
    if (cheatsHaveScrolled == FALSE) {
        return;
    }

    //Show the up/down navigation arrows when it's possible to scroll
    submenu->menuItems[0].flags &= ~PICMENU_INTANGIBLE;
    submenu->menuItems[(CHEATS_PER_SCREEN + 1)].flags &= ~PICMENU_INTANGIBLE;
    if (sCheatsTopIdx == 0) {
        submenu->menuItems[0].flags |= PICMENU_INTANGIBLE;
    }
    if (sCheatsTopIdx == firstIdxLastPage) {
        submenu->menuItems[(CHEATS_PER_SCREEN + 1)].flags |= PICMENU_INTANGIBLE;
    }

    //Enable/disable the 4 visible cheats based on their checkbox control's state 
    //@recomp: comment this out and instead update cheats as soon as a checkbox is toggled (earlier in this function)
    // for (i = 0; i < CHEATS_PER_SCREEN; i++) {
    //     gDLL_29_Gplay->vtbl->set_cheat_enabled(
    //         previousCheatsTopIdx + i, 
    //         gDLL_75->vtbl->get_value(sCtrls[i + 1])
    //     );
    // }

    //@recomp: only show a cheat index if it has a string (avoids a crash)
    numCheatsShown = MIN(CHEATS_PER_SCREEN, totalCheats - sCheatsTopIdx);

    //Update the cheats' strings and checkbox controls
    for (i = 0; i < numCheatsShown; i++) {
        if (gDLL_29_Gplay->vtbl->is_cheat_unlocked(sCheatsTopIdx + i)) {
            sprintf(sCheatStrings[i], "%2d: %s", (int)(sCheatsTopIdx + i + 1), sGametextCheats->strings[sCheatsTopIdx + i]);
            submenu->menuItems[i + 1].text = sCheatStrings[i];
            submenu->menuItems[i + 1].flags &= ~(PICMENU_TRANSPARENT | PICMENU_DISABLED);
            gDLL_75->vtbl->set_unlock_state(sCtrls[i + 1], 1);

            enabled = gDLL_29_Gplay->vtbl->is_cheat_active(sCheatsTopIdx + i);
            gDLL_75->vtbl->set_value(sCtrls[i + 1], enabled ? 1 : 0);
        } else {
            sprintf(sCheatStrings[i], "%2d:", (int)(sCheatsTopIdx + i + 1));
            submenu->menuItems[i + 1].text = sCheatStrings[i];
            submenu->menuItems[i + 1].flags |= (PICMENU_TRANSPARENT | PICMENU_DISABLED);
            gDLL_75->vtbl->set_unlock_state(sCtrls[i + 1], 0);
            gDLL_75->vtbl->set_value(sCtrls[i + 1], 0);
        }

        sCtrls[i + 1]->flags &= ~FRONTEND_FLAG_40_Hidden;

        //@recomp: reconfigure downLinks/upLinks
        if (i < numCheatsShown - 1) {
            submenu->menuItems[i + 1].downLink = (i + 2);
        } else {
            submenu->menuItems[i + 1].downLink = CHEATS_PER_SCREEN + 1;
            submenu->menuItems[CHEATS_PER_SCREEN + 1].upLink = numCheatsShown;
        }

        submenu->menuItems[1 + i].flags &= ~PICMENU_INTANGIBLE;
    }

    //@recomp: end the list early when there aren't enough cheats to fill out all 4 slots
    for (i = numCheatsShown; i < CHEATS_PER_SCREEN; i++) {
        submenu->menuItems[1 + i].flags |= PICMENU_INTANGIBLE;
        sCtrls[i + 1]->flags |= FRONTEND_FLAG_40_Hidden;
    }

    gDLL_74_Picmenu->vtbl->update_text(submenu->menuItems);
    gDLL_74_Picmenu->vtbl->update_flags(submenu->menuItems);

    if (sCheatsTopIdx == 0) {
        gDLL_75->vtbl->set_highlight_state(sCtrls[OPTIONS_CHEATS_1_CheatShown1], 1);
        gDLL_74_Picmenu->vtbl->set_selected_item(OPTIONS_CHEATS_1_CheatShown1);
    } else if (sCheatsTopIdx == firstIdxLastPage) {
        gDLL_75->vtbl->set_highlight_state(sCtrls[OPTIONS_CHEATS_4_CheatShown4], 1);
        gDLL_74_Picmenu->vtbl->set_selected_item(OPTIONS_CHEATS_4_CheatShown4);
    }

    //@recomp: make sure the selection isn't stuck on a hidden picmenu item
    if ((submenu->menuItems[CHEATS_PER_SCREEN + 1].flags & PICMENU_INTANGIBLE) && 
        (gDLL_74_Picmenu->vtbl->get_selected_item() > numCheatsShown)
    ) {
        gDLL_74_Picmenu->vtbl->set_selected_item(numCheatsShown);
    }
    
    sRedrawFrames = 2;
}

static void dinomod_setup_standard_resolution(void) {
    extern void main_func_8001440C(s32 arg0);

    mainSetBits(BIT_Menus_Selection_Blocked, 0);
    gDLL_29_Gplay->vtbl->load_game_options();

    viInit(1, mainGetScheduler(), FALSE);
    trackSetZBufferOn(1);
    trackSetSkyOn(1);

    main_func_8001440C(1);
}

static void dinomod_goto_old_title_screen_menu(void) {
    dinomod_setup_standard_resolution();
    menuSet(MENU_OLD_TITLE_SCREEN);
}

/* Add a secret way of accessing the old Title Screen and Level Select */
RECOMP_HOOK_DLL(options_update1) void options_secret_code_goto_old_menus(void) {
    /* L V L S */
    static u16 rsCheatCode[] = {
        L_TRIG,
        D_JPAD,
        L_TRIG,
        START_BUTTON
    };
    static ButtonCode rsOldMenusCheat;
    static s32 rsTransitionTimer = 0;

    //Set up button code
    if (rsOldMenusCheat.initialised == FALSE) {
        button_code_setup(
            &rsOldMenusCheat, 
            rsCheatCode, 
            ARRAYCOUNT(rsCheatCode)
        );
    }

    //Handle fadeout transition
    if (rsTransitionTimer > 0) {
        rsTransitionTimer += gUpdateRate;

        //Block buttons and joystick during fadeout
        joyDisableButtons(0, START_BUTTON | A_BUTTON);
        joy_disable_stick(0);

        if (rsTransitionTimer > 70) {
            rsTransitionTimer = 0;
            dinomod_goto_old_title_screen_menu();
        }

        return;
    }

    //Check if the button sequence was entered
    if (!rsOldMenusCheat.finished && button_code_entered(&rsOldMenusCheat)) {
        dll_amSfx->Play(NULL, 
            SOUND_5EB_Magic_Refill_Chime, 
            MAX_VOLUME, 0, 0, 0, 0
        );
        
        rsTransitionTimer = 1;
        gDLL_28_ScreenFade->vtbl->fade(60, SCREEN_FADE_BLACK);
        
        joyDisableButtons(0, START_BUTTON | A_BUTTON);

        button_code_reset(&rsOldMenusCheat);
    }    
}
