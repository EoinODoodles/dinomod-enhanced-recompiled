#include "modding.h"
#include "recompconfig.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "PR/os.h"
#include "game/objects/object.h"
#include "sys/fonts.h"
#include "sys/gfx/texture.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "sys/rcp.h"
#include "dll.h"
#include "types.h"

#include "recomp/dlls/engine/60_post_recomp.h"

extern u8 data_0;
extern u8 dExpansionPakMissing;
extern Texture *dTexN64Logo;
extern Texture *dTexN64LogoShadow;
extern Texture *dTexExpansionPak;

extern f32 bss_0;
extern s8 bss_4;
extern s8 bss_5;
extern s8 bss_6;
extern f32 bss_8;
extern f32 bss_C;
extern f32 bss_10;
extern GameTextChunk *splashGametext;
extern Texture *sTexDolbyBig;
extern Texture *sTexDolbySmall;

static char dinomod_enhanced_message[] = "(Dinomod Enhanced: Recompiled - v0.8.0)";

// Repositions the N64 and text (originally by MusicalProgrammer)
RECOMP_PATCH void dll_60_draw(Gfx **gdl, Mtx **mtxs, Vertex **vtxs) {
    u8 dColourTint[4] = {0xe4, 0x9c, 0x44, 0xff}; //Colour multiplier for the greyscale background
    u8 _stackPad[4];
    f32 var5;
    s32 fontYSpacing;

    font_window_set_coords(2, 0, 0, 
        (GET_VIDEO_WIDTH(vi_get_current_size())) - 50,
        (GET_VIDEO_HEIGHT(vi_get_current_size())));
    
    font_window_flush_strings(2);
    font_window_use_font(2, FONT_FUN_FONT);
    rcp_clear_screen(gdl, mtxs, 1);

    if (dExpansionPakMissing == 1) {
        fontYSpacing = font_get_y_spacing(FONT_FUN_FONT);

        font_window_set_text_colour(2, 183, 139, 97, 255, 255);
        font_window_add_string_xy(2, 320, 198,                splashGametext->strings[0], 1, ALIGN_TOP_CENTER);
        font_window_add_string_xy(2, 320, 272,                splashGametext->strings[1], 1, ALIGN_TOP_CENTER);
        font_window_add_string_xy(2, 320, fontYSpacing + 272, splashGametext->strings[2], 1, ALIGN_TOP_CENTER);
        font_window_add_string_xy(2, 320, 356,                splashGametext->strings[3], 1, ALIGN_TOP_CENTER);
        font_window_add_string_xy(2, 320, fontYSpacing + 356, splashGametext->strings[4], 1, ALIGN_TOP_CENTER);
        rcp_screen_full_write(gdl, dTexExpansionPak, 0xfd, 0x42, 0, 0, 0xff, 0);
    } else {
        gDLL_76->vtbl->func2(gdl, mtxs);

        if (bss_0 < 240.0f) {
            font_window_enable_wordwrap(2);
            font_window_set_text_colour(2, 183, 139, 97, 255, 255);

            //@recomp: print mod date
            font_window_use_font(2, FONT_DINO_SUBTITLE_FONT_1);
            font_window_add_string_xy(2, GET_VIDEO_WIDTH(vi_get_current_size())/2, 30,  dinomod_enhanced_message, 1, ALIGN_TOP_CENTER);
            font_window_use_font(2, FONT_FUN_FONT);

            font_window_add_string_xy(2, 57, 54,  splashGametext->strings[0], 1, ALIGN_TOP_LEFT);
            font_window_add_string_xy(2, 179, 88, splashGametext->strings[1], 1, ALIGN_TOP_LEFT);
            font_window_add_string_xy(2, 57, 172, splashGametext->strings[2], 1, ALIGN_TOP_LEFT);
            font_window_add_string_xy(2, 57, 222, splashGametext->strings[3], 1, ALIGN_TOP_LEFT);
            font_window_add_string_xy(2, 57, 381, splashGametext->strings[4], 1, ALIGN_TOP_LEFT);
            rcp_screen_full_write(gdl, sTexDolbyBig, 0x3a, 0x65, 0, 0, 0xff, 0);
            rcp_screen_full_write(gdl, sTexDolbySmall, 0x16d, 0x68, 0, 0, 0xff, 0);
        }

        if (bss_0 > 240.0f && data_0 == 0) {
            data_0 = 1;
            gDLL_76->vtbl->func0(0x41a, 0x2c9, 0xf, -470, 0xffffffee, 1.0f, (u32*)&dColourTint, 0xe002, 5, 1, 0, 0);
        }

        if (bss_0 > 280.0f && data_0 == 1) {
            data_0 = 3;
            bss_8 = 425.0f;
        }
        
        if (bss_0 > 450.0f && data_0 == 3) {
            data_0 = 4;
            bss_10 = 200.0f;
        }
        
        if (data_0 > 3) {
            if (bss_10 <= 100.0f) {
                var5 = 1.0f - ((100.0f - bss_10) / 100.0f);
            } else {
                var5 = 1.0f - ((bss_10 - 100.0f) / 100.0f);
            }

            if (var5 < 0.0f) {
                var5 = 0.0f;
            } else if (var5 > 1.0f) {
                var5 = 1.0f;
            }
            
            //@recomp: reposition logo
            rcp_screen_full_write(gdl, dTexN64LogoShadow, 0x2B, 0xBE, 0, 0, (s16)(255.0f * var5), 0);
        }

        if (data_0 >= 2) {
            if (bss_8 <= 106.0f) {
                var5 = 1.0f - ((106.0f - bss_8) / 106.0f);
            } else {
                var5 = 1.0f - ((bss_8 - 319.0f) / 106.0f);
            }

            if (var5 < 0.0f) {
                var5 = 0.0f;
            } else if (var5 > 1.0f) {
                var5 = 1.0f;
            }

            //@recomp: reposition logo
            rcp_screen_full_write(gdl, dTexN64Logo, 0x2B, 0xBE, 0, 0, (u32)(255.0f * var5) & 0xFF, 0);
        }
    }

    font_window_draw(gdl, NULL, NULL, 2);
    bss_5 -= 1;
    if (bss_5 < 0) {
        bss_5 = 0;
    }
}
