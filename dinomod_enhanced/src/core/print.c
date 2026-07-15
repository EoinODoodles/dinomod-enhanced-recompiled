#include "PR/ultratypes.h"
#include "dll.h"
#include "modding.h"

#include "sys/print.h"

#include "core/print.h"

/* -------- .bss start 800be1d0 -------- */
extern char gDebugPrintBufferStart[0x900];
extern Texture *gDiTextures[3];
extern u16 D_800BEADC;
extern u16 D_800BEADE;
extern u16 D_800BEAE0;
extern u16 D_800BEAE2;
extern s32 D_800BEAE4;
extern s32 D_800BEAE8;
extern s32 D_800BEAEC;
extern s32 D_800BEAF0;
extern s32 D_800BEAF4;
extern s32 D_800BEAF8;
extern u32 D_800BEAFC;
extern u16 D_800BEB00; // gDebugScreenWidth?
extern u16 D_800BEB02; // gDebugScreenHeight?
extern u8 D_800BEB04;
extern u8 D_800BEB05;
extern u8 D_800BEB06;
extern u8 D_800BEB07;
/* -------- .bss end 800beb10 -------- */

extern u8 D_800931AC;
extern u8 D_800931B0;
extern u8 D_800931B4;
extern u8 D_800931B8;
extern u16 D_800930E4;

extern void diPrintfUpdateBounds();
extern void diPrintfOrigin();
extern s32 diPrintf_func_80061210(Gfx **gdl, char *buffer);
extern void diPrintfRenderBackground(Gfx **gdl, u32 ulx, u32 uly, u32 lrx, u32 lry);
extern s32 diPrintfRenderChar(Gfx**gdl, s32 asciiVal);

static _Bool rsFixedWidthAll = FALSE;

/* Allows diPrintf to use fixed width mode for all characters */
void de_print_set_fixedwidth_all(_Bool enable) {
    rsFixedWidthAll = (enable != 0);
}

/* Check whether diPrintf is using fixed width mode for all characters */
_Bool de_print_get_fixedwidth_all(void) {
    return rsFixedWidthAll;
}

/* Add an option to use fixed-width mode with all characters */
RECOMP_PATCH s32 diPrintf_func_80061210(Gfx** gdl, char* buffer) {
    char* bufferCopy;
    s32 temp_hi;
    s32 xOffset;
    s32 v0;
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
    u8 bufferValue;

    bufferCopy = buffer;
    while ((bufferValue = *buffer++) != '\0') {
        xOffset = 0;
        switch (bufferValue) {
        case 0x83: // Leave fixed-width mode
            D_800BEAE4 = 0;
            break;
        case 0x84: // Enter fixed-width mode
            D_800BEAE4 = 1;
            break;
        case 0x81: // Set the text color from the next 4 bytes
            red = buffer[0];
            green = buffer[1];
            blue = buffer[2];
            alpha = buffer[3];
            buffer += 4;
            if (D_800BEAE8) {
                dl_set_env_color(gdl, red, green, blue, alpha);
            }
            break;
        case 0x87:
            D_800931B4 = buffer[0];
            D_800931B8 = buffer[1];
            buffer += 2;
            if (D_800931B4 || D_800931B8) {
                gDPSetOtherMode(
                    *gdl,
                    G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_POINT | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE | 0x1,
                    G_AC_NONE | G_ZS_PIXEL | G_RM_XLU_SURF | G_RM_XLU_SURF2
                );
                dl_apply_other_mode(gdl);
            } else {
                gDPSetOtherMode(
                    *gdl,
                    G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE,
                    G_AC_NONE | G_ZS_PIXEL | G_RM_XLU_SURF | G_RM_XLU_SURF2
                );
                dl_apply_other_mode(gdl);
            }
            break;
        case 0x85: // Set the background color from the next 4 bytes
            red = buffer[0];
            green = buffer[1];
            blue = buffer[2];
            alpha = buffer[3];
            buffer += 4;
            if (!D_800BEAE8) {
                D_800BEB04 = red;
                D_800BEB05 = green;
                D_800BEB06 = blue;
                D_800BEB07 = alpha;
                dl_set_prim_color(gdl, red, green, blue, alpha);
            }
            break;
        case 0x82: // Set debug text position from the next 4 bytes
            if (!D_800BEAE8) {
                diPrintfRenderBackground(gdl, D_800BEAE0, D_800BEAE2, D_800BEADC, D_800BEADE + 10);
            }
            D_800BEADC = buffer[0];
            D_800BEADC |= buffer[1] << 8;
            D_800BEADE = buffer[2];
            D_800BEADE |= buffer[3] << 8;
            D_800BEAE0 = D_800BEADC;
            D_800BEAE2 = D_800BEADE;
            buffer += 4;
            break;
        case 0x86:
            D_800930E4 = buffer[0];
            D_800930E4 |= buffer[1] << 8;
            buffer += 2;
            break;
        case ' ': // Space
            xOffset = 6;
            break;
        case '\n': // Line Feed
            if (!D_800BEAE8) {
                diPrintfRenderBackground(gdl, D_800BEAE0, D_800BEAE2, D_800BEADC, D_800BEADE + 10);
            }
            diPrintfNewline();
            D_800BEAE0 = D_800BEADC;
            D_800BEAE2 = D_800BEADE;
            break;
        case '\t': // HT - Horizontal Tab
            temp_hi = D_800BEADC % D_800930E4;
            if (temp_hi == 0) {
                xOffset = D_800930E4;
            } else {
                xOffset = D_800930E4 - temp_hi;
            }
            break;
        default:
            xOffset = diPrintfRenderChar(gdl, bufferValue);
        }

        if ((rsFixedWidthAll && bufferValue != '\n') || //@recomp
            (D_800BEAE4 != 0 && bufferValue >= 0x20 && bufferValue < 0x80)
        ) {
            xOffset = 7;
        }

        D_800BEADC += xOffset;
        if ((D_800BEB00 - 16) < D_800BEADC) {
            if (!D_800BEAE8) {
                diPrintfRenderBackground(gdl, D_800BEAE0, D_800BEAE2, D_800BEADC, D_800BEADE + 10);
            }
            diPrintfNewline();
            D_800BEAE0 = D_800BEADC;
            D_800BEAE2 = D_800BEADE;
        }
    }

    return buffer - bufferCopy;
}
