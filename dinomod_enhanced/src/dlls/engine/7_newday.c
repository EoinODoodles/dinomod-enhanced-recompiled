#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "game/objects/object.h"
#include "sys/camera.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/memory.h"
#include "sys/rand.h"
#include "sys/vi.h"
#include "dll.h"
#include "gbi_extra.h"

// @recomp: Track the last envfx action index so we can use it in debug messages
static u16 recomp_lastEnvfxActionIndex = -1;

RECOMP_HOOK("envfxAction") void newday_envfxAction_hook(Object *calledBy, Object *target, u16 actionIndex, s32 arg3) {
    recomp_lastEnvfxActionIndex = actionIndex;
}
RECOMP_HOOK("envfxRestoreAction") void newday_envfxRestoreAction_hook(Object *calledBy, Object *target, u16 actionIndex, s32 arg3) {
    recomp_lastEnvfxActionIndex = actionIndex;
}

#include "recomp/dlls/engine/7_newday_recomp.h"

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

extern s32 data_4;
extern f32 data_8;
extern f32 data_C;
extern s32 data_10;
extern s32 data_30;
extern s32 data_40;
extern s32 data_44;
extern s32 data_48;
extern s32 data_4C;
extern s32 data_50;
extern s32 data_54;
extern f32 data_5C; //time of day
extern s32 data_60;
extern f32 data_6C[][7];
extern Object* data_C0;
extern u8 data_C8;
extern u8 data_CC[][6];

extern NewDayStruct* bss_30[2];

extern s32 newday_func_56F8(Gfx** gdl);
extern void newday_func_2464(EnvFxAction*, f32, f32, f32);

RECOMP_PATCH void newday_func_18(Object* arg0, Object* arg1, EnvFxAction* arg2, s32 arg3) {
    PlayerEnvActions* temp_v0;
    s32 i;

    temp_v0 = gDLL_29_Gplay->vtbl->get_current_player_envactions();
    // OSReport("doDayAction\n"); // default.dol
    // @recomp: Don't accept actions with an index of 1, newday can't handle this in the current build and letting the
    //          the action be handled can result in the newday struct being allocated twice without freeing the previous.
    //          There are currently no actions in ENVFXACT.bin that have a non-zero index.
    if (arg2->unk26 != 0) {
        recomp_eprintf("[newday] ENVFX ACTION %d HAS OUT OF BOUNDS NEWDAY IDX: %d\n", recomp_lastEnvfxActionIndex, arg2->unk26);
    }
    if ((arg2 != NULL) && (arg2->unk26 == 0 /*< 2*/)) {
        // @bug: if arg2->unk26 is 1, bss_30[0] will be allocated twice without the previous value being freed
        if (bss_30[arg2->unk26] == NULL) {
            if (!(arg2->unk58 & 0x20) && (arg2->unk58 & 2)) {
                // @recomp: Don't set or accept the flag that starts a time transition. We have nothing to transition
                //          from in this case, so the transition func will use uninitialized values that can result in
                //          it getting stuck a loop, making the day/night cycle go super fast until another action is set.
                //          No existing action has this flag set in ENVFXACT.bin but clearing the flag is good to do anyway.
                if (arg2->unk58 & 0x40) {
                    recomp_eprintf("[newday] ENVFX ACTION %d HAS TRANSITION FLAG BUT WE HAVE NO STRUCT ALLOCATED!\n", recomp_lastEnvfxActionIndex);
                }
                arg2->unk58 &= ~0x40;
                //arg2->unk58 |= 0x40;
                if (arg0 != NULL) {
                    newday_func_2464(arg2, arg0->srt.transl.x, arg0->srt.transl.y, arg0->srt.transl.z);
                } else {
                    newday_func_2464(arg2, 0.0f, 0.0f, 0.0f);
                }
            }
        } else if (bss_30[arg2->unk26] != NULL) {
            if (arg2->unk58 & 2) {
                bss_30[arg2->unk26]->unk112 |= 0x40;
                data_4 = 1;
                if (arg2->unk58 & 1) {
                    data_8 = bss_30[arg2->unk26]->timeSeconds;
                    bss_30[0]->unk112 |= 1;
                } else {
                    data_8 = (f32) ((arg2->unk50 * 0x3C) + (arg2->unk4E * 0xE10));
                }
                if (arg2->unk5D == 0) {
                    bss_30[0]->unkC8 = 0.9f;
                } else if (arg2->unk5D == 1) {
                    bss_30[0]->unkC8 = 1.0f;
                } else if (arg2->unk5D == 2) {
                    bss_30[0]->unkC8 = 0.9f;
                } else if (arg2->unk5D == 3) {
                    bss_30[0]->unkC8 = 0.75f;
                }
                if (arg2->unk54 == 0) {
                    bss_30[0]->unk40 = bss_30[0]->unkC8 * 2000.0f;
                } else if (arg2->unk54 == 1) {
                    bss_30[0]->unk40 = bss_30[0]->unkC8 * 10000.0f;
                } else if (arg2->unk54 == 2) {
                    bss_30[0]->unk40 = bss_30[0]->unkC8 * 12000.0f;
                } else if (arg2->unk54 == 3) {
                    bss_30[0]->unk40 = bss_30[0]->unkC8 * 14000.0f;;
                }
                bss_30[0]->unkE8 = (s32) arg2->unk56;
                data_C = bss_30[arg2->unk26]->unk40;
                bss_30[arg2->unk26]->unkDC = (s32) arg2->unk52;
                bss_30[arg2->unk26]->unkDC = 0x168;
                bss_30[arg2->unk26]->unkC4 = (f32) bss_30[arg2->unk26]->unkDC / 60.0f;
                bss_30[arg2->unk26]->unkCC = (s32) arg2->unk2A;
                bss_30[arg2->unk26]->unkD0 = (s32) arg2->unk2C;
                bss_30[arg2->unk26]->unkD8 = 0;
                bss_30[arg2->unk26]->unkD4 = 1;
                temp_v0->unk4 = (s16) arg2->unk24 - 1;
                data_10 = 0;
                data_60 = 1;
                for (i = 0; i < (3 * 7); i++) {
                    ((f32*)data_6C)[i] = ((f32*)bss_30[0]->unk44)[i];
                }
                bss_30[0]->unk44[0][0] = arg2->r[0];
                bss_30[0]->unk44[0][1] = arg2->r[0];
                bss_30[0]->unk44[0][2] = arg2->r[1];
                bss_30[0]->unk44[0][3] = arg2->r[2];
                bss_30[0]->unk44[0][4] = arg2->r[3];
                bss_30[0]->unk44[0][5] = arg2->r[0];
                bss_30[0]->unk44[0][6] = arg2->r[0];
                bss_30[0]->unk44[1][0] = arg2->g[0];
                bss_30[0]->unk44[1][1] = arg2->g[0];
                bss_30[0]->unk44[1][2] = arg2->g[1];
                bss_30[0]->unk44[1][3] = arg2->g[2];
                bss_30[0]->unk44[1][4] = arg2->g[3];
                bss_30[0]->unk44[1][5] = arg2->g[0];
                bss_30[0]->unk44[1][6] = arg2->g[0];
                bss_30[0]->unk44[2][0] = arg2->b[0];
                bss_30[0]->unk44[2][1] = arg2->b[0];
                bss_30[0]->unk44[2][2] = arg2->b[1];
                bss_30[0]->unk44[2][3] = arg2->b[2];
                bss_30[0]->unk44[2][4] = arg2->b[3];
                bss_30[0]->unk44[2][5] = arg2->b[0];
                bss_30[0]->unk44[2][6] = arg2->b[0];
                bss_30[0]->unkEC[0] = arg2->fogMax[0] + 0x210;
                bss_30[0]->unkEC[1] = arg2->fogMax[1] + 0x210;
                bss_30[0]->unkEC[2] = arg2->fogMax[2] + 0x210;
                bss_30[0]->unkEC[3] = arg2->fogMax[3] + 0x210;
                bss_30[0]->unkEC[4] = arg2->fogMin[0] + 0x210;
                bss_30[0]->unkEC[5] = arg2->fogMin[1] + 0x210;
                bss_30[0]->unkEC[6] = arg2->fogMin[2] + 0x210;
                bss_30[0]->unkEC[7] = arg2->fogMin[3] + 0x210;
                bss_30[0]->unk114 = 1;
            } else if (arg2->unk58 & 0x20) {
                bss_30[arg2->unk26]->unk112 |= 0x40;
                data_4 = 1;
                bss_30[0]->unkC8 = 1.0f;
                bss_30[0]->unk40 = bss_30[0]->unkC8 * 10000.0f;
                data_C = bss_30[arg2->unk26]->unk40;
                bss_30[arg2->unk26]->unkDC = (s32) arg2->unk52;
                bss_30[arg2->unk26]->unkDC = 0x168;
                bss_30[arg2->unk26]->unkC4 = (f32) bss_30[arg2->unk26]->unkDC / 60.0f;
                bss_30[arg2->unk26]->unkD8 = 1;
                bss_30[arg2->unk26]->unkD4 = 0;
                bss_30[arg2->unk26]->unkCC = (s32) arg2->unk2A;
                bss_30[arg2->unk26]->unkD0 = (s32) arg2->unk2C;
                bss_30[0]->unk114 = 0;
            }
        }
    }
}

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
    if ((temp_t6 >> 1) != 0) {
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
