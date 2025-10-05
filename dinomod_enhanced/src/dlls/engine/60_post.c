#include "modding.h"
#include "recompconfig.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "PR/os.h"
#include "game/objects/object.h"
#include "sys/fonts.h"
#include "sys/gfx/gx.h"
#include "sys/gfx/texture.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "dll.h"
#include "functions.h"
#include "types.h"

#include "recomp/dlls/engine/60_post_recomp.h"

extern u8 data_0;
extern u8 data_4;
extern Texture *data_8;
extern Texture *data_C;
extern Texture *data_10;
extern u32 _data_14;
extern u32 data_18;

extern f32 bss_0;
extern s8 bss_4;
extern s8 bss_5;
extern s8 bss_6;
extern f32 bss_8;
extern f32 bss_C;
extern f32 bss_10;
extern GameTextChunk *bss_14;
extern Texture *bss_18;
extern Texture *bss_1C;

static char dinomod_enhanced_message[] = "(Dinomod Enhanced - 26th January 2025)";

// Repositions the N64 and text (originally by MusicalProgrammer)
RECOMP_PATCH void dll_60_draw(Gfx **gdl, Mtx **mtxs, Vertex **vtxs) {
    u32 local4;
    u8 _stackPad[4];
    f32 var5;
    s32 fontYSpacing;

    local4 = data_18;

    font_window_set_coords(2, 0, 0, 
        (RESOLUTION_WIDTH(get_some_resolution_encoded())) - 50,
        (RESOLUTION_HEIGHT(get_some_resolution_encoded())));
    
    font_window_flush_strings(2);
    font_window_use_font(2, FONT_FUN_FONT);
    func_80037A14(gdl, mtxs, 1);

    if (data_4 == 1) {
        fontYSpacing = font_get_y_spacing(FONT_FUN_FONT);

        font_window_set_text_colour(2, 183, 139, 97, 255, 255);
        font_window_add_string_xy(2, 320, 198,                bss_14->strings[0], 1, ALIGN_TOP_CENTER);
        font_window_add_string_xy(2, 320, 272,                bss_14->strings[1], 1, ALIGN_TOP_CENTER);
        font_window_add_string_xy(2, 320, fontYSpacing + 272, bss_14->strings[2], 1, ALIGN_TOP_CENTER);
        font_window_add_string_xy(2, 320, 356,                bss_14->strings[3], 1, ALIGN_TOP_CENTER);
        font_window_add_string_xy(2, 320, fontYSpacing + 356, bss_14->strings[4], 1, ALIGN_TOP_CENTER);
        func_8003825C(gdl, data_10, 0xfd, 0x42, 0, 0, 0xff, 0);
    } else {
        gDLL_76->vtbl->func2(gdl, mtxs);

        if (bss_0 < 240.0f) {
            font_window_enable_wordwrap(2);
            font_window_set_text_colour(2, 183, 139, 97, 255, 255);

            //@recomp: print mod date
            font_window_use_font(2, FONT_DINO_SUBTITLE_FONT_1);
            font_window_add_string_xy(2, RESOLUTION_WIDTH(get_some_resolution_encoded())/2, 30,  dinomod_enhanced_message, 1, ALIGN_TOP_CENTER);
            font_window_use_font(2, FONT_FUN_FONT);

            font_window_add_string_xy(2, 57, 54,  bss_14->strings[0], 1, ALIGN_TOP_LEFT);
            font_window_add_string_xy(2, 179, 88, bss_14->strings[1], 1, ALIGN_TOP_LEFT);
            font_window_add_string_xy(2, 57, 172, bss_14->strings[2], 1, ALIGN_TOP_LEFT);
            font_window_add_string_xy(2, 57, 222, bss_14->strings[3], 1, ALIGN_TOP_LEFT);
            font_window_add_string_xy(2, 57, 381, bss_14->strings[4], 1, ALIGN_TOP_LEFT);
            func_8003825C(gdl, bss_18, 0x3a, 0x65, 0, 0, 0xff, 0);
            func_8003825C(gdl, bss_1C, 0x16d, 0x68, 0, 0, 0xff, 0);
        }

        if (bss_0 > 240.0f && data_0 == 0) {
            data_0 = 1;
            gDLL_76->vtbl->func0(0x41a, 0x2c9, 0xf, -470, 0xffffffee, 1.0f, &local4, 0xe002, 5, 1, 0, 0);
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
            func_8003825C(gdl, data_C, 0x2B, 0xBE, 0, 0, (s16)(255.0f * var5), 0);
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
            func_8003825C(gdl, data_8, 0x2B, 0xBE, 0, 0, (u32)(255.0f * var5) & 0xFF, 0);
        }
    }

    font_window_draw(gdl, NULL, NULL, 2);
    bss_5 -= 1;
    if (bss_5 < 0) {
        bss_5 = 0;
    }
}
