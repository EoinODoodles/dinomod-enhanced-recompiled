#include "configs.h"
#include "macros.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "dll.h"
#include "game/gamebits.h"
#include "sys/fonts.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "types.h"

extern s32 D_800A7D6C;
extern f32 D_800A7D70;
extern f32 D_800A7D74; // UI countdown time (in frames)
extern s8 D_800A7D79;
extern u32 D_800A7D7C;
extern s8 D_800A7D78;
extern char D_800A7D98[8];
extern char D_800A7DA0[8];

static u32 countChars(char* str, u32 strBufferSize) {
    u32 i;
    
    if (str == NULL) {
        return 0;
    }

    for (i = 0; i < strBufferSize; i++) {
        if (str[i] == '\0') {
            return i;
        }
    }

    return strBufferSize;
}

static s32 getPositionTweak(u32 value) {
    s8 shift = 0;
    u8 tens = (value / 10) % 10;
    u8 ones = value % 10;

    switch (tens) {
    case 4:
        shift += -1;
        break;
    }

    return shift;
}

static s32 getPositionTweakBesideFractions(u32 value) {
    s8 shift = 0;
    u8 tens = (value / 10) % 10;
    u8 ones = value % 10;

    switch (tens) {
    case 1:
    case 3:
    case 5:
    case 6:
        shift += 1;
        break;
    }

    return shift;
}

/* Centre the timer text in the UI box, and add an option to show fractions of a second
   (the decimals are printed in Dec 2000, but they're drawn outside the box and end up invisible!) */
RECOMP_PATCH void menu_func_8000FB2C(Gfx **gdl) {
    s32 _stackPad[2];
    f32 delay;
    s32 timerRanOut;
    /* RECOMP */
    _Bool timerTextShowFractions = configs_GetMenuTimerFractionConfig();
    u16 integersStableWidth = 0;
    u16 textFullWidth = 0;
    s8 integersShift = 0;
    char lastFractionalDigit[2] = { 0, 0 };

    delay = gUpdateRateF;
    if (D_800A7D6C != 0 || mainGetPauseState() != 0) {
        delay = 0.0f;
    }

    timerRanOut = FALSE;
    if (D_800A7D94 == 1) {
        D_800A7D70 -= delay;

        if (D_800A7D70 <= 0.0f) {
            timerRanOut = TRUE;
            D_800A7D70 = 0.0f;
        }
    } else {
        if (0) {}

        D_800A7D70 += delay;

        if (D_800A7D70 > D_800A7D74) {
            timerRanOut = TRUE;
            D_800A7D70 = D_800A7D74;
        }
    }

    if (timerRanOut) {
        if (D_800A7D79 & 8) {
            dll_amSfx->Play(0, SOUND_242_Failure_Glissando, MAX_VOLUME, 0, 0, 0, 0);
        }

        if (D_800A7D7C != 0) {
            dll_amSfx->Stop(D_800A7D7C);
            D_800A7D7C = 0;
        }

        D_800A7D94 = 0;
        D_800A7D78 = 1;
        D_800A7D79 = 0;
    }

    if (D_800A7D7C != 0) {
        if ((D_800A7D79 & 1)) {
            dll_amSfx->SetPitch(D_800A7D7C, 1.3f - (D_800A7D70 / D_800A7D74) * 0.6f);
            dll_amSfx->SetVol(D_800A7D7C, (127 - (u8)((D_800A7D70 / D_800A7D74) * 80.0f)));
        } else {
            dll_amSfx->SetPitch(D_800A7D7C, (D_800A7D70 / D_800A7D74) * 0.6f + 0.69999999f);
            dll_amSfx->SetVol(D_800A7D7C, ((u8)((D_800A7D70 / D_800A7D74) * 80.0f)) + 47);
        }
    }

    if (D_800A7D79 & 0x10) {
        #define CENTRE_SCREEN 160
        #define TOP_MARGIN 25
        #define BOX_WIDTH 60
        #define BOX_HEIGHT 25
        #define FONT_CHAR_WIDTH 8       //On average
        #define FONT_CHAR_HEIGHT 10     //On average
        #define TEXT_Y ((BOX_HEIGHT/2) - (FONT_CHAR_HEIGHT/2) + 1)

        fontWindowSetCoords(2, CENTRE_SCREEN - BOX_WIDTH/2, TOP_MARGIN, CENTRE_SCREEN + BOX_WIDTH/2, TOP_MARGIN + BOX_HEIGHT);
        fontWindowUseFont(2, FONT_DINO_SUBTITLE_FONT_1);
        fontWindowSetBgColour(2, 0, 0, 0, 128);
        fontWindowFlushStrings(2);
    
        if (timerTextShowFractions) {
            integersShift = getPositionTweakBesideFractions((int)D_800A7D70 / 60);

            sprintf(D_800A7D98, "%d", (int)D_800A7D70 / 60);
            integersStableWidth = (countChars(D_800A7D98, ARRAYCOUNT(D_800A7D98)) * FONT_CHAR_WIDTH) - 1;

            textFullWidth = integersStableWidth + 17;
        } else {
            integersShift = getPositionTweak((int)D_800A7D70 / 60);

            //@recomp: get the text width for the current 10 second grouping (so the text doesn't vibrate around from second to second based on character width)
            sprintf(D_800A7D98, "%d", ((int)D_800A7D70 / 60) - (((int)D_800A7D70 / 60) % 10));
            integersStableWidth = fontGetTextWidth(1, D_800A7D98, 0, FONT_DINO_SUBTITLE_FONT_1);
            sprintf(D_800A7D98, "%d", (int)D_800A7D70 / 60);

            textFullWidth = integersStableWidth;
        }

        sprintf(D_800A7D98, "%d", (int)D_800A7D70 / 60);
        fontWindowAddStringXY(2, (BOX_WIDTH/2) - 1 - (textFullWidth/2) + integersShift, TEXT_Y, D_800A7D98, 1, ALIGN_TOP_LEFT);
        
        if (timerTextShowFractions) {
            //@recomp: split the fractional digits, so the last digit doesn't vibrate around based on its character width
            sprintf(D_800A7DA0, ".%1d", ((int)D_800A7D70 % 60)/10);
            fontWindowAddStringXY(2, (BOX_WIDTH/2) - 1 - (textFullWidth/2) + integersStableWidth, TEXT_Y, D_800A7DA0, 1, ALIGN_TOP_LEFT);
            
            sprintf(lastFractionalDigit, "%d", ((int)D_800A7D70 % 10));
            fontWindowAddStringXY(2, (BOX_WIDTH/2) - 1 - (textFullWidth/2) + integersStableWidth + (2*FONT_CHAR_WIDTH) - 1, TEXT_Y, lastFractionalDigit, 1, ALIGN_TOP_CENTER);
        }
        
        fontWindowDraw(gdl, NULL, NULL, 2);
    }
}
