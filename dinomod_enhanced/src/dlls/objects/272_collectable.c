#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "custom_object_ids.h"

#include "common.h"
#include "sys/main.h"
#include "sys/newshadows.h"
#include "dlls/objects/common/collectable.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/272_collectable.h"

#include "recomp/dlls/objects/272_collectable_recomp.h"

typedef struct {
    u32 soundHandle;            //Cleared on free, but not used for any sound calls
    f32 distanceToPlayer;  
    f32 interactionRadius;
    f32 timerDestroy;           //Countdown after receiving collection message from sender
    s8 sidekickArgBase;         //Base arg value used for sidekick-related collectables
    u8 objHitsValue;
    u8 unused12;
    u8 pause;                   //Control function ends early when this is set
    s16 gamebitCollected;       //Set when collected: collectable vanishes when set
    u8 unused16;
    u8 unused17;
    s16 gamebitShow;            //(Optional) Collectable only shows up once gamebit set
    u8 shadowOpacity;           //For magic
    u8 unused1B;
    s32 areaValue;              //Received from Area object, if the collectable is inside one
    s8 delayCollect;            //Timer, can only collect object once at 0
    u8 moving;
    u8 isHidden;
    u32 uID;
    Vec3f savedPosition;
    f32 pitchAnimate;           //Rotation for Dino Eggs' rattle animation
    s16 soundTimer;             //Timer for Dino Eggs' rattle sound/animation
    u8 useColourMultiplier;     //Toggles colour multiplier
    u8 interactFlags;           //Toggles ability to collect
    u8 multiplyR;               //Colour multiplier for model
    u8 multiplyG;               //Colour multiplier for model
    u8 multiplyB;               //Colour multiplier for model
    u8 unused3F;
    s16 rootTimer;              //Affects opacity of Alpine Root
    u16 unused42;
} Collectable_Data;

typedef enum {
    Collectable_FLAG_Interaction_Off = 1
} Collectable_Flags;

/** 
  * - Adds spinning behaviour for custom `OBJ_WCTrexTooth2` collectable (originally by MusicalProgrammer).
  * - Framerate dependent behaviour fixes.
  */
RECOMP_PATCH void collectable_handle_animation_and_fx(Object* self) {
    s16 id;
    Collectable_Data* objdata;
    ObjectShadow* shadow;
    s32 opacity;
    CollectableDef* collectableDef;
    s32 temp;

    collectableDef = self->def->collectableDef;
    if (collectableDef == NULL) {
        return;
    }

    objdata = self->data;

    //Handle magic (@framerate-dependent)
    if (collectableDef->type == Collectable_Type_Magic) {
        //Spin (@recomp: fix framerate dependency, assuming average N64 gUpdateRate of 2)
        self->srt.yaw   += 25*gUpdateRate; 
        self->srt.pitch += 25*gUpdateRate;
        self->srt.roll  += 25*gUpdateRate;

        //Grow
        if (self->srt.scale < 0.008f) {
            self->srt.scale += 0.0001f * gUpdateRateF; //@recomp: fix framerate dependency
        }

        //Vertical motion + shadow animation
        if (self->velocity.y < 0.0f) {
            self->srt.transl.y += self->velocity.y * gUpdateRate;   //@recomp: fix framerate dependency
            self->velocity.y += 0.015f * gUpdateRateF;              //@recomp: fix framerate dependency
            if (self->velocity.y >= 0.0f) {
                shadows_func_8004D974(1);
                objdata->shadowOpacity = 0;
            }
        } else {
            shadow = self->shadow;
            if (shadow) {
                opacity = objdata->shadowOpacity + (gUpdateRate * 8);
                if (opacity > OBJECT_OPACITY_MAX) {
                    opacity = OBJECT_OPACITY_MAX;
                }
                objdata->shadowOpacity = opacity;
                temp = shadows_calc_opacity(self, shadow);
                shadow->opacity = (temp * (opacity + 1)) >> 8;
            }
        }

        //Sparkles
        if (rand_next(0, 80) == 0) {
            gDLL_17_partfx->vtbl->spawn(self, 0x47, 0, 4, -1, 0);
        }
    }

    id = self->id;
    switch (id) {
    case OBJ_meatPickup:
        //Play rattle sound when random timer runs out
        objdata->soundTimer -= gUpdateRate;
        if (objdata->soundTimer <= 0) {
            objdata->pitchAnimate = rand_next(600, 800);
            objdata->soundTimer = rand_next(180, 240);
            gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_8FC_Egg_Rattle, MAX_VOLUME, 0, 0, 0, 0);
        }

        //Rapidly oscillate rotational pitch for a basic rattle animation
        self->srt.pitch = objdata->pitchAnimate;
        objdata->pitchAnimate *= -0.8f; //@framerate-dependent 
                                        //(maybe intentional though, to avoid skips creating a 
                                        // run of frames where the value has the same sign?)
        if ((10 > self->srt.pitch) && (self->srt.pitch > -10)) {
            self->srt.pitch = 0;
            return;
        }
        break;
    case OBJ_DIM2PuzzleKey:
    case OBJ_DIM2GoldKey:
    case OBJ_DIM2SilverKey:
    case OBJ_DIMTruthHorn:
    case OBJ_DIMBridgeCogCol:
        self->srt.yaw += gUpdateRate * 200;
        return;
    case OBJ_SC_golden_nugge:
        if (objdata->distanceToPlayer < 200.0f) {
            if (rand_next(0, 10) == 0) {
                gDLL_17_partfx->vtbl->spawn(self, 0x423, 0, 2, -1, 0);
            }
            self->srt.yaw += 182.0f * gUpdateRateF;
            return;
        }
        break;
    case OBJ_WCTrexTooth:
        if (objdata->distanceToPlayer < 200.0f) {
            if (rand_next(0, 10) == 0) {
                if (self->modelInstIdx == 0) {
                    gDLL_17_partfx->vtbl->spawn(self, 0x73D, 0, 2, -1, 0);
                } else {
                    gDLL_17_partfx->vtbl->spawn(self, 0x73E, 0, 2, -1, 0);
                }
            }
            self->srt.yaw += 182.0f * gUpdateRateF;
        }
        break;
    case OBJ_WCTrexTooth2: //@recomp: new case
        self->srt.yaw += 182.0f * gUpdateRateF;
        break;
    }
}
