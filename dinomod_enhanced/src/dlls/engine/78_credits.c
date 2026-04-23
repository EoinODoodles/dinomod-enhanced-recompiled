#include "modding.h"
#include "recompconfig.h"
#include "dll_util.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "PR/os.h"
#include "sys/fonts.h"
#include "sys/gfx/texture.h"
#include "sys/rcp.h"
#include "sys/vi.h"
#include "types.h"
#include "dlls/engine/21_gametext.h"

#include "recomp/dlls/_asm/78_recomp.h"

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

/*0x0*/ extern CreditsGroup data_0[10];

/*0x0*/ extern Texture* bss_0;                  //Shown at the beginning of the credits
/*0x4*/ extern u8 bss_4;                        //The index of the current group of names
/*0x8*/ extern GameTextChunk* bss_8;            //Gametext file 0x1FD
/*0xC*/ extern f32 bss_C;                       //The current frame of the credits

#define MAX_OPACITY 0xFF

#define BASE_X_LEFT 19
#define BASE_X_RIGHT 299
#define BASE_Y 53

/**
  * Centres the Dinosaur Planet logo in widescreen mode
  * (not needed in recomp, only for ROM patches)
  */
RECOMP_PATCH void dll_78_func_570(Gfx** gdl, s32 arg1, s32 arg2) {
    CreditsLine* line;
    s32 align;
    s32 i;
    s32 x;
    s32 y;

    if (bss_4 == ARRAYCOUNT(data_0)) {
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
        
    if (bss_4 == 0) {
        //Draw the Dinosaur Planet logo
        line = &data_0[bss_4].lines[0];

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
        rcp_screen_full_write(gdl, bss_0, 45, 76, 0, 0, line->opacity, 0);
        #endif
    } else {
        //Draw the developer credits
        for (i = 0; i < data_0[bss_4].lineCount; i++) {
            line = &data_0[bss_4].lines[i];
            
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
            
            //Text
            font_window_set_extra_char_spacing(1, line->spacing);
            font_window_add_string_xy(1, x, y, bss_8->strings[line->textID], 1, align);
            
            //Text drop-shadow
            font_window_set_text_colour(1, 0, 0, 0, 0xFF, line->opacity);
            font_window_add_string_xy(1, x - 2, y - 2, bss_8->strings[line->textID], 2, align);
        }
    }
    
    font_window_draw(gdl, 0, 0, 1);
}
