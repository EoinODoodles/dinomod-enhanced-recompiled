#include "game/objects/object.h"
#include "modding.h"

#include "dll.h"
#include "gbi_extra.h"
#include "PR/ultratypes.h"
#include "recomputils.h"
#include "sys/camera.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/vi.h"

#include "recomp/dlls/engine/7_newday_recomp.h"

//TEMPORARY DEFINES
#define newday_func_102C dll_7_func_102C
#define newday_func_56F8 dll_7_func_56F8
//END OF TEMPORARY DEFINES

typedef struct {
/*000*/ DLTri *unk0;
/*004*/ Vtx *unk4;
/*008*/ Texture *unk8;
/*00C*/ Texture *unkC;
/*010*/ Texture *unk10;
/*014*/ Texture *unk14;
/*018*/ Texture *unk18;
/*01C*/ Texture *unk1C;
/*020*/ s32 unk20;
/*024*/ u8 _unk20[8];
/*02C*/ f32 unk2C;
/*030*/ f32 unk30;
/*034*/ f32 unk34;
/*038*/ u8 _unk38[8];
/*040*/ f32 unk40;
/*044*/ f32 unk44[3][7]; // splines
/*098*/ u8 _unk98[0xc0 - 0x98];
/*0C0*/ f32 timeSeconds; //time of day (seconds)
/*0C4*/ f32 unkC4;
/*0C8*/ f32 unkC8;
/*0CC*/ s32 unkCC;
/*0D0*/ s32 unkD0;
/*0D4*/ s32 unkD4;
/*0D8*/ s32 unkD8;
/*0DC*/ s32 unkDC;
/*0E0*/ s32 unkE0;
/*0E4*/ s32 unkE4;
/*0E8*/ s32 unkE8;
/*0EC*/ s32 unkEC[8];
/*10C*/ s32 unk10C;
/*110*/ u8 unk110;
/*111*/ u8 unk111;
/*112*/ u8 unk112;
/*113*/ u8 unk113;
/*114*/ u8 unk114;
/*115*/ u8 _unk115[3];
} NewDayStruct;

extern s32 data_30;
extern s32 data_40;
extern s32 data_44;
extern s32 data_48;
extern s32 data_4C;
extern s32 data_50;
extern s32 data_54;
extern f32 data_5C; //time of day
extern Object* data_C0;
extern u8 data_C8;
extern u8 data_CC[][6];

extern NewDayStruct* bss_30[2];

extern s32 newday_func_56F8(Gfx** gdl);

/* Fix an uncommon crash that can occur when warping, due to a missing NULL check */
RECOMP_PATCH void newday_func_102C(Gfx** gdl, Mtx** arg1) {
    /*0x1C0*/ static s16 data_1C0 = 0;
    /*0x1C4*/ static s16 data_1C4 = 0;
    f32 sp64;
    s32 temp_v0_8;
    f32 sp5C;
    f32 sp58;
    f32 var_fv0;
    f32 sp50;
    s32 temp_t8;
    s32 temp_t6;
    s32 sp44;
    Camera* sp40;
    Texture* sp3C;
    Texture* sp38;
    f32 fov;
    f32 sp30;
    f32 fa1;

    sp44 = 0;
    texRenderReset();
    var_fv0 = data_5C / 86400.0f;
    if (var_fv0 < 0.0f) {
        var_fv0 = 0.0f;
    }
    if (var_fv0 > 1.0f) {
        var_fv0 = 1.0f;
    }
    sp30 = 0.0f;
    if ((var_fv0 >= 0.0f) && (var_fv0 < (1.0f/8.0f))) {
        sp30 = var_fv0 / (1.0f/8.0f);
    } else if ((var_fv0 >= (1.0f/8.0f)) && (var_fv0 < (2.0f/8.0f))) {
        sp44 = 1;
        sp30 = (var_fv0 - (1.0f/8.0f)) / (1.0f/8.0f);
    } else if ((var_fv0 >= (2.0f/8.0f)) && (var_fv0 < (3.0f/8.0f))) {
        sp44 = 2;
        sp30 = (var_fv0 - (2.0f/8.0f)) / (1.0f/8.0f);
    } else if ((var_fv0 >= (3.0f/8.0f)) && (var_fv0 < (4.0f/8.0f))) {
        sp44 = 3;
        sp30 = (var_fv0 - (3.0f/8.0f)) / (1.0f/8.0f);
    } else if ((var_fv0 >= (4.0f/8.0f)) && (var_fv0 < (5.0f/8.0f))) {
        sp44 = 4;
        sp30 = (var_fv0 - (4.0f/8.0f)) / (1.0f/8.0f);
    } else if ((var_fv0 >= (5.0f/8.0f)) && (var_fv0 < (6.0f/8.0f))) {
        sp44 = 5;
        sp30 = (var_fv0 - (5.0f/8.0f)) / (1.0f/8.0f);
    } else if ((var_fv0 >= (6.0f/8.0f)) && (var_fv0 < (7.0f/8.0f))) {
        sp44 = 6;
        sp30 = (var_fv0 - (6.0f/8.0f)) / (1.0f/8.0f);
    } else if ((var_fv0 >= (7.0f/8.0f)) && (var_fv0 <= (8.0f/8.0f))) {
        sp44 = 7;
        sp30 = (var_fv0 - (7.0f/8.0f)) / (1.0f/8.0f);
    }

    if ((bss_30[0] != NULL) && (sp44 != data_30) && (data_C8 == 0)) {
        bss_30[0]->unk10C = sp44;
        if (bss_30[0]->unk114 != 0) {
            bss_30[0]->unk18 = bss_30[0]->unk1C;
            bss_30[0]->unk18->refCount += 1;
            data_1C0 = data_1C4;
        } else {
            if (bss_30[0]->unk18 != NULL) {
                texFreeTexture(bss_30[0]->unk18);
            }
            bss_30[0]->unk18 = NULL;
        }
        if (bss_30[0]->unk1C != NULL) {
            texFreeTexture(bss_30[0]->unk1C);
        }
        bss_30[0]->unk1C = NULL;
        if (bss_30[0]->unk10C == 0) {
            if (bss_30[0]->unk114 != 1) {
                bss_30[0]->unk18 = texLoadTexture(bss_30[0]->unkEC[0]);
                data_1C0 = bss_30[0]->unkEC[0] - 0x210;
            }
            bss_30[0]->unk1C = texLoadTexture(bss_30[0]->unkEC[1]);
            data_1C4 = bss_30[0]->unkEC[1] - 0x210;
        } else if (bss_30[0]->unk10C == 1) {
            if (bss_30[0]->unk114 != 1) {
                bss_30[0]->unk18 = texLoadTexture(bss_30[0]->unkEC[1]);
                data_1C0 = bss_30[0]->unkEC[1] - 0x210;
            }
            bss_30[0]->unk1C = texLoadTexture(bss_30[0]->unkEC[2]);
            data_1C4 = bss_30[0]->unkEC[2] - 0x210;
        } else if (bss_30[0]->unk10C == 2) {
            if (bss_30[0]->unk114 != 1) {
                bss_30[0]->unk18 = texLoadTexture(bss_30[0]->unkEC[2]);
                data_1C0 = bss_30[0]->unkEC[2] - 0x210;
            }
            bss_30[0]->unk1C = texLoadTexture(bss_30[0]->unkEC[3]);
            data_1C4 = bss_30[0]->unkEC[3] - 0x210;
        } else if (bss_30[0]->unk10C == 3) {
            if (bss_30[0]->unk114 != 1) {
                bss_30[0]->unk18 = texLoadTexture(bss_30[0]->unkEC[3]);
                data_1C0 = bss_30[0]->unkEC[3] - 0x210;
            }
            bss_30[0]->unk1C = texLoadTexture(bss_30[0]->unkEC[4]);
            data_1C4 = bss_30[0]->unkEC[4] - 0x210;
        } else if (bss_30[0]->unk10C == 4) {
            if (bss_30[0]->unk114 != 1) {
                bss_30[0]->unk18 = texLoadTexture(bss_30[0]->unkEC[4]);
                data_1C0 = bss_30[0]->unkEC[4] - 0x210;
            }
            bss_30[0]->unk1C = texLoadTexture(bss_30[0]->unkEC[5]);
            data_1C4 = bss_30[0]->unkEC[5] - 0x210;
        } else if (bss_30[0]->unk10C == 5) {
            if (bss_30[0]->unk114 != 1) {
                bss_30[0]->unk18 = texLoadTexture(bss_30[0]->unkEC[5]);
                data_1C0 = bss_30[0]->unkEC[5] - 0x210;
            }
            bss_30[0]->unk1C = texLoadTexture(bss_30[0]->unkEC[6]);
            data_1C4 = bss_30[0]->unkEC[6] - 0x210;
        } else if (bss_30[0]->unk10C == 6) {
            if (bss_30[0]->unk114 != 1) {
                bss_30[0]->unk18 = texLoadTexture(bss_30[0]->unkEC[6]);
                data_1C0 = bss_30[0]->unkEC[6] - 0x210;
            }
            bss_30[0]->unk1C = texLoadTexture(bss_30[0]->unkEC[7]);
            data_1C4 = bss_30[0]->unkEC[7] - 0x210;
        } else {
            if (bss_30[0]->unk114 != 1) {
                bss_30[0]->unk18 = texLoadTexture(bss_30[0]->unkEC[7]);
                data_1C0 = bss_30[0]->unkEC[7] - 0x210;
            }
            bss_30[0]->unk1C = texLoadTexture(bss_30[0]->unkEC[0]);
            data_1C4 = bss_30[0]->unkEC[0] - 0x210;
        }
        if (bss_30[0]->unk114 != 0) {
            bss_30[0]->unk114--;
        }
    } else if (data_C8 != 0) {
        if (bss_30[0] != NULL) { //@recomp: add null check
            if (bss_30[0]->unk1C != NULL) {
                texFreeTexture(bss_30[0]->unk1C);
            }
            if (bss_30[0]->unk18 != NULL) {
                texFreeTexture(bss_30[0]->unk18);
            }
            if (bss_30[0]->unk10C < 7) {
                bss_30[0]->unk18 = texLoadTexture(bss_30[0]->unkEC[bss_30[0]->unk10C - 1]);
                bss_30[0]->unk1C = texLoadTexture(bss_30[0]->unkEC[bss_30[0]->unk10C]);
                data_1C0 = bss_30[0]->unkEC[bss_30[0]->unk10C - 1] - 0x210;
                data_1C4 = bss_30[0]->unkEC[bss_30[0]->unk10C] - 0x210;
            } else {
                bss_30[0]->unk18 = texLoadTexture(bss_30[0]->unkEC[0]);
                bss_30[0]->unk1C = texLoadTexture(bss_30[0]->unkEC[7]);
                data_1C0 = bss_30[0]->unkEC[7] - 0x210;
                data_1C4 = bss_30[0]->unkEC[0] - 0x210;
            }
        }
    }

    data_30 = sp44;
    if (sp30 > 1.0f) {
        sp30 = 1.0f;
    }
    if (sp30 < 0.0f) {
        sp30 = 0.0f;
    }
    data_4C = data_CC[data_1C0][0] + ((f32) (data_CC[data_1C4][0] - data_CC[data_1C0][0]) * sp30);
    data_50 = data_CC[data_1C0][1] + ((f32) (data_CC[data_1C4][1] - data_CC[data_1C0][1]) * sp30);
    data_54 = data_CC[data_1C0][2] + ((f32) (data_CC[data_1C4][2] - data_CC[data_1C0][2]) * sp30);
    data_40 = data_CC[data_1C0][3] + ((f32) (data_CC[data_1C4][3] - data_CC[data_1C0][3]) * sp30);
    data_44 = data_CC[data_1C0][4] + ((f32) (data_CC[data_1C4][4] - data_CC[data_1C0][4]) * sp30);
    data_48 = data_CC[data_1C0][5] + ((f32) (data_CC[data_1C4][5] - data_CC[data_1C0][5]) * sp30);
    if (bss_30[0] == NULL) {
        sp3C = texLoadTexture(0x210);
        sp38 = texLoadTexture(0x210);
    } else {
        sp3C = bss_30[0]->unk18;
        sp38 = bss_30[0]->unk1C;
    }
    sp40 = camGet();
    fov = camGetFOV();

    sp5C = (((sp50 = (f32) (sp3C->height | ((sp3C->widthHeightHi & 0xF) << 8))) * (fov * 0.5f)) / 180.0f) * 3.0f;
    sp58 = mathCosfInterp((s16) (-sp40->roll)) * sp5C;
    fa1 = (((sp50 * 0.5f) - 6.0f) - ((3.0f * (sp50 * (f32) sp40->pitch)) / 32768.0f));
    sp64 = (fa1 + sp58) * 32.0f;
    gSPLoadGeometryMode(*gdl, 0);
    dlApplyGeometryMode(gdl);
    gDPLoadTextureBlockS((*gdl)++, 
        sp3C + 1, 
        G_IM_FMT_RGBA, 
        G_IM_SIZ_16b, 
        4, 
        256, 
        0, 
        G_TX_NOMIRROR | G_TX_CLAMP, 
        G_TX_NOMIRROR | G_TX_CLAMP, 
        G_TX_NOMASK, G_TX_NOMASK, 
        G_TX_NOLOD, G_TX_NOLOD);
    gDPLoadMultiBlockS((*gdl)++, 
        sp38 + 1, 
        (s16) sp38->sizeBytes >> 3, 
        1, 
        G_IM_FMT_RGBA, 
        G_IM_SIZ_16b, 
        4, 
        256, 
        0, 
        G_TX_NOMIRROR | G_TX_CLAMP, 
        G_TX_NOMIRROR | G_TX_CLAMP, 
        G_TX_NOMASK, G_TX_NOMASK, 
        G_TX_NOLOD, G_TX_NOLOD);
    if (data_C0 != NULL) {
        if (newday_func_56F8(gdl) == 0) {
            gDLL_8_newfog->vtbl->func5(gdl);
        }
    } else {
        gDLL_8_newfog->vtbl->func5(gdl);
    }

    dlSetEnvColor(gdl, 0xFF, 0xFF, 0xFF, (u8) (s16) (255.0f * sp30));
    gDPSetCombineLERP(*gdl, 
        TEXEL1, TEXEL0, ENV_ALPHA, TEXEL0, TEXEL1, TEXEL0, ENVIRONMENT, TEXEL0, 
        PRIMITIVE, COMBINED, PRIMITIVE_ALPHA, COMBINED, COMBINED, 0, PRIMITIVE, 0);
    dlApplyCombine(gdl);
    gDPSetOtherMode(*gdl, 
        G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_2CYCLE | G_PM_NPRIMITIVE, 
        G_AC_NONE | G_ZS_PIXEL | G_RM_OPA_SURF | G_RM_OPA_SURF2);
    dlApplyOtherMode(gdl);
    temp_v0_8 = viGetCurrentSize();
    temp_t6 = (temp_v0_8 >> 0x10) & 0xFFFF;
    temp_t8 = (temp_v0_8 & 0xFFFF);
    
    //@recomp: handle division by zero
    if (temp_t6 >> 1 != 0) {
        sp58 /= (temp_t6 >> 1);
    }

    sp58 *= 1024.0f;
    gSPTextureRectangle((*gdl)++, 
        0, 
        0, 
        temp_t8 << 2, 
        temp_t6 << 2, 
        G_TX_RENDERTILE, 
        0, 
        (s32)sp64, 
        qs510(1), 
        -(s32)sp58);
    gDLBuilder->needsPipeSync = 1;
    if (bss_30[0] == NULL) {
        if (sp3C != NULL) {
            texFreeTexture(sp3C);
        }
        if (sp38 != NULL) {
            texFreeTexture(sp38);
        }
    }
    texRenderReset();
}
