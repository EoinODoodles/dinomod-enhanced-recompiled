#include "modding.h"

#include "PR/gbi.h"
#include "PR/os.h"
#include "sys/camera.h"
#include "sys/gfx/textable.h"
#include "sys/map.h"
#include "sys/math.h"
#include "sys/memory.h"
#include "sys/rand.h"
#include "dll.h"
#include "macros.h"

#include "recomp/dlls/engine/10_newstars_recomp.h"

typedef struct {
/*00*/ Vtx* vtxs[2];
/*08*/ DLTri* tris[2];
/*10*/ Mtx* mtxs[2];
/*18*/ s32 vtxSel;
/*1C*/ s32 triSel;
/*20*/ s32 mtxSel;
/*24*/ u8 _unk24[0xC4 - 0x24];
/*C4*/ void* membase;
} StarsState;

// size: 0x28
typedef struct {
/*00*/ s32 unk0;
/*04*/ s32 unk4; // legacy texture ID (textable)?
/*08*/ s32 unk8;
/*0C*/ Vec3f unkC;
/*18*/ f32 scale;
/*1C*/ s16 unk1C;
/*1E*/ s16 unk1E;
/*20*/ s16 unk20;
/*22*/ s16 unk22;
/*24*/ s16 numStars;
/*26*/ s16 unk26;
} StarCluster;

/*0x8*/ extern Texture* sStarTexture;
/*0x12C*/ extern s32 sStarTextureProgress;

/*0x0*/ extern StarsState* sState;
/*0x8*/ extern StarCluster sClusters[9];

extern s32 newstars_func_119C(Vec3f* arg0, Vec3f* arg1, f32* arg2);

RECOMP_PATCH void newstars_Print(Gfx** gdl) {
    Mtx* temp_s4;
    Camera* camera;
    s32 dlNeedsTex;
    Vtx* vtxs;
    DLTri* tris;
    MtxF sp194;
    s32 i;
    MtxF sp150;
    MtxF sp110;
    MtxF spD0;
    s32 _pad2;
    Mtx* projMtx;
    MtxF* temp_s0_2;
    Vec3f spB8;
    Vec3f spAC;
    f32 spA8;
    s32 nstars;
    SRT sp8C;
    f32 sp88;
    f32 temp_s0;
    u32 newvar;
    s32 texAnimFlags;
    s32 vtxIdx;
    s32 _pad;

    dlNeedsTex = TRUE;
    spA8 = 0.0f;
    sp88 = 0.0f;
    temp_s0 = 0.0f;
    if (gDLL_7_Newday->vtbl->func8(&sp88) != 0) {
        if (sp88 > 4000.0f) {
            sp88 = 255.0f;
        } else {
            sp88 = (sp88 / 4000.0f) * 255.0f;
        }
    } else if ((sp88 > 12000.0f) || (sp88 == 0.0f)) {
        return;
    } else {
        sp88 = 255.0f - ((sp88 / 12000.0f) * 255.0f);
    }

    camera = camGet();
    spB8.x = 0.0f;
    spB8.y = 0.0f;
    spB8.z = 1.0f;
    spAC.x = 0.0f;
    spAC.y = 0.0f;
    spAC.z = 1.0f;
    sp8C.transl.x = 0.0f;
    sp8C.transl.y = 0.0f;
    sp8C.transl.z = 0.0f;
    sp8C.roll = 0;
    sp8C.pitch = 0;
    sp8C.yaw = 0x4000 - camera->srt.yaw;
    mathRotateRPY(&sp8C, spB8.f);
    newstars_func_119C(&spB8, &spAC, &spA8);
    sState->mtxSel = 1 - sState->mtxSel;
    temp_s4 = sState->mtxs[sState->mtxSel];
    newvar = (u32)temp_s4;
    temp_s0_2 = &spD0;
    for (i = 0; i < (4*4); i++) {
        ((f32*)temp_s0_2)[i] = 0.0f;
    }
    spD0.m[1][1] = 1.0f;
    spD0.m[3][3] = 1.0f;
    spD0.m[0][0] = mathCosfInterp(-(s16)temp_s0);
    spD0.m[0][2] = mathSinfInterp(-(s16)temp_s0);
    spD0.m[2][0] = -mathSinfInterp(-(s16)temp_s0);
    spD0.m[2][2] = mathCosfInterp(-(s16)temp_s0);
    spD0.m[3][0] = camera->srt.transl.x - gWorldX;
    spD0.m[3][1] = camera->srt.transl.y;
    spD0.m[3][2] = camera->srt.transl.z - gWorldZ;
    projMtx = camGetRSPProjectionMtx();
    temp_s0_2 = camGetViewMtx();
    guMtxL2F(sp194.m, projMtx);
    mathMtxCatF(temp_s0_2, &sp194, &sp150);
    mathMtxCatF(&spD0, &sp150, &sp110);
    mathMtxF2L(&sp110, temp_s4);
    gSPForceMatrix((*gdl)++, OS_PHYSICAL_TO_K0(newvar));
    tris = sState->tris[sState->triSel];
    vtxs = sState->vtxs[sState->vtxSel];
    vtxIdx = 0;
    // @recomp: Don't zero out texture anim speed (chosen speed is opinionated)
    sStarTexture->animSpeed = 60;
    texAnimFlags = 0;
    texAnimateTexture(sStarTexture, &texAnimFlags, &sStarTextureProgress);
    for (i = 0; i < 9; i++) {
        //if (1) {} // @fake
        newstars_func_119C(&spB8, &sClusters[i].unkC, &spA8);
        if (spA8 > 0.5f) {
            if (dlNeedsTex) {
                texDPTextures(gdl, sStarTexture, NULL, RENDER_NO_CULL, TEX_FRAMEBLEND(0, sStarTextureProgress << 8), FALSE, TRUE);
                dlNeedsTex = FALSE;
            }
            nstars = sClusters[i].numStars;
            while (nstars != 0) {
                if (nstars >= 9) {
                    if (nstars % 4) {
                        dlSetPrimColor(gdl, 255, 255, 255, (u8) ((mathRnd(0, 255) * (s16) sp88) >> 8));
                    } else {
                        dlSetPrimColor(gdl, 255, 255, 255, 155);
                    }
                    gSPVertex((*gdl)++, OS_PHYSICAL_TO_K0(&vtxs[vtxIdx]), 24, 0);
                    dlTriangles(gdl, tris + 1, 8);
                    vtxIdx += 24;
                    nstars -= 8;
                } else {
                    dlSetPrimColor(gdl, 255, 255, 255, (u8) ((mathRnd(0, 255) * (s16) sp88) >> 8));
                    gSPVertex((*gdl)++, OS_PHYSICAL_TO_K0(&vtxs[vtxIdx]), nstars * 3, 0);
                    dlTriangles(gdl, tris + 1, nstars);
                    vtxIdx += nstars * 3;
                    nstars = 0;
                }
            }
        } else {
            vtxs += 3 * sClusters[i].numStars;
        }
    }
    // FAKE
    i = 0;
    sState->triSel = 1 - sState->triSel;
    sState->vtxSel = 1 - sState->vtxSel;
}
