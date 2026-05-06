#include "modding.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "sys/main.h"

#include "recomp/dlls/engine/24_waterfx_recomp.h"

#define MAX_CIRCULAR_RIPPLES 30
#define MAX_MOVEMENT_RIPPLES 30
#define MAX_SPLASHES 10
#define MAX_SPLASH_PARTICLES 30

// size: 0x1C
typedef struct {
/*00*/ f32 x;
/*04*/ f32 y;
/*08*/ f32 z;
/*0C*/ f32 unkC;
/*10*/ f32 scale;
/*14*/ s16 yaw;
/*16*/ s16 alpha;
/*18*/ s16 decayRate;
} CircularWaterRipple;

// size: 0x5C
typedef struct {
/*00*/ f32 x;
/*04*/ f32 y;
/*08*/ f32 z;
/*0C*/ f32 unkC[6];
/*24*/ f32 unk24[6];
/*3C*/ f32 unk3C[6];
/*54*/ s16 alpha;
/*56*/ s16 unk56;
/*58*/ u8 particleCount;
} WaterSplash;

// size: 0x1C
typedef struct {
/*00*/ f32 x;
/*04*/ f32 y;
/*08*/ f32 z;
/*0C*/ f32 unkC;
/*10*/ f32 scale;
/*14*/ s16 alpha;
/*16*/ s16 yaw;
/*18*/ u8 hide;
} MovementWaterRipple;

// size: 0x14
typedef struct {
/*00*/ f32 xVel;
/*04*/ f32 zVel;
/*08*/ f32 speed; // lateral only
/*0C*/ f32 yVel;
/*10*/ s16 unk10;
/*12*/ s8 splashIdx; // index of the linked water splash instance
} WaterSplashParticle;

extern Vtx *sWaterSplashVerts;
extern Vtx *sWaterSplashPartVerts;
extern s32 sNumCircularRipples;
extern CircularWaterRipple *sCircularRipples;
extern s32 sNumWaterSplashes;
extern WaterSplash *sWaterSplashes;
extern s32 sNumMovementRipples;
extern MovementWaterRipple *sMovementRipples;
extern s32 sNumWaterSplashParticles;
extern WaterSplashParticle *sWaterSplashParticles;

extern void waterfx_spawn_circular_ripple(f32 x, f32 y, f32 z, s16 yaw, f32 arg4, s32 decayRate);

RECOMP_PATCH void waterfx_tick(void) {
    s32 i;
    f32 temp_fv1;
    f32 transparency;
    s16 yMove;
    s16 alpha;
    s16 xMove;
    s16 zMove;
    s32 j;
    Vtx* vtx;
    Vtx* temp_a1_2;
    WaterSplash* splash;
    CircularWaterRipple* circRipple;
    MovementWaterRipple* movRipple;
    WaterSplashParticle* part;
    WaterSplash *temp2;
    f32 temp;

    for (i = 0; i < MAX_CIRCULAR_RIPPLES; i++) {
        circRipple = &sCircularRipples[i];
        if (circRipple->alpha != 0) {
            circRipple->scale += 0.0007f * gUpdateRateF;
            circRipple->alpha -= (gUpdateRate * circRipple->decayRate);
            if (circRipple->alpha < 0) {
                circRipple->alpha = 0;
                sNumCircularRipples -= 1;
            }
        }
    }

    for (i = 0; i < MAX_SPLASHES; i++) {
        splash = &sWaterSplashes[i];
        if (splash->alpha != 0) {
            vtx = &sWaterSplashVerts[i * 14];
            transparency = vtx[1].v.ob[1] / 400.0f;
            if (transparency < 0.0f) {
                transparency = 0.0f;
            } else if (transparency > 1.0f) {
                transparency = 1.0f;
            } 
            j = 0;
            while (j < 14) {
                if (j & 1) {
                    vtx[j].v.ob[0] += (s16) (splash->unkC[(j % 12) >> 1] * gUpdateRateF * 100.0f);
                    vtx[j].v.ob[1] += (s16) (splash->unk24[(j % 12) >> 1] * gUpdateRateF * 100.0f);
                    vtx[j].v.ob[2] += (s16) (splash->unk3C[(j % 12) >> 1] * gUpdateRateF * 100.0f);
                }
                alpha = 255.0f - (transparency * 255.0f);
                vtx[j].v.cn[3] = alpha;
                j++;
            }
            if (vtx[1].v.ob[1] > 400) {
                splash->alpha = 0;
                sNumWaterSplashes -= 1;
            }
        }
    }

    for (i = 0; i < MAX_MOVEMENT_RIPPLES; i++) {
        movRipple = &sMovementRipples[i];
        if (movRipple->alpha != 0) {
            movRipple->scale += 0.004f * gUpdateRateF;
            movRipple->alpha -= gUpdateRate * 5;
            if (movRipple->alpha < 0) {
                movRipple->alpha = 0;
                sNumMovementRipples -= 1;
            }
            // @recomp: Re-enable movement ripples
            // if (movRipple->hide == FALSE) {
            //     movRipple->hide = TRUE;
            // }
        }
    }

    for (i = 0; i < MAX_SPLASH_PARTICLES; i++) {
        part = &sWaterSplashParticles[i];
        if (part->splashIdx != -1) {
            splash = &sWaterSplashes[part->splashIdx];
            vtx = &sWaterSplashPartVerts[i * 4];

            temp = 100.0f * gUpdateRateF;
            temp_fv1 = part->speed * temp;
            xMove = (part->xVel * temp_fv1);
            yMove = (part->yVel * temp);
            zMove = (part->zVel * temp_fv1);

            vtx->v.ob[0] += xMove;
            vtx->v.ob[1] += yMove;
            vtx->v.ob[2] += zMove;
            vtx += 3;
            vtx[-2].v.ob[0] += xMove;
            vtx[-2].v.ob[1] += yMove;
            vtx[-2].v.ob[2] += zMove;
            vtx[-1].v.ob[0] += xMove;
            vtx[-1].v.ob[1] += yMove;
            vtx[-1].v.ob[2] += zMove;
            vtx->v.ob[0] += xMove;
            vtx->v.ob[1] += yMove;
            vtx->v.ob[2] += zMove;

            part->yVel += -0.025f * gUpdateRateF;
            if (splash->alpha == 0) {
                if (vtx->v.ob[1] < 0) {
                    part->splashIdx = -1;
                    sNumWaterSplashParticles -= 1;
                    splash->particleCount--;
                    waterfx_spawn_circular_ripple((vtx->v.ob[0] / 100.0f) + splash->x, splash->y, (vtx->v.ob[2] / 100.0f) + splash->z, 0, 0.0f, 4);
                }
            } else {
                temp_a1_2 = &sWaterSplashVerts[part->splashIdx * 14];
                part->unk10 = (0xFF - temp_a1_2[1].v.cn[3]);
            }
        }
    }
}
