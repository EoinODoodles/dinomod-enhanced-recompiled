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

#include "recomp/dlls/engine/61_rareware_recomp.h"

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
        func_8003825C(gdl, bss_10, 0x2A, 0xAF, 0, 0, (s16)(255.0f * var1), 0);
        func_8003825C(gdl, bss_18, 0x82, 0xD0, 0, 0, (s16)(255.0f * var1), 0);
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
        func_8003825C(gdl, bss_C, 0x2A, 0xAF, 0, 0, (u32)(255.0f * var1) & 0xFF, 0);
        func_8003825C(gdl, bss_14, 0x82, 0xD0, 0, 0, (u32)(255.0f * var1) & 0xFF, 0);
    }
}
