#include "modding.h"

#include "PR/gbi.h"
#include "sys/camera.h"
#include "sys/map.h"
#include "sys/camera.h"

extern s16 gLetterboxSize;

#include "recomp/dlls/engine/28_screen_fade_recomp.h"

extern f32 sFadeAlpha;

RECOMP_PATCH void screen_fade_draw_simple_black(Gfx **gdl, Mtx **mtxs, Vertex **vtxs) {
    s32 ulx;
    s32 uly;
    s32 lrx;
    s32 lry;

    camViewportGetFullRect(&ulx, &uly, &lrx, &lry);

    // @recomp: Don't exclude letterboxing, otherwise the fade will be underneath the cmdmenu.
    //          Only include this for the black fade as anything else would look weird overlaid on top
    //          of the letterboxed region. Notably, this improves seq fadeouts that have the HUD or
    //          an energy bar drawn at the same time.
    uly -= gLetterboxSize;
    lry += gLetterboxSize;

    gDPSetScissor((*gdl)++, G_SC_NON_INTERLACE, ulx, uly, lrx, lry);

    gDPSetCombineMode(*gdl, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    dlApplyCombine(gdl);

    gDPSetOtherMode(*gdl,
                    G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP |
                        G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE,
                    G_AC_NONE | G_ZS_PIXEL | G_RM_CLD_SURF | G_RM_CLD_SURF2);
    dlApplyOtherMode(gdl);

    dlSetPrimColor(gdl, 0, 0, 0, (u8)sFadeAlpha);

    gDPFillRectangle((*gdl)++, ulx, uly, lrx, lry);

    gDLBuilder->needsPipeSync = TRUE;
    camApplyScissor(gdl);
}
