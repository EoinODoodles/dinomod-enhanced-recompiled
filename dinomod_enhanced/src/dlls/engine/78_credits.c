#include "recomputils.h"
#include "modding.h"
#include "recompconfig.h"
#include "dll_util.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "PR/os.h"
#include "sys/fonts.h"
#include "sys/gfx/texture.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/objects.h"
#include "sys/rcp.h"
#include "sys/vi.h"
#include "types.h"
#include "dlls/engine/21_gametext.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/engine/78_credits_recomp.h"

#include "engine/20_screens.h"
#include "engine/78_credits.h"

extern s32 D_8008C890;

typedef struct {
    u16 frameIn;        //Line starts fading in
    u16 frameHold;      //Line finished fading in, holds at max opacity
    u16 frameOut;       //Line starts fading out
    u16 frameFinished;  //Line finished fading out, holds at zero opacity
    s8 textID;          //String number in the credits' text file (`gametext1FD`)
    s8 lineIndex;       //Decides y position for string
    s8 alignment;       //Screen left/right (`CreditsAlignments`), also decides text colour (left for headings)
    u8 opacity;         //Opacity of the line's text
    f32 spacing;        //Font tracking/character spacing (expands when group line's group finished)
} CreditsLine;

typedef struct {
    CreditsLine lines[9];   //Section heading and developer names
    u16 frameExpand;        //Group lines' horizontal text spacing animates outwards once this frame is reached
    u16 frameFinished;      //Advances to the next group once this frame is reached
    u8 lineCount;           //Count of nonzero items in lines
} CreditsGroup;

typedef enum {
    CREDITS_L,
    CREDITS_R
} CreditsAlignments;

/*0x0*/ extern CreditsGroup dCredits[10];

/*0x0*/ extern Texture* sDinosaurPlanetLogoTex; //Shown at the beginning of the credits
/*0x4*/ extern u8 sGroupIdx;                    //The index of the current group of names
/*0x8*/ extern GameTextChunk* sText;            //Gametext file 0x1FD
/*0xC*/ extern f32 sTime;                       //The current frame of the credits

#define MAX_OPACITY 0xFF

#define BASE_X_LEFT 19
#define BASE_X_RIGHT 299
#define BASE_Y 53

#define FRAME_END 7350

/* For restoring the credits' playback frame when the DLL is reloaded (i.e. pausing/unpausing) */
static f32 rsCreditsRestoredFrame = 0;

/* For tracking how long the credits have been playing without an important animation sequence playing 
   (used to ensure credits don't accidentally persist into gameplay) */
static u32 rsCreditsFramesWithoutSequence = 0;

static char rsCreditsLeadVoices[3][64] = {
    "LEAD VOICES", 
    "EVELINE FISCHER",
    "STEVE MALPASS",
};

RECOMP_HOOK_DLL(credits_ctor) void credits_ctor_hook(void* dll) {
    //Insert credits for Krystal/Sabre's voice actors
    //Also retime things so names aren't fading in during a transition
    {
        CreditsGroup* audio = &dCredits[7];
        CreditsLine linesEdited[9] = {
            {5067, 5107, 5233, 5273, 29, 1, CREDITS_L, 0, 0},
            {5067, 5107, 5233, 5273, 30, 2, CREDITS_R, 0, 0},
            {5193, 5233, 5359, 5399, 31, 3, CREDITS_L, 0, 0},
            {5193, 5233, 5359, 5399, 32, 4, CREDITS_R, 0, 0},
            {5319, 5359, 5514, 5554, /*(replaced)*/ 33, 5, CREDITS_L, 0, 0},
            {5319, 5359, 5514, 5554, /*(replaced)*/ 33, 6, CREDITS_R, 0, 0},
            {5372, 5412, 5514, 5554, /*(replaced)*/ 33, 7, CREDITS_R, 0, 0},
            {5554, 5594, 5720, 5760, 33, 8, CREDITS_L, 0, 0},
            {5554, 5594, 5720, 5760, 34, 9, CREDITS_R, 0, 0}
        };
        audio->frameExpand = 5720;
        audio->frameFinished = 5760;
        audio->lineCount += 3;

        //Insert credits for Krystal/Sabre's voice actors
        bcopy(linesEdited, audio->lines, sizeof(linesEdited));
    }

    //Shift animator credits slightly so they don't get obscured by a shot transition
    {
        CreditsGroup* animators = &dCredits[5];
        animators->frameExpand += 60;
        animators->frameFinished += 60;
        for (int i = 0; i < animators->lineCount; i++) {
            animators->lines[i].frameIn += 132;
            animators->lines[i].frameHold += 132;
            animators->lines[i].frameOut += 60;
            animators->lines[i].frameFinished += 60;
        }

        //Shift in-point of next group slightly so animator group doesn't overlap with it
        CreditsGroup* design = &dCredits[6];
        for (int i = 0; i < design->lineCount; i++) {
            design->lines[i].frameIn += 60;
            design->lines[i].frameHold += 60;
        }
    }
}

/** 
  * - Make sure gameplay menu is restored at end of credits.
  * - End credits when a Screens image is shown (Krystal's Adventure).
  * - Restore playback when unpausing.
  */
RECOMP_PATCH s32 credits_update1(void) {
    CreditsLine* line;
    s32 i;
    f32 tValue;
    u8 opacity;

    //@recomp: restore playback position
    if (rsCreditsRestoredFrame > sTime) {
        sTime = rsCreditsRestoredFrame;
    }

    //@recomp: stop drawing if a Screens image is being shown (Krystal's Adventure)
    if (screens_is_screen_visible()) {
        credits_finish();
        return 0;
    }

    if (sGroupIdx < (s32)ARRAYCOUNT(dCredits)) {
        //Advance credits' time
        sTime += gUpdateRateF;
        
        //Advance to the next group of names
        if (sTime >= dCredits[sGroupIdx].frameFinished) {
            sGroupIdx++;
        }
        
        //Update the current text group's opacity and spacing animation
        if (sGroupIdx < (s32)ARRAYCOUNT(dCredits)) {
            //Iterate over the group's lines
            for (i = 0; i < dCredits[sGroupIdx].lineCount; i++) {
                line = &dCredits[sGroupIdx].lines[i];

                //Line hasn't shown up yet, not visible
                if (sTime < line->frameIn) {
                    opacity = 0;

                //Line fading in
                } else if (sTime < line->frameHold) {
                    //Get opacity tValue for fade-in
                    tValue = (sTime - line->frameIn) / (line->frameHold - line->frameIn);
                    if (tValue < 0.0f) {
                        tValue = 0.0f;
                    } else if (tValue > 1.0f) {
                        tValue = 1.0f;
                    }
                    
                    opacity = (u8)(tValue * MAX_OPACITY);

                //Line holding at max opacity
                } else if (sTime < line->frameOut) {
                    opacity = MAX_OPACITY;

                //Line fading out
                } else if (sTime < line->frameFinished) {
                    tValue = (sTime - line->frameOut) / (line->frameFinished - line->frameOut);
                    if (tValue < 0.0f) {
                        tValue = 0.0f;
                    } else if (tValue > 1.0f) {
                        tValue = 1.0f;
                    }
                    
                    opacity = MAX_OPACITY - (u8)(tValue * MAX_OPACITY);

                //Line finished fading out
                } else {
                    opacity = 0;
                }
                
                line->opacity = opacity;
                
                //Animate group's text spacing expanding outwards before it disappears
                if ((sTime >= line->frameIn) && 
                    (sTime <= line->frameFinished) && 
                    (sTime >= dCredits[sGroupIdx].frameExpand)
                ) {
                    // line->spacing += (gUpdateRateF / 60.0f) * 8.0f;

                    //@recomp: restore text tracking animation when unpausing
                    line->spacing = (sTime - dCredits[sGroupIdx].frameExpand) * 0.1333f;
                }
            }
        }
    }
    
    //@recomp: make sure gameplay menu is restored at end
    if (sTime >= FRAME_END) {
        credits_finish();
        return 0;
    } else if (sTime == 0) {
        rsCreditsRestoredFrame = 0;
    }

    //@recomp: keep track of playback position so it can be restored when unpausing
    if (rsCreditsRestoredFrame < sTime) {
        rsCreditsRestoredFrame = sTime;
    }

    return 0;
}

/**
  * - Centres the Dinosaur Planet logo in widescreen mode
  *   (not needed in recomp, only for ROM patches)
  *
  * - Stops drawing if a Screens image is being shown.
  * 
  * - Splices in static text for lead voice actors.
  */
RECOMP_PATCH void credits_draw(Gfx** gdl, Mtx** mtx, Vertex** vtx) {
    CreditsLine* line;
    s32 align;
    s32 i;
    s32 x;
    s32 y;
    char* string;

    if (sGroupIdx == ARRAYCOUNT(dCredits)) {
        return;
    }

    //@recomp: stop drawing if a Screens image is being shown (Krystal's Adventure)
    if (screens_is_screen_visible()) {
        return;
    }
    
    //Set up text window and font
    font_window_set_coords(
        1, 0, 0, 
        GET_VIDEO_WIDTH(vi_get_current_size()), 
        GET_VIDEO_HEIGHT(vi_get_current_size())
    );
    font_window_flush_strings(1);
    font_window_use_font(1, FONT_DINO_SUBTITLE_FONT_1);

    if (sGroupIdx == 0) {
        //Draw the Dinosaur Planet logo
        line = &dCredits[sGroupIdx].lines[0];

        //@rom-patch: centre logo in widescreen
        #ifdef DINOMOD_ROM_PATCH
        if (D_8008C890) { 
            //Widescreen aspect
            rcp_screen_full_write(gdl, bss_0, 45, 68, 0, 0, line->opacity, 0);
        } else { 
            //Standard aspect
            rcp_screen_full_write(gdl, bss_0, 45, 76, 0, 0, line->opacity, 0);
        }
        #else
        rcp_screen_full_write(gdl, sDinosaurPlanetLogoTex, 45, 76, 0, 0, line->opacity, 0);
        #endif
    } else {
        //Draw the developer credits
        for (i = 0; i < dCredits[sGroupIdx].lineCount; i++) {
            line = &dCredits[sGroupIdx].lines[i];
            
            if (line->opacity == 0) {
                continue;
            }
            
            //Set text colour, alignment, and screen x
            if (line->alignment == CREDITS_L) {
                //Section headings
                font_window_set_text_colour(1, 0xFF, 0xFF, 0xFF, 0xFF, line->opacity);
                align = ALIGN_TOP_LEFT;
                x = BASE_X_LEFT;
            } else {
                //Developer names
                font_window_set_text_colour(1, 0x98, 0x9F, 0xBA, 0xFF, line->opacity);
                align = ALIGN_TOP_RIGHT;
                x = BASE_X_RIGHT;
            }
            
            y = BASE_Y + ((line->lineIndex - 1) << 4);

            //@recomp: insert extra lines for Krystal/Sabre voice actors
            string = sText->strings[line->textID];

            if (sGroupIdx == 7) {
                switch (i) {
                case 4:
                    string = rsCreditsLeadVoices[0];
                    break;
                case 5:
                    string = rsCreditsLeadVoices[1];
                    break;
                case 6:
                    string = rsCreditsLeadVoices[2];
                    break;
                }
            }

            //Text
            font_window_set_extra_char_spacing(1, line->spacing);
            font_window_add_string_xy(1, x, y, string, 1, align);
            
            //Text drop-shadow
            font_window_set_text_colour(1, 0, 0, 0, 0xFF, line->opacity);
            font_window_add_string_xy(1, x - 2, y - 2, string, 2, align);
        }
    }
    
    font_window_draw(gdl, 0, 0, 1);
}

/* Counts how many frames have elapsed without an important sequence playing (one that takes over player control) */
static void credits_count_frames_without_sequence(void) {
    Object* player = get_player();
    if (!player) {
        return;
    }

    //Check if the player Object isn't involved in a sequence
    if ((player->stateFlags & OBJSTATE_IN_SEQ) == FALSE) {

        //If the player control is unlocked, assume there isn't an important sequence playing
        Player_Data* playerData = player->data;
        if (playerData && !(playerData->flags & 0x200000)) {
            rsCreditsFramesWithoutSequence += gUpdateRate;
            return;
        }
    }
    
    //Otherwise, assume an important sequence is playing
    if (rsCreditsFramesWithoutSequence) {
        rsCreditsFramesWithoutSequence = 0;
    }
}

/** Restore credits when exiting pause menu */
RECOMP_CALLBACK("*", recomp_on_game_tick_start) void restoreCredits() {
    u8 previousMenu = menu_get_previous();
    u8 currentMenu = menu_get_current();

    //Restore credits when unpausing
    if (
        ((previousMenu == MENU_PAUSE)) && 
        (rsCreditsRestoredFrame > 0) &&
        (currentMenu != MENU_CREDITS) 
    ) {
        menu_set(MENU_CREDITS);
        sTime = rsCreditsRestoredFrame;
    }

    //Track how long the credits have been playing outside of a sequence
    if ((rsCreditsRestoredFrame > 0) && (currentMenu != MENU_PAUSE)) {
        credits_count_frames_without_sequence();
    }

    // End the credits if they've been playing too long outside of a sequence (i.e. persisting into gameplay)
    // The credits do look really cool playing over gameplay, but they replace the cmdmenu while active!
    if (rsCreditsFramesWithoutSequence > 60) {
        credits_finish();
    }
}

/** Returns the current frame of the credits */
s32 credits_get_frame() {
    return (s32)sTime;
}

/** Allows the credits to be scrubbed to a particular frame */
void credits_set_frame(s32 frame) {   
    if (frame < 0) {
        return;
    }

    if (menu_get_current() != MENU_CREDITS) {
        menu_set(MENU_CREDITS);
    }

    rsCreditsRestoredFrame = frame;
}

/** Makes sure the credits are at least at a certain frame */
void credits_sync_frame(s32 frame) {
    if (frame < 0) {
        return;
    }

    if (menu_get_current() != MENU_CREDITS) {
        menu_set(MENU_CREDITS);
    }

    if (sTime < frame) {
        rsCreditsRestoredFrame = frame;
    }
}

/** End the credits and restore gameplay menus */
void credits_finish() {
    sTime = 0;
    rsCreditsRestoredFrame = 0;
    rsCreditsFramesWithoutSequence = 0;
    menu_set(MENU_GAMEPLAY);
}
