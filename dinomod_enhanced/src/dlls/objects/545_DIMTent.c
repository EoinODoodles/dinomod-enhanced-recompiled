#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "configs.h"

#include "common.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/gfx/modgfx.h"
#include "sys/main.h"
#include "sys/objanim.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/common/collectable.h"

#include "recomp/dlls/objects/545_DIMTent_recomp.h"

typedef struct {
    ObjSetup base;
    s8 yaw;
    s16 tentIndex;     //An index (0-11 inclusive) for deciding which tent holds the bridge cog
    u16 unused1C;
    s16 gamebitBurnt;  //Set when the tent's been destroyed, prevents it reappearing
} DIMTent_Setup;

typedef struct {
    f32 maskY;      //Progress of the burn masking effect
    f32 maskSpeed;  //Speed/direction of the burn masking effect
    u8 isBurning;   //Starts the burn mask animation
    s8 hitPoints;   //Attacks needed until the tent begins burning (always starts at 1)
    /* RECOMP */
    u8 cogCreated;   //Whether the cog's already been dropped by the tent
    u8 outerOpacity; //The opacity of the main outer tent model
    u8 innerOpacity; //The opacity of the inner burnt tent model
    u32 soundHandle; //For the burning sound loop
} DIMTent_Data_Extended;

extern DLL_IModgfx* dModGfxDLL;
extern const DLTri sMaskTris[12];
extern const Vec3f sMaskVertCoords[8];
extern const f32 sMaskSpeeds[1];

#define MASK_SPEED 0.1f

#define MASK_MAX 82.0f
#define MASK_MIN -5.0f

#define MASK_TOP_Y 20
#define MASK_BASE_Y -7
#define MASK_WIDTH_TOP 2.0
#define MASK_WIDTH_BASE 34.5

#define SCALE_Y 20.0f
#define SCALE_X 20.0f
#define SCALE_Z 20.0f

#define THRESHOLD_SNOW_EVAPORATE 60.0f
#define THRESHOLD_END_FADEOUT 68.0f

extern void DIMTent_draw_mask(Object* self, Gfx** gdl, Mtx** mtxs, Vtx_t** vtxs, Triangle** pols);

// Split the tent's Bridge Cog behaviour out into its own function
static void DIMTent_createCog(Object* self, DIMTent_Setup* objSetup, DIMTent_Data_Extended* objData) {
    //Check if the Bridge Cog's already collected
    if (main_get_bits(BIT_DIM_Gear_3)) {
        return;
    }

    //Check if this isn't the tent that should drop the Bridge Cog
    if (main_get_bits(BIT_DIM_Gear_3_Random_Tent) != (u32)objSetup->tentIndex) {
        return;
    }

    //Check if the cog's already been created
    if (objData->cogCreated) {
        return;
    }

    //Create the cog
    {
        Collectable_Setup* cogSetup;
        cogSetup = (Collectable_Setup*)obj_alloc_setup(sizeof(Collectable_Setup), OBJ_DIMBridgeCogCol);
        cogSetup->base.x = objSetup->base.x;
        cogSetup->base.y = objSetup->base.y + 8.0f;
        cogSetup->base.z = objSetup->base.z;
        cogSetup->base.loadFlags = objSetup->base.loadFlags;
        cogSetup->base.byte5 = objSetup->base.byte5;
        cogSetup->base.byte6 = objSetup->base.byte6;
        cogSetup->base.fadeDistance = objSetup->base.fadeDistance;
        cogSetup->gamebitCollected = BIT_DIM_Gear_3;
        cogSetup->gamebitSecondary = NO_GAMEBIT;
        cogSetup->gamebitCount = NO_GAMEBIT;
        cogSetup->objHitsValue = 5;
        cogSetup->yaw = self->srt.yaw >> 8;
        obj_create((ObjSetup*)cogSetup, 5, self->mapID, -1, NULL);
        objData->cogCreated = TRUE;
    }
}

RECOMP_PATCH void DIMTent_setup(Object* self, DIMTent_Setup* objSetup, s32 arg2) {
    DIMTent_Data_Extended* objData = self->data;
    /* RECOMP */
    u8 tentConfig = recomp_get_config_u32("dim_tent_cinders");

    self->srt.yaw = objSetup->yaw << 8;
    self->stateFlags |= OBJSTATE_UPDATE_DISABLED;

    objData->hitPoints = 1;

    //Check if already burnt
    if (main_get_bits(objSetup->gamebitBurnt)) {
        objData->hitPoints = 0;
        self->objhitInfo->unk58 &= ~1;
        objData->outerOpacity = 0;

        if (tentConfig == DIM_TENT_CINDERS_ON_FADEOUT) {
            objData->innerOpacity = 0;
        } else {
            objData->innerOpacity = OBJECT_OPACITY_MAX;
        }
    } else {
        objData->outerOpacity = OBJECT_OPACITY_MAX;
        objData->innerOpacity = OBJECT_OPACITY_MAX;
    }

    objData->maskSpeed = -sMaskSpeeds[0]; //@recomp: start burning downwards, without briefly burning upwards
    dModGfxDLL = dll_load_deferred(DLL_ID_165, 1);
}

/** 
  * - Fix a bug where you can miss out on the Bridge Cog 
  * - Fix a bug where the tents' burn animation could ping-pong and play in reverse 
  *   (it would happen if they're off-screen/not being drawn around the intended endpoint of the behaviour)
  */
RECOMP_PATCH void DIMTent_control(Object* self) {
    Object* listedObject;
    DIMTent_Setup* objSetup;
    DIMTent_Data_Extended* objData;
    s32 index;
    s8 damaged;
    /* RECOMP */
    s32 opacity;

    objSetup = (DIMTent_Setup*)self->setup;
    objData = self->data;

    //@recomp: make sure to create the cog, avoiding a softlock where it can be missed
    if (objData->cogCreated == FALSE) {
        DIMTent_createCog(self, objSetup, objData);
    }

    //When outer tent is invisible / burnt
    if (objData->outerOpacity == 0) {
        //@recomp: stop sound loop
        if (objData->soundHandle != 0) {
            gDLL_6_AMSFX->vtbl->stop(objData->soundHandle);
            objData->soundHandle = 0;

            //Play "whoosh" sound as snow evaporates
            gDLL_6_AMSFX->vtbl->play(self, 0x95B, 0x40, NULL, NULL, 0, NULL);
        }

        //Fade out the inner tent
        if (objData->innerOpacity > 0) {
            if (objData->innerOpacity >= (gUpdateRate * 4)) {
                objData->innerOpacity -= gUpdateRate * 4;
            } else {
                objData->innerOpacity = 0;
            }
        }

        return;
    }

    //When burning, remove collision and gradually mask the model away
    if (objData->hitPoints <= 0) {
        //@recomp: remove collision once nearly burnt
        if ((self->objhitInfo->unk58 & 1) && 
            ((objData->outerOpacity < OBJECT_OPACITY_MAX) || (objData->maskY >= MASK_MAX - 10))
        ) {
            self->objhitInfo->unk58 &= ~1;
        }

        //Handle burn mask speeds/progress
        if (objData->isBurning == TRUE) {
            objData->maskY += objData->maskSpeed * gUpdateRateF;
            if (objData->maskY > MASK_MAX) {
                objData->maskY = MASK_MAX;
                objData->maskSpeed = -MASK_SPEED;
            } else if (objData->maskY < MASK_MIN) {
                objData->maskY = MASK_MIN;
                objData->maskSpeed = MASK_SPEED;
                dModGfxDLL->vtbl->func0(self, 0, 0, 4, -1, 0);
            }
        }
    }

    //Fade out the tent when nearly fully burnt
    /*@recomp: in the unedited ROM, this won't run when the tent is off-screen since 
               it's handled via the print function instead of control, which can cause
               the burn anim to ping-pong. To fix that, it's been moved to control instead! */
    if ((objData->maskY >= THRESHOLD_END_FADEOUT) && (objData->outerOpacity > 0)) {
        if (objData->outerOpacity >= (gUpdateRate * 4)) {
            objData->outerOpacity -= gUpdateRate * 4;
        } else {
            objData->outerOpacity = 0;
        }
    }

    //Do nothing if this is the unused big tent (probably intended to be burnt a different way)
    if (self->id == OBJ_DIMBigTent) {
        return;
    }

    //Check for damage via polyhits
    damaged = FALSE;
    for (index = 0; index < self->polyhits->unk10F; index++) {
        listedObject = self->polyhits->unk100[index];

        //Curiously, setting fire to the tents via SnowHorn attacks seems to be intentional!
        if ((listedObject->id == OBJ_DIMSnowHorn1) || (listedObject->id == OBJ_DIMCannonBall)) {
            damaged = TRUE;
            break;
        }
    }

    //Handle being damaged
    if (damaged) {
        //Decrement the tent's health (health starts at 1, so it begins burning after any hit anyway)
        objData->hitPoints--;
        if (objData->hitPoints > 0) {
            return;
        }

        //After being fully damaged, track that the tent has been burnt and start the burning effect
        main_set_bits(objSetup->gamebitBurnt, 1);
        objData->isBurning = TRUE;

        //@recomp: Start burning sound loop
        if (objData->soundHandle == 0) {
            gDLL_6_AMSFX->vtbl->play(self, SOUND_50b_Fire_Burning_High_Loop, 0x60, &objData->soundHandle, NULL, 0, NULL);
        }

        //Drop the bridge cog if this tent's index matches the random one picked by DIMLevelControl
        DIMTent_createCog(self, objSetup, objData);
    }
}

RECOMP_PATCH void DIMTent_print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    /* RECOMP */
    DIMTent_Data_Extended* objData = self->data;
    u8 prevOpacity = self->opacity;
    u8 prevOpacityWithFade = self->opacityWithFade;
    u8 tentConfig = recomp_get_config_u32("dim_tent_cinders");

    s32 tempOpacity;

    if (visibility) {
        //Draw the inner burnt tent
        if ((objData->isBurning == TRUE) || (objData->outerOpacity == 0) || (objData->maskY > 0)) {
            if (tentConfig >= DIM_TENT_CINDERS_ON_FADEOUT) {
                //Switch to its model
                if (self->modelInstIdx != 1) {
                    self->modelInstIdx = 1;
                }

                //Scale down slightly to avoid z-fighting/clipping
                if (self->srt.scale > 0.9f) {
                    self->srt.scale = 0.9f;
                }

                //Handle optional fadeout
                if (tentConfig == DIM_TENT_CINDERS_ON_FADEOUT) {
                    if (objData->innerOpacity && (objData->innerOpacity < OBJECT_OPACITY_MAX)) {
                        self->opacityWithFade = prevOpacity * (((f32)objData->innerOpacity) / 255.0f);
                    }
                }

                //Draw the tent if it hasn't faded out (or if the config option has it persist)
                if (!((tentConfig == DIM_TENT_CINDERS_ON_FADEOUT) && (objData->innerOpacity == 0))) {
                    draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);
                }

                //Restore opacity after draw
                if (self->opacityWithFade != prevOpacityWithFade) {
                    self->opacityWithFade = prevOpacityWithFade;
                }
            }

            if (objData->outerOpacity > 0) {
                DIMTent_draw_mask(self, gdl, mtxs, (Vtx_t**)vtxs, pols);
            }
        }

        //Draw the outer tent
        if (objData->outerOpacity > 0) {
            //Switch to its model
            if (self->modelInstIdx != 0) {
                self->modelInstIdx = 0;
            }

            //Restore scale
            if (self->srt.scale < 1) {
                self->srt.scale = 1;
            }

            //Set opacity and draw
            self->opacity = objData->outerOpacity;
            draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);
            self->opacity = prevOpacity;
        }
    }
}

// Moving opacity handling to control, to fix the bug where the burning can continue in reverse
RECOMP_PATCH void DIMTent_draw_mask(Object* self, Gfx** gdl, Mtx** mtxs, Vtx_t** vtxs, Triangle** pols) {
    Vtx_t* initVtx;
    DIMTent_Data_Extended* objData;
    s32 index;
    Vtx_t* vtx;
    f32 maskY;
    f32 maskProgress;
    SRT srt;
    u32 i;

    objData = self->data;

    maskProgress = maskY = objData->maskY;

    initVtx = *vtxs;
    vtx = *vtxs;

    maskProgress /= MASK_MAX; //tValue for the mask animation (0 at top, 1 at bottom)

    //Set up mesh masking draw configs
    dl_set_prim_color(gdl, 0xFF, 0xFF, 0xFF, 0x80);
    gSPLoadGeometryMode(*gdl, G_ZBUFFER | G_SHADE | G_CULL_BACK | G_FOG | G_SHADING_SMOOTH);
    dl_apply_geometry_mode(gdl);
    gDPSetCombineLERP(*gdl, TEXEL0, 0, SHADE, 0, TEXEL0, 0, PRIMITIVE, 0, COMBINED, 0, PRIMITIVE, 0, 0, 0, 0, COMBINED);
    dl_apply_combine(gdl);

    //Position the cube's vertices (forming a section of a pyramid)
    for (i = 0; i < ARRAYCOUNT(sMaskVertCoords); i++) {
        if (i < 4) {
            vtx->ob[1] = MASK_MAX * SCALE_Y;
        } else {
            //Move the base of the cube with maskY
            vtx->ob[1] = (MASK_MAX - maskY) * SCALE_Y;
        }

        if (i < 4) {
            vtx->ob[0] = sMaskVertCoords[i].x * SCALE_X;
            vtx->ob[2] = sMaskVertCoords[i].z * SCALE_Z;
        } else {
            //Expand the bottom of the cube outwards as the mask moves down
            vtx->ob[0] = sMaskVertCoords[i].x * maskProgress * SCALE_X;
            vtx->ob[2] = sMaskVertCoords[i].z * maskProgress * SCALE_Z;
        }

        vtx->cn[0] = 0xFF;
        vtx->cn[1] = 0;
        vtx->cn[2] = 0;
        vtx->cn[3] = 0x40;

        vtx++;
    }

    gDPSetOtherMode(*gdl,
        G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP |
        G_TP_PERSP | G_CYC_2CYCLE | G_PM_NPRIMITIVE, G_AC_NONE | G_ZS_PIXEL | Z_CMP | Z_UPD | IM_RD | CVG_DST_SAVE |
        ZMODE_XLU | FORCE_BL | G_RM_FOG_SHADE_A | GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_MEM, G_BL_1MA));
    dl_apply_other_mode(gdl);

    srt.transl.x = self->srt.transl.x;
    srt.transl.y = self->srt.transl.y;
    srt.transl.z = self->srt.transl.z;
    srt.yaw = self->srt.yaw;
    srt.pitch = 0;
    srt.roll = 0;
    srt.scale = 0.05f;
    camera_setup_object_srt_matrix(gdl, mtxs, &srt, 1, 0, NULL);

    gSPVertex((*gdl)++, OS_PHYSICAL_TO_K0(initVtx), 8, 0);
    dl_triangles(gdl, (DLTri*)sMaskTris, ARRAYCOUNT(sMaskTris));

    //Set transform for particles
    srt.transl.x = (30.0f * maskProgress) + 2.0f;
    srt.transl.y = (MASK_MAX - maskY) - 4.0f;
    srt.transl.z = (30.0f * maskProgress) + 2.0f;
    srt.yaw = self->srt.yaw;
    srt.pitch = 0;
    srt.roll = 0;
    srt.scale = 0.001f;

    //Create fire particles until nearly fully burnt (particle count scales with maskY)
    if (objData->maskY < THRESHOLD_END_FADEOUT) {
        for (index = 0; index < (s32)(objData->maskY / 10.0f); index++) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_203, &srt, 4, -1, NULL);
        }
    }

    //Randomly create snow evaporation particles approaching the end of the burning
    if (objData->maskY > THRESHOLD_SNOW_EVAPORATE) {
        if (rand_next(0, 2) == 0) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_206, &srt, 4, -1, NULL);
        }
    //Otherwise create fire/smoke/ember particles
    } else {
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_203, &srt, 4, -1, NULL);
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_203, &srt, 4, -1, NULL);
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_205, &srt, 4, -1, NULL);
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_204, &srt, 4, -1, NULL);
    }

    //@recomp: opacity handling moved to control to avoid ping-pong burning bug

    *vtxs = vtx;
}

RECOMP_PATCH void DIMTent_free(Object* self, s32 arg1) {
    DIMTent_Data_Extended* objData = self->data; //@recomp

    if (dModGfxDLL) {
        dll_unload(dModGfxDLL);
    }

    //@recomp: stop sound loop
    if (objData->soundHandle != 0) {
        gDLL_6_AMSFX->vtbl->stop(objData->soundHandle);
        objData->soundHandle = 0;
    }
}

RECOMP_PATCH u32 DIMTent_get_data_size(Object *self, u32 a1) {
    return sizeof(DIMTent_Data_Extended);
}
