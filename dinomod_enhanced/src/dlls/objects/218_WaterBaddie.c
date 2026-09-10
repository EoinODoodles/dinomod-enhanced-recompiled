#include "game/objects/interaction_arrow.h"
#include "modding.h"
#include "recomp/dlls/_asm/789_recomp.h"
#include "recomputils.h"

#include "dll.h"
#include "dlls/engine/33_BaddieControl.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "sys/rand.h"

#include "recomp/dlls/_asm/218_recomp.h"

// #define DEBUG_WATERBADDIE_CURVES

//TEMPORARY DEFINES
#define WaterBaddie_obj_Setup dll_218_obj_Setup
#define WaterBaddie_obj_Control dll_218_obj_Control
#define WaterBaddie_obj_Update dll_218_obj_Update
#define WaterBaddie_obj_GetDataSize dll_218_obj_GetDataSize
#define WaterBaddie_animCallback dll_218_func_9D0
#define WaterBaddie_logicState0Hit dll_218_func_1D8C
#define WaterBaddie_logicState3Swimming dll_218_func_203C
#define WaterBaddie_tick dll_218_func_C6C
#define WaterBaddie_engageTarget dll_218_func_1088
#define WaterBaddie_searchForTarget dll_218_func_1238

#define sAnimStateCallbacks bss_0
#define sLogicStateCallbacks bss_10
#define sCurveValue bss_44
#define dCurveTypes data_0
#define dWaterSounds data_8
#define dBaddieSounds data_C
//END OF TEMPORARY DEFINES

typedef struct {
    f32 pitchSpeed;
    f32 rollSpeed;
    f32 pitchAcceleration;
    f32 waterHeight;
    f32 turnSpeed;
    f32 moveSpeed;
    f32 unk18;
    s16 bobPhaseAngle;
    s16 prevYaw;
    u16 rippleFXTimer;
    u16 turnFXTimer;
    u8 createTurnRipples;
    s32 diveAmount;
    /* RECOMP */
    f32 groundHeight;       //worldSpace Y of the closest ground plane under the WaterBaddie's water plane
    s32 curveValue;         //objData version of sCurveValue, so it only affects a particular WaterBaddie
    f32 stunnedTimer;       //objData version of sStunnedTimer, so it only affects a particular WaterBaddie
    s8 curveSearchTimer;    //Interval between curve searches, for when the WaterBaddie fails to find its curves on setup
    u8 customFlags;         //`WaterBaddie_CustomFlags`
} WaterBaddie_DataActual;

typedef enum {
    WaterBaddie_CUSTOMFLAG_1_Shallow_Water = 1
} WaterBaddie_CustomFlags;

typedef enum {
    WaterBaddie_ASTATE_0_Turn_To_Target,
    WaterBaddie_ASTATE_1_Swimming,
    WaterBaddie_ASTATE_2_Hit
} WaterBaddie_AnimStates;

typedef enum {
    WaterBaddie_LSTATE_0_Hit,
    WaterBaddie_LSTATE_1_Dying,
    WaterBaddie_LSTATE_2_Dead,
    WaterBaddie_LSTATE_3_Swimming,
    WaterBaddie_LSTATE_4_Top
} WaterBaddie_LogicStates;

extern int WaterBaddie_animCallback(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue);
extern void WaterBaddie_tick(Object* self, Baddie* baddie, ObjFSA_Data* fsa);
extern void WaterBaddie_engageTarget(Object* self, AnimObj_Data* animData, Baddie* baddie, ObjFSA_Data* fsa);
extern void WaterBaddie_searchForTarget(Object* self, Baddie* baddie, ObjFSA_Data* fsa);

/*0x0*/ extern ObjFSA_StateCallback sAnimStateCallbacks[3];
/*0x10*/ extern ObjFSA_StateCallback sLogicStateCallbacks[5];

/*0x0*/ extern s32 dCurveTypes[];
/*0x8*/ extern u16 dWaterSounds[];
/*0xC*/ extern u16 dBaddieSounds[];

/*0x44*/ extern s32 sCurveValue;

/* Store the ground height too, as well as the water height */
RECOMP_PATCH void WaterBaddie_obj_Setup(Object* self, Baddie_Setup* objSetup, s32 reset) {
    Baddie* baddie;
    WaterBaddie_DataActual* objData;
    s32 count;
    f32 depth;
    s32 i;
    TrackHeightResult** trackResult;
    u8 flags;

    baddie = self->data;
    
    flags = 0x10;
    if (reset) {
        flags = 0x10 | 1;
    }
    if (!(objSetup->unk2B & 1)) {
        flags |= 8;
    }
    gDLL_33_BaddieControl->vtbl->setup(self, objSetup, baddie, 3, 5, 0x100, flags, 20.0f);
    
    self->animCallback = WaterBaddie_animCallback;
    gDLL_18_objfsa->vtbl->set_anim_state(self, &baddie->fsa, 0);
    baddie->fsa.logicState = WaterBaddie_LSTATE_4_Top;
    
    objData = baddie->objdata;
    objData->prevYaw = self->srt.yaw;
    objData->moveSpeed = objSetup->unk2F / 100.0f;
    
    count = trackGetHeight(self, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, &trackResult, 0, 0);
    objData->waterHeight = 0.0f;
    
    //Find the local water height
    //(This is only done during setup, so it assumes the WaterBaddie will be swimming around a perfectly flat water plane)
    if (count) {
        for (i = 0, objData->waterHeight = -9999.0f; i < count; i++){
            depth = trackResult[i]->y - self->srt.transl.y;
            if ((trackResult[i]->unk14 == 0xE) && (objData->waterHeight < depth)) {
                objData->waterHeight = depth;
            }
        }
    }    

    objData->waterHeight += self->srt.transl.y;
    objData->unk18 = 0.075f;

    //@recomp: store ground height too (closest under the water height)
    {
        for (i = 0, objData->groundHeight = -9999.0f; i < count; i++){
            if ((trackResult[i]->unk14 != 0xE) && 
                (trackResult[i]->y < objData->waterHeight) && 
                (objData->groundHeight < trackResult[i]->y)
            ) {
                objData->groundHeight = trackResult[i]->y;
            }
        }
        objData->groundHeight += 3.0f; //Add a little bit of padding

        //Flag when the WaterBaddie is in shallow water
        if (objData->waterHeight - 15.0f < objData->groundHeight) {
            objData->customFlags |= WaterBaddie_CUSTOMFLAG_1_Shallow_Water;
        }
    }
}

/* Edited to handle when the WaterBaddie fails to find its curves.

  Fixes a bug where the WaterBaddie along Discovery Falls' entrance route would go in a straight line, 
  passing clean through the wall ahead of it when the player is heading back to SwapStone Circle from Discovery Falls. 
  The WaterBaddie used continue on straight ahead into infinity, similar to an occasional bug that happens to the ThornTails!
  Maybe the WaterBaddie can load before its nearby curves do when the player is travelling in the DF->SC direction?
*/
RECOMP_PATCH void WaterBaddie_obj_Control(Object* self) {
    Baddie* baddie;
    Baddie_Setup* objSetup;
    /* RECOMP */
    WaterBaddie_DataActual* objData;

    baddie = self->data;
    objSetup = (Baddie_Setup*)self->setup;
    
    if (self->unkDC != 0) {
        if (gDLL_29_Gplay->vtbl->did_time_expire(objSetup->base.uID)) {
            gDLL_33_BaddieControl->vtbl->setup(self, objSetup, baddie, 3, 5, 0x100, 0x30, 20.0f);
            dll_amSfx->Play(self, SOUND_B20_Low_Grunt, MAX_VOLUME, NULL, NULL, 0, NULL);
            baddie->fsa.unk33A = FALSE;
            self->opacity = OBJECT_OPACITY_MAX;
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
            baddie->unk3B2 |= 0x100;
        }
        return;
    }

    if (self->unkE0 == 0) {
        self->srt.transl.x = objSetup->base.x;
        self->srt.transl.y = objSetup->base.y;
        self->srt.transl.z = objSetup->base.z;
        gDLL_3_Animation->vtbl->start_obj_sequence(objSetup->unk2E, self, -1);
        self->unkE0 = 1;
        return;
    }
    
    if (baddie->unk3B2 & 2) {
        gDLL_33_BaddieControl->vtbl->func9(self, &baddie->fsa, &baddie->unk34C, baddie->unk39E, (s8*)&baddie->unk3B4, 4, 0, 0, 1);
        baddie->unk3B2 &= ~2;
    }
    
    if (gDLL_33_BaddieControl->vtbl->func11(self, baddie, 1)) {
        //@recomp: don't continue until the WaterBaddie's curves are found
        {
            objData = baddie->objdata;
            if (objData->curveValue < 0) {
#ifdef DEBUG_WATERBADDIE_CURVES
                diPrintf("ERROR: WaterBaddie %x couldn't find curve! %d\n", self->setup->uID, objData->curveValue);
#endif

                //Keep trying to find the curve at intervals
                if (objData->curveSearchTimer == 0) {
                    gDLL_26_Curves->vtbl->func_4288(baddie->unk3F8, self, 400.0f, dCurveTypes, -1);
                    objData->curveValue = baddie->unk3F8->unk0.unk10;

                    objData->curveSearchTimer = 30;
                } else {
                    objData->curveSearchTimer -= gUpdateRate;
                    if (objData->curveSearchTimer < 0) {
                        objData->curveSearchTimer = 0;
                    }
                }

                //Return if the curve still wasn't found
                if (objData->curveValue < 0) {
                    return;
                }
            }
        }

        WaterBaddie_tick(self, baddie, &baddie->fsa);
        if ((baddie->fsa.target != NULL) && (baddie->fsa.hitpoints != 0)) {
            WaterBaddie_engageTarget(self, 0, baddie, &baddie->fsa);
        } else {
            WaterBaddie_searchForTarget(self, baddie, &baddie->fsa);
        }
    }
}

/* Fix a bug where the WaterBaddies ignored the ground in shallow water (like near the entrance to Discovery Falls) */
RECOMP_PATCH void WaterBaddie_obj_Update(Object* self) {
    gDLL_18_objfsa->vtbl->func2(self, self->data, sAnimStateCallbacks);
    
    //@recomp: don't disappear under the ground
    {
        Baddie* baddie = self->data;
        WaterBaddie_DataActual* objData = baddie->objdata;

        //Don't allow going lower than ground height
        if (objData && self->srt.transl.y < objData->groundHeight) {
            self->srt.transl.y = objData->groundHeight;

            //Bounce off the ground
            if (objData->pitchAcceleration > 0.0f) {
                objData->pitchAcceleration *= -0.5f;
            }
        }
    }
}

/* Use objData instead of a static for the stun timer, so it only affects this WaterBaddie */
RECOMP_PATCH s32 WaterBaddie_logicState0Hit(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
// /*40*/ static f32 sStunnedTimer; //@bug?: won't this affect all WaterBaddies when there are multiple of them?
    
    //@recomp: use objData instead of a static for sStunnedTimer
    Baddie* baddie = self->data;
    WaterBaddie_DataActual* objData = baddie->objdata;

    if (fsa->enteredLogicState) {
        objData->stunnedTimer = 0.0f;
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, WaterBaddie_ASTATE_2_Hit);
    }
    
    if (fsa->hitpoints <= 0) {
        return FSA_NEXTSTATE_SYNC(WaterBaddie_LSTATE_1_Dying);
    }
    
    //Drift straight ahead for about 3 seconds
    if (objData->stunnedTimer > 200.0f) {
        return FSA_NEXTSTATE_SYNC(WaterBaddie_LSTATE_4_Top);
    } else {
        objData->stunnedTimer += gUpdateRateF;
    }
    
    return 0;
}

/* Use objData instead of a static for sCurveValue, so it only affects this WaterBaddie */
RECOMP_PATCH s32 WaterBaddie_logicState3Swimming(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
// /*0x44*/ static s32 sCurveValue; //@bug?: won't this affect all WaterBaddies when there are multiple of them?
    
    Baddie* baddie;
    CurvesStruct* sp3C;
    UnkCurvesStruct* curves;
    f32 curveDelta;
    f32 dx;
    f32 dz;
    f32 speed;
    f32 turnSpeed;
    f32 magnitude;
    f32 temp;
    WaterBaddie_DataActual* objData;

    baddie = self->data;
    gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, 1);
    curves = baddie->unk3F8;
    objData = baddie->objdata;
    
    dx = curves->unk0.unk68.x - self->srt.transl.x;
    dz = curves->unk0.unk68.z - self->srt.transl.z;
    curveDelta = 10.0f / sqrtf(SQ(dx) + SQ(dz));
    
    if (self->animProgress > 0.01f) {
        self->animProgress -= 0.01f;
    } else {
        self->animProgress = 0.0f;
    }

    //@recomp: use objData instead of a static for sCurveValue
    if ((curves_func_800053B0(&curves->unk0, curveDelta) || objData->curveValue != curves->unk0.unk10) && 
        gDLL_26_Curves->vtbl->func_4704(curves) && 
        gDLL_26_Curves->vtbl->func_4288(baddie->unk3F8, self, 400.0f, dCurveTypes, -1)
    ) {
        baddie->unk3B2 &= ~8;
    }
    objData->curveValue = curves->unk0.unk10;

    //Do a bobbing dive through the water occasionally
    if ((fsa->unk278 > 0.15f) && (objData->diveAmount == 0)) {
        if (objData->pitchSpeed < 0.0f) {
            magnitude = -objData->pitchSpeed;
        } else {
            magnitude = objData->pitchSpeed;
        }

        if (magnitude < 0.015f) {
            if (objData->waterHeight < self->srt.transl.y) {
                magnitude = -(objData->waterHeight - self->srt.transl.y);
            } else {
                magnitude = objData->waterHeight - self->srt.transl.y;
            }

            if ((magnitude < 1.0f) && mathRnd(0, 100)) {
                objData->diveAmount = mathRnd(25, 75);
            }
        }
    }

    if (objData->diveAmount) {
        if ((objData->customFlags & WaterBaddie_CUSTOMFLAG_1_Shallow_Water && objData->pitchAcceleration > 0.15f) || //@recomp: handle dives in shallow water
            objData->pitchAcceleration > 0.4f
        ) {
            if (mathRnd(0, 1) != 0) {
                objData->pitchAcceleration = 0.0f;
            }
            objData->diveAmount = 0;
            dll_amSfx->Play(self, dBaddieSounds[mathRnd(0, 1)], MAX_VOLUME, NULL, NULL, 0, NULL);
        } else {
            objData->pitchAcceleration += 0.0001f * objData->diveAmount;
        }
    } else if (objData->pitchAcceleration > 0.0f) {
        objData->pitchAcceleration /= 1.04f;
    }
    
    gDLL_18_objfsa->vtbl->func6(self, fsa, curves->unk0.unk68.x, curves->unk0.unk68.z, 0, 0, 60.0f);

    turnSpeed = objData->turnSpeed;
    if (objData->turnSpeed) {
        temp = objData->moveSpeed / turnSpeed;
        speed = temp;
    } else {
        speed = objData->moveSpeed;
    }
    
    fsa->xAnalogInput *= speed;
    fsa->yAnalogInput *= speed;

    if (self->animProgress > 0.01f) {
        self->animProgress -= 0.01f;
    }
    
    return 0;
}

/* Extend objData */
RECOMP_PATCH u32 WaterBaddie_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(Baddie) + sizeof(WaterBaddie_DataActual);
}
