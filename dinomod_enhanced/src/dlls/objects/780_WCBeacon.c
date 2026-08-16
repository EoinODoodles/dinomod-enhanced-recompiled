#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/338_LFXEmitter.h"
#include "game/gamebits.h"
#include "game/objects/object_id.h"
#include "game/objects/object.h"
#include "sys/gfx/modgfx.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/objhits.h"
#include "sys/print.h"
#include "sys/rand.h"

#include "objects/780_WCBeacon.h"

#include "recomp/dlls/objects/780_WCBeacon_recomp.h"

//TEMPORARY DEFINES
#define WCBeacon_obj_Setup dll_780_setup
#define WCBeacon_obj_Control dll_780_control
#define WCBeacon_obj_Free dll_780_free
#define WCBeacon_obj_GetDataSize dll_780_get_data_size

#define PARTICLE_1A3 0x1A3
//END OF TEMPORARY DEFINES

typedef struct {
/*0*/ f32 timer;
/*4*/ u8 state;
/* RECOMP */
    u32 soundHandleA;
    u32 soundHandleB;
    Object* lfxEmitter;
    f32 sinYawScaled;
    f32 cosYawScaled;
} WCBeacon_Data;

typedef enum {
    WCBEACON_STATE_0_Underground = 0,
    WCBEACON_STATE_1_Unlit = 1,
    WCBEACON_STATE_2_Lighting_Up = 2,
    WCBEACON_STATE_3_Lit = 3
} WCBeacon_States;

#define MAX_FRAME 0x100

/* Ensures the beacon's WCSlabDoor gamebit is set when the beacon's lit gamebit is set, 
   and (during setup) ensures the beacon's pressure switch's gamebit isn't set if the beacon is still underground. */
static void WCBeacon_restoreStateViaGamebits(Object* self) {
    WCBeacon_Setup* objSetup = (WCBeacon_Setup*)self->setup;
    WCBeacon_Data* objData = self->data;

    if (mainGetBits(objSetup->gamebitLit)) {
        objData->state = WCBEACON_STATE_3_Lit;

        //Ensure WCSlabDoor symbol is lit
        if (objSetup->gamebitSlab > NO_GAMEBIT + 1) {
            mainSetBits(objSetup->gamebitSlab, TRUE);
        }
    } else {
        objData->state = WCBEACON_STATE_0_Underground;

        if (objSetup->gamebitSwitch > NO_GAMEBIT + 1) {
            mainSetBits(objSetup->gamebitSwitch, FALSE);
        }
        if (objSetup->gamebitSlab > NO_GAMEBIT + 1) {
            mainSetBits(objSetup->gamebitSlab, FALSE);
        }

        mainSetBits(objSetup->gamebitRise, FALSE);
    }
}

/* 
 * - Fix bugs in restoring state on revisit.
 * - Configure objHits so the the beacon can detect damage sources more than once. 
 * - Store some trigonometry calcs for placing the LFXEmitter.
 */
RECOMP_PATCH void WCBeacon_obj_Setup(Object* self, WCBeacon_Setup* objSetup, s32 reset) {
    WCBeacon_Data* objData = self->data;
    TextureAnimator* animatedTexture;
    
    self->srt.yaw = objSetup->yaw << 8;
    
    //Set model
    self->modelInstIdx = objSetup->modelIndex;
    if (self->modelInstIdx >= self->def->numModels) {
        self->modelInstIdx = 0;
    }

    //Restore state (@recomp: reworked)
    WCBeacon_restoreStateViaGamebits(self);

    //Restore texture frame (@recomp: any state from 2 onwards)
    animatedTexture = objExprGetTexAnimator(self, 0, 0);
    if ((animatedTexture != NULL) && (objData->state >= WCBEACON_STATE_2_Lighting_Up)) {
        animatedTexture->frame = MAX_FRAME;
    }

    //@recomp: set position
    if (objData->state > WCBEACON_STATE_0_Underground) {
        self->srt.transl.y = objSetup->base.y + 42;
        self->unkDC = 1; //Don't use objSeq preempt
    }

    //@recomp: increase size of hitbox for detecting fire damage
    if (self->objhitInfo != NULL) {
        self->objhitInfo->unk52 *= 3;
        self->objhitInfo->unk56 *= 5;
        self->objhitInfo->unk54 *= 3;
    }

    //@recomp: make sure the beacon can check for damage after the first time it detects damage 
    //(since we're now discarding non-fire damage)
    if (self->objhitInfo) {
        self->objhitInfo->unk58 &= ~0x80;
    }

    //@recomp: store calcs for LFXEmitter's position
    if (self->srt.yaw) {
        objData->sinYawScaled = Sinf(self->srt.yaw);
        objData->cosYawScaled = Cosf(self->srt.yaw);
    } else {
        objData->sinYawScaled = 0;
        objData->cosYawScaled = 1;
    }
    objData->sinYawScaled *= 30;
    objData->cosYawScaled *= 30;
}

/* Creates a warm LFXEmitter in front of the beacon, for use while it's lit */
static Object* WCBeacon_createLight(Object* self) {
    static s16 rsLfxIndices[] = { 547, 550, 553 };
    WCBeacon_Data* objData;
    LFXEmitter_Setup* setup;

    objData = self->data;
    if (objData == NULL) {
        return NULL;
    }

    setup = (LFXEmitter_Setup*)objAllocSetup(sizeof(LFXEmitter_Setup), OBJ_LFXEmitter);
    setup->base.loadFlags = OBJSETUP_LOAD_MANUAL;
    setup->base.fadeFlags = OBJSETUP_FADE_MANUAL;
    setup->base.loadDistance = 0xFF;
    setup->base.fadeDistance = 0xFF;
    setup->base.x = self->srt.transl.x - objData->sinYawScaled;
    setup->base.y = self->srt.transl.y + 15;
    setup->base.z = self->srt.transl.z - objData->cosYawScaled;
    setup->unk20 = 0;
    setup->unk1E = rsLfxIndices[0];
    setup->unk22 = -1;
    setup->unk18 = 0;
    setup->unk1A = 0;
    setup->unk1C = 0;
    setup->unk24 = 1;
    setup->unk25 = 50;
    
    return objSetupObject(&setup->base, OBJINIT_STANDALONE | OBJINIT_FLAG4, self->mapID, -1, NULL);
}

/* 
 * - Ensure that the beacon is only lit by fire, and not just any kind of damage.
 * - Use soundHandles for restoring/managing the fire sound loops.
 * - Create light/fire effects.
 */
RECOMP_PATCH void WCBeacon_obj_Control(Object* self) {
    WCBeacon_Setup* objSetup;
    WCBeacon_Data* objData;
    TextureAnimator* texAnim;
    s16 frameBlend;
    /* RECOMP */
    s32 damageType;
    Object* player;

    objData = self->data;
    objSetup = (WCBeacon_Setup*)self->setup;

    damageType = func_80025F40(self, NULL, NULL, NULL);

    //@recomp: ensure state is synced with lit gamebit (when it's set)
    {
        if ((objData->state != WCBEACON_STATE_3_Lit) && mainGetBits(objSetup->gamebitLit)) {
            WCBeacon_restoreStateViaGamebits(self);
            
            self->srt.transl.y = objSetup->base.y + 42;
            self->unkDC = 1; //Don't use objSeq preempt
            return;
        }
    }

    //@recomp: Handle sounds and effects
    if (objData->state >= WCBEACON_STATE_2_Lighting_Up) {
        //Burn loop A
        if (objData->soundHandleA == 0) {
            objData->soundHandleA = dll_amSfx->Play(self, SOUND_50a_Fire_Burning_Low_Loop, MAX_VOLUME, NULL, NULL, 0, NULL);
        }

        //Burn loop B
        if (objData->soundHandleB == 0) {
            objData->soundHandleB = dll_amSfx->Play(self, SOUND_50b_Fire_Burning_High_Loop, MAX_VOLUME, NULL, NULL, 0, NULL);
        }

        //LFXEmitter
        if (objData->lfxEmitter == NULL) {
            player = objGetPlayer();
            if (player && vec3DistanceSquared(&self->globalPosition, &player->globalPosition) < SQ(100)) {
                objData->lfxEmitter = WCBeacon_createLight(self);
            }
        } else {
            objData->lfxEmitter->srt.transl.x = self->srt.transl.x - objData->sinYawScaled;
            objData->lfxEmitter->srt.transl.y = self->srt.transl.y + 15;
            objData->lfxEmitter->srt.transl.z = self->srt.transl.z - objData->cosYawScaled;

            player = objGetPlayer();
            if (player && vec3DistanceSquared(&self->globalPosition, &player->globalPosition) >= SQ(100)) {
                objFreeObject(objData->lfxEmitter);
                objData->lfxEmitter = NULL;
            } else {
                //Create ember particles when player nearby too
                gDLL_17_partfx->vtbl->spawn(self, PARTICLE_1A3, NULL, 0, -1, NULL);
            }
        }
    } else {
        //Burn loop A
        if (objData->soundHandleA) {
            dll_amSfx->Stop(objData->soundHandleA);
            objData->soundHandleA = 0;
        }

        //Burn loop B
        if (objData->soundHandleB) {
            dll_amSfx->Stop(objData->soundHandleB);
            objData->soundHandleB = 0;
        }

        //LFXEmitter
        if (objData->lfxEmitter) {
            objFreeObject(objData->lfxEmitter);
            objData->lfxEmitter = NULL;
        }
    }

    switch (objData->state) {
    case WCBEACON_STATE_1_Unlit:
        //Descend if the pressure switch gamebit unsets (player didn't light beacon in time)
        if (mainGetBits(objSetup->gamebitRise) == FALSE) {
            gDLL_3_Animation->vtbl->start_obj_sequence(1, self, -1);
            objData->state = WCBEACON_STATE_0_Underground;
        }

        //Become lit when hit (@recomp: only fire-type damage lights the beacon, instead of any attack)
        switch (damageType) {
        case Damage_Type_Projectile:
        case Damage_Type_Flame_Command:
        case Damage_Type_Explosion:
            //@recomp: make sure the player's not way below the beacon (shooting from near the pressure switch)
            player = objGetPlayer();
            if (player && (self->globalPosition.y - 50.0f > player->globalPosition.y)) {
                break;
            }

            //@recomp: move sound plays elsewhere, so they also play when restoring lit state on revisit
            objData->state = WCBEACON_STATE_2_Lighting_Up;
            objData->timer = 0.0f;
            break;
        }

        break;
    case WCBEACON_STATE_0_Underground:
        //Rise up when the beacon's corresponding pressure switch is activated
        if (mainGetBits(objSetup->gamebitRise)) {
            //@recomp: exclude Tricky from the sequence, since he's not really animated but can prevent it from playing if he's unloaded
            gDLL_3_Animation->vtbl->start_obj_sequence(0, self, ~(1 << 4));
            objData->state = WCBEACON_STATE_1_Unlit;
        }
        break;
    case WCBEACON_STATE_2_Lighting_Up:
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_73A, NULL, PARTFXFLAG_2, -1, NULL);
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_73B, NULL, PARTFXFLAG_2, -1, NULL);

        //Count up until advancing state
        objData->timer += gUpdateRateF;
        if (objData->timer >= 90.0f) {
            objData->state = WCBEACON_STATE_3_Lit;
        }

        //Fade in the lit texture frame
        {
            frameBlend = (objData->timer / 45.0f) * MAX_FRAME;
            if (frameBlend > MAX_FRAME) {
                frameBlend = MAX_FRAME;
            }

            texAnim = objExprGetTexAnimator(self, 0, 0);
            if (texAnim != NULL) {
                texAnim->frame = frameBlend;
            }
        }
        break;
    case WCBEACON_STATE_3_Lit:
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_73A, NULL, PARTFXFLAG_2, -1, NULL);
        mainSetBits(objSetup->gamebitLit, TRUE);

        texAnim = objExprGetTexAnimator(self, 0, 0);
        if (texAnim != NULL) {
            texAnim->frame = MAX_FRAME;
        }

        if (self->unkDC == 0) {
            gDLL_3_Animation->vtbl->preempt_sequence_time(self, 105);
            gDLL_3_Animation->vtbl->start_obj_sequence(0, self, 1);
        }
        break;
    }

    self->unkDC = 1;
}

/* Free soundHandles and LFXEmitter */
RECOMP_PATCH void WCBeacon_obj_Free(Object* self, s32 onlySelf) { 
    WCBeacon_Data* objData = self->data;
    Object* player;

    if (objData->soundHandleA) {
        dll_amSfx->Stop(objData->soundHandleA);
        objData->soundHandleA = 0;
    }

    if (objData->soundHandleB) {
        dll_amSfx->Stop(objData->soundHandleB);
        objData->soundHandleB = 0;
    }

    if (objData->lfxEmitter) {
        objFreeObject(objData->lfxEmitter);
        objData->lfxEmitter = NULL;
    }
}

/* Extend data struct */
RECOMP_PATCH u32 WCBeacon_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(WCBeacon_Data);
}
