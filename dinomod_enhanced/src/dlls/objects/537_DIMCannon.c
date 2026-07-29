#include "configs.h"
#include "game/gamebits.h"
#include "macros.h"
#include "math_util.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/210_player.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object_id.h"
#include "game/objects/object.h"
#include "sys/gfx/modgfx.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "sys/rand.h"

#include "objects/223_CannonClaw.h"

#include "recomp/dlls/objects/537_DIMCannon_recomp.h"

// #define CANNON_DEBUG

/* 
    TODO:

    - Don't draw the cannon, base, or CannonClaw when they're inside the silo and the silo door is closed?
*/

#define CANNON_CLAW_SHOT_ANTICIPATE 70.0f
#define CANNON_STOP_SFX_MIN_MOVE 200

//TEMPORARY DEFINES
#define DIMCannon_obj_Setup dll_537_setup
#define DIMCannon_obj_Control dll_537_control
#define DIMCannon_obj_Print dll_537_print
#define DIMCannon_obj_Free dll_537_free
#define DIMCannon_obj_GetDataSize dll_537_get_data_size
#define DIMCannon_animCallback dll_537_func_A94
#define DIMCannon_aimCannonClaw dll_537_func_DAC
#define DIMCannon_fireWhenReady dll_537_func_1150
#define DIMCannon_setupCannonBall dll_537_func_1314
#define DIMCannon_tickCannonBall dll_537_func_1430
#define DIMCannon_freeCannonBall dll_537_func_1640
#define DIMCannon_createCannonBallExplosion dll_537_func_16AC

#define dModGfxDLL _data_0

#define SOUND_1D4_Metal_Ratcheting_Loop 0x1D4
#define SOUND_1D5_Metal_Squeak 0x1D5
#define SOUND_125_Metal_Clunk 0x125
#define SOUND_12C_Metal_Unclunk 0x12C
#define SOUND_780_Hatch_Opening 0x780
#define SOUND_809_Mechanical_Ratcheting_Loop 0x809

#define M_1_DEGREE    0xB6
//END OF TEMPORARY DEFINES

typedef struct {
/*00*/  ObjSetup base;
/*18*/  s16 gamebitSiloCoverOpen;
/*1A*/  s16 gamebitCannonClawDead;
/*1C*/  s16 gamebitCannonClawAboard;
/*1E*/  s16 gamebitCannonClawTruce;
/*20*/  s16 gamebitSiloEnter;
/*22*/  s16 gamebitSiloExit;
/*24*/  s16 unk24;
/*26*/  s16 hostileRange;
/*28*/  s8 yaw;
/*29*/  u8 cooldownMin;
/*2A*/  u8 cooldownMax;
/*2B*/  u8 rangeSiloRetreat;
} DIMCannon_Setup;

typedef struct {
    Object* targetObj;
    Vec3f targetCoords;
    f32 targetDistSq;
    Vec3f muzzleCoords;
    s16 fireCooldown;
    s16 enemyAimCooldown;
    u8 state;
    u8 fire;
    u8 distracted;
    s8 interactLockTimer;
    /* RECOMP */
    u32 soundHandleMove;
    u32 soundHandleAim;
    u32 soundHandleSilo;
    s32 aimFxPhase;
    f32 prevStickXStrength;
    f32 prevY;
    s16 prevYaw;
    s16 moveTimer;
    u8 prevState;
    u8 prevDistracted;
    u8 hatchStoppedTimer;
    Object* hatch;
    f32 prevHatchDistance;
} DIMCannon_Data;

typedef enum {
    DIMCannon_STATE_1_Controlled_by_CannonClaw_Aim_Hostile = 1, //CannonClaw aims and fires - sets silo-hiding gamebit when a target approaches
    DIMCannon_STATE_2_Controlled_by_CannonClaw_Idle,            //Unused state! No aiming - just sets silo-hiding gamebit when a target approaches
    DIMCannon_STATE_3_Controlled_by_Player,                     //The player has mounted the cannon turret
    DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile,  //CannonClaw aims but doesn't fire - DOESN'T retreat into silo when a target approaches
    DIMCannon_STATE_5_Idle,                                     //No CannonClaw, player can interact and mount the cannon turret
    DIMCannon_STATE_6_Retreated_into_Silo                       //Sets a silo-exiting gamebit when the target backs off
} DIMCannon_States;

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 unk19;
    s16 velocityY;
    s16 velocityX;
    s16 unk1E;
    s16 unk20;
    s16 unk22;
} DIMCannonBall_Setup;

typedef struct {
    u8 createModGfx;
} DIMCannonBall_Data;

typedef struct {
    ObjSetup base;
    s16 unk18;
    s16 unk1A;
    s16 unk1C;
    s16 unk1E;
    s16 unk20;
    s16 unk22;
} DIMExplosion_Setup;

extern int DIMCannon_animCallback(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue);
extern void DIMCannon_fireWhenReady(Object* self);
extern void DIMCannon_freeCannonBall(Object* self);
extern void DIMCannon_tickCannonBall(Object* self);
extern void DIMCannon_aimCannonClaw(Object* self, f32 x, f32 y, f32 z, f32 targetDist);
extern void DIMCannon_setupCannonBall(Object* self, DIMCannonBall_Setup* objSetup);

extern DLL_IModgfx* dModGfxDLL;

#ifdef CANNON_DEBUG

static char dStateNames[][100] = {
    "Nothing",
    "Controlled_by_CannonClaw_Aim_Hostile",
    "Controlled_by_CannonClaw_Idle",
    "Controlled_by_Player",
    "Controlled_by_CannonClaw_Aim_Nonhostile",
    "Idle",
    "Retreated_into_Silo"
};

static void printGamebit(s16 gamebitID, char* name) {
    if (gamebitID == NO_GAMEBIT) {
        if (name) {
            diPrintf("%s (NO_GAMEBIT): N/A\n", name);
        } else {
            diPrintf("NO_GAMEBIT: N/A\n");
        }
    } else {
        if (name) {
            diPrintf("%s (%x): %d\n", name, gamebitID, mainGetBits(gamebitID));
        } else {
            diPrintf("BIT_%x: %d\n", gamebitID, mainGetBits(gamebitID));
        }
    }
}

static void printSoundHandles(Object* self) {
    DIMCannon_Data* objData = self->data;

    diPrintf("soundHandleAim: %d\n", objData->soundHandleAim);
    diPrintf("soundHandleMove: %d\n", objData->soundHandleMove);
    diPrintf("soundHandleSilo: %d\n", objData->soundHandleSilo);
}

static void debugCannon(Object* self) {
    DIMCannon_Data* objData = self->data;
    DIMCannon_Setup* objSetup = (DIMCannon_Setup*)self->setup;

    //Ignore DIMCannonBall
    if (self->id != OBJ_DIMCannon) {
        return;
    }

    diPrintf("DIMCannon state: %d\n\t(%s) [%s]\n", 
        objData->state, dStateNames[objData->state], 
        self->stateFlags & OBJSTATE_IN_SEQ ? "animCallback" : "Control"
    );

    diPrintf("fireCooldown: %d\n", objData->fireCooldown);

    printGamebit(objSetup->gamebitSiloCoverOpen, "gamebitSiloCoverOpen");

    // printSoundHandles(self);
}

RECOMP_HOOK_DLL(DIMCannon_obj_Control) void printCannonState(Object* self) {
    debugCannon(self);
} 
RECOMP_HOOK_DLL(DIMCannon_animCallback) void printCannonStateAnimCallback(Object* self) {
    debugCannon(self);
} 
#endif

/* Checks whether a cannon fire sound should be used */
static s32 DIMCannon_configUseBasicSounds(void) {
    if (recomp_get_config_u32(DIMCannonSounds_Config) >= DIM_CANNON_SFX_ON_BASIC) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* Checks whether extra cannon sound design should be used (machinery sounds, CannonClaw voices, etc.) */
static s32 DIMCannon_configUseFancySounds(void) {
    if (recomp_get_config_u32(DIMCannonSounds_Config) == DIM_CANNON_SFX_ON_FULL) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* Attempts to find any nearby DIMCannonCover object, and stores a reference to it. */
static void DIMCannon_findHatchCover(Object* self, f32 maxDistance) {
    DIMCannon_Data* objData;
    Object** objects;
    s32 count;
    s32 i;

    objData = self->data;

    maxDistance = SQ(maxDistance);

    //Try to find a nearby cannon cover
    for (objects = objGetObjects(&i, &count); i < count; i++) {
        switch (objects[i]->id) {
        case OBJ_DIMCannonCover1:
        case OBJ_DIMCannonCover2:
        case OBJ_DIMCannonCover3:
        case OBJ_DIMCannonCover4:
            if (vec3DistanceXZSquared(&self->srt.transl, &objects[i]->srt.transl) > maxDistance){
                continue;
            }
            objData->hatch = objects[i];
            break;
        }
    }
}

/* Calculates a simulated stickX value for the cannon's rotation sounds, while it's being operated by a CannonClaw. */
static s32 DIMCannon_simulateCannonClawStickX(Object* self) {
    DIMCannon_Data* objData = self->data;
    s32 stickX;

    //Calculate simulated stickX for the CannonClaw during sequences
    if (objData->prevYaw) {
        stickX = self->srt.yaw - objData->prevYaw;
        CIRCLE_WRAP(stickX);
        if (stickX < 0) {
            stickX = -stickX;
        }

        //Ignore large angular jumps (like at the end/beginning of a sequence)
        if (stickX >= M_45_DEGREES) {
            stickX = 0;                          
        } else {
            if (gUpdateRateF != 0) {
                stickX /= gUpdateRateF;
            }
            stickX >>= 3;
            if (stickX > 70) {
                stickX = 70;
            }
        }
    } else {
        stickX = 0;
    }

    objData->prevYaw = self->srt.yaw;

    return stickX;
}

/* Reverses shot buildup sounds when the CannonClaw can no longer fire (player left firing range, etc.) */
static void DIMCannon_reverseAimSoundBuildup(Object* self) {
    DIMCannon_Data* objData = self->data;

    if (objData->fireCooldown >= CANNON_CLAW_SHOT_ANTICIPATE) {
        return;
    }

    switch (objData->state) {
    case DIMCannon_STATE_6_Retreated_into_Silo:
    case DIMCannon_STATE_2_Controlled_by_CannonClaw_Idle:
    case DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile:
        if (objData->fireCooldown < CANNON_CLAW_SHOT_ANTICIPATE) {
            objData->fireCooldown += gUpdateRate;
        }
    }
}

/* Checks whether the cannon is currently controlled by a CannonClaw */
static _Bool DIMCannon_hasCannonClaw(Object* self) {
    DIMCannon_Data* objData = self->data;
    DIMCannon_Setup* objSetup = (DIMCannon_Setup*)self->data;

    if (mainGetBits(objSetup->gamebitCannonClawDead)) {
        return FALSE;
    } else if (mainGetBits(objSetup->gamebitCannonClawAboard)) {
        return TRUE;
    }

    //Fallback checks, just in case the gamebits are incorrect/contradictory
    switch (objData->state) {
    case DIMCannon_STATE_1_Controlled_by_CannonClaw_Aim_Hostile:
    case DIMCannon_STATE_2_Controlled_by_CannonClaw_Idle:
    case DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile:
    case DIMCannon_STATE_6_Retreated_into_Silo:
        return TRUE;
    }
    return FALSE;
}

/* Checks whether the CannonClaw will fire at the player */
static _Bool DIMCannon_isCannonClawNonhostile(Object* self) {
    DIMCannon_Data* objData = self->data;
    DIMCannon_Setup* objSetup = (DIMCannon_Setup*)self->data;

    if (mainGetBits(objSetup->gamebitCannonClawTruce)) {
        return TRUE;
    }

    if (DIMCannon_hasCannonClaw(self) && (objData->state == 0)) {
        return TRUE;
    }

    return FALSE;
}

/* Free soundHandles */
static void DIMCannon_stopSoundLoops(Object* self) {
    DIMCannon_Data* objData = self->data;

    //Free aim sound loop
    if (objData->soundHandleAim) {
        dll_amSfx->Stop(objData->soundHandleAim);
        objData->soundHandleAim = 0;
    }

    //Free move sound loop
    if (objData->soundHandleMove) {
        dll_amSfx->Stop(objData->soundHandleMove);
        objData->soundHandleMove = 0;
    }

    //Free silo sound loop
    if (objData->soundHandleSilo) {
        dll_amSfx->Stop(objData->soundHandleSilo);
        objData->soundHandleSilo = 0;
    }
}

/* Custom sound design setup for the cannon */
static void DIMCannon_handleSoundsAndFX(Object* self, s32 stickXMagnitude, SeqJoint* barrelJoint) {
    DIMCannon_Data* objData = self->data;
    u8 hasCannonClaw;
    if ((objData == NULL) || (barrelJoint == NULL)) {
        return;
    }

    //Check mod config
    if (DIMCannon_configUseFancySounds() == FALSE) {
        DIMCannon_stopSoundLoops(self);
        return;
    }
    
    //Check whether the CannonClaw is steering
    hasCannonClaw = DIMCannon_hasCannonClaw(self);

    //Turning sounds
    {
        if (stickXMagnitude) {
            if (objData->moveTimer < CANNON_STOP_SFX_MIN_MOVE) {
                objData->moveTimer += gUpdateRate * stickXMagnitude;
                if (objData->moveTimer > CANNON_STOP_SFX_MIN_MOVE) {
                    objData->moveTimer = CANNON_STOP_SFX_MIN_MOVE;
                }
            }

            //@recomp: play sound when moving
            if (objData->soundHandleMove == 0) {
                objData->soundHandleMove = dll_amSfx->Play(self, SOUND_1D4_Metal_Ratcheting_Loop, MAX_VOLUME, NULL, NULL, 0, NULL);
            } else {
                //Adjust pitch/volume with stickX magnitude
                objData->prevStickXStrength = (objData->prevStickXStrength + stickXMagnitude/70.0f) / 2.0f;
                dll_amSfx->SetPitch(objData->soundHandleMove, 0.95f + 0.1f*objData->prevStickXStrength);
                dll_amSfx->SetVol(objData->soundHandleMove, lerp_float(objData->prevStickXStrength, (MAX_VOLUME>>2), MAX_VOLUME));
            }
        } else {
            //@recomp: stop sound loop when stopped
            if (objData->soundHandleMove) {
                dll_amSfx->Stop(objData->soundHandleMove);
                objData->soundHandleMove = 0;
            }

            //@recomp: play sound when stopping (but only if the cannon had moved significantly)
            if ((objData->moveTimer >= CANNON_STOP_SFX_MIN_MOVE) && (objData->prevStickXStrength >= 0.2f)) {
                dll_amSfx->Play(self, SOUND_12C_Metal_Unclunk, lerp_float(objData->prevStickXStrength, 10, 25), NULL, NULL, 0, NULL);
                objData->moveTimer = 0;
            }
        }

        //Handle sounds when exiting the cannon
        if ((objData->state == DIMCannon_STATE_5_Idle) && objData->soundHandleMove) {
            //Free move sound loop
            dll_amSfx->Stop(objData->soundHandleMove);
            objData->soundHandleMove = 0;

            //Play stopping sound
            dll_amSfx->Play(self, SOUND_125_Metal_Clunk, lerp_float(objData->prevStickXStrength, (MAX_VOLUME>>4), (MAX_VOLUME>>2)), NULL, NULL, 0, NULL);
        }
    }

    //Sounds/effects while aiming
    {
        s32 angle = -barrelJoint->pitch;

        if ((!hasCannonClaw && (angle > 10)) ||                          //For player: only play sound when aiming barrel
            (hasCannonClaw && !DIMCannon_isCannonClawNonhostile(self))   //For CannonClaw: play at least a faint rumble (unless nonhostile)
        ) { 
            f32 tValueAim;

            //Handle pitch differently depending on who's controlling the cannon
            if (hasCannonClaw) {                
                tValueAim = (CANNON_CLAW_SHOT_ANTICIPATE - objData->fireCooldown)/CANNON_CLAW_SHOT_ANTICIPATE;
                
                if (tValueAim > 1.0f) {
                    tValueAim = 1.0f;
                } else if (tValueAim < 0) {
                    tValueAim = 0.0f;
                }
            } else {
                tValueAim = (f32)angle/M_45_DEGREES;
            }

            if (((hasCannonClaw == FALSE) && (joyGetButtons(0) & A_BUTTON)) || ((hasCannonClaw == TRUE) && tValueAim)) {
                objData->aimFxPhase += gUpdateRate * tValueAim * 0x3000;
                if (objData->aimFxPhase > M_360_DEGREES) {
                    objData->aimFxPhase -= M_360_DEGREES;
                }
            }

#ifdef CANNON_DEBUG
            diPrintf("tValueAim: %f\n", &tValueAim);
#endif

            //Start sound loop
            if (objData->soundHandleAim == 0) {
                objData->soundHandleAim = dll_amSfx->Play(self, SOUND_1D4_Metal_Ratcheting_Loop, MAX_VOLUME, NULL, NULL, 0, NULL);
            } else {
                //Adjust pitch
                dll_amSfx->SetPitch(objData->soundHandleAim, lerp_float(tValueAim, 0.3f, 1.5f));
            }
            
            //Shake barrel seqJoint
            if (objData->aimFxPhase) {
                barrelJoint->translateX = mathSinfInterp(objData->aimFxPhase) * tValueAim * 20;
            } else {
                barrelJoint->translateX = 0;
            }
        } else {
            //Stop sound loop
            if (objData->soundHandleAim) {
                dll_amSfx->Stop(objData->soundHandleAim);
                objData->soundHandleAim = 0;
            }

            //Barrel vibration decays while not firing
            if (objData->aimFxPhase > 0) {
                objData->aimFxPhase -= gUpdateRate;
                if (objData->aimFxPhase < 0) {
                    objData->aimFxPhase = 0;
                }
            }

            barrelJoint->translateX = 0;
        }

        //Stop shaking when firing
        if (objData->fire) {
            objData->aimFxPhase = 0;
        }
    }

    //Play a sound when the hatch moves
    {
        if (objData->hatchStoppedTimer == 0) {
            if (objData->hatch) {
                f32 distance = vec3DistanceXZSquared((Vec3f*)&self->setup->x, &objData->hatch->srt.transl);
                
                if (objData->prevHatchDistance && (objData->prevHatchDistance != distance)) {
                    dll_amSfx->Play(self, SOUND_780_Hatch_Opening, MAX_VOLUME, NULL, NULL, 0, NULL);
#ifdef CANNON_DEBUG
                    recomp_printf("Hatch sound started!\n");
#endif
                    objData->hatchStoppedTimer = 1;
                }

                objData->prevHatchDistance = distance;
            }
        } else {
            objData->hatchStoppedTimer += gUpdateRate;
            if (objData->hatchStoppedTimer > 180) {
                objData->hatchStoppedTimer = 0;
                objData->prevHatchDistance = 0;
#ifdef CANNON_DEBUG
                recomp_printf("Hatch sound finished.\n");
#endif
            }
        }
    }

    //Handle sounds when entering/exiting silo
    {
        //Play voice line when retreating (by messaging the CannonClaw)
        if (hasCannonClaw &&
            (objData->prevState != DIMCannon_STATE_6_Retreated_into_Silo) &&
            (objData->state == DIMCannon_STATE_6_Retreated_into_Silo)
        ) {
            objSendMesgManyNearby(OBJ_CannonClaw, 40.0f, 0, self, CannonClaw_FLAG_4_Entered_Silo, 0);
        }

        //Play voice line when re-emerging (by messaging the CannonClaw)
        if (hasCannonClaw &&
            (objData->prevState == DIMCannon_STATE_6_Retreated_into_Silo) &&
            (objData->state != DIMCannon_STATE_6_Retreated_into_Silo)
        ) {
            objSendMesgManyNearby(OBJ_CannonClaw, 40.0f, 0, self, CannonClaw_FLAG_8_Exited_Silo, 0);
        }

        //Play a mechanical sound when the cannon moves up/down
        {
            int stopSoundLoop = TRUE;
            if (objData->state != DIMCannon_STATE_3_Controlled_by_Player) {
                if ((self->stateFlags & OBJSTATE_IN_SEQ) && (objData->prevY != self->srt.transl.y)) {
                    f32 yDiff = self->srt.transl.y - objData->prevY;
                    if (yDiff < 0) {
                        yDiff = -yDiff;
                    }
                    
                    //Play ratcheting sound loop
                    if ((yDiff != 0.0f) && 
                        (yDiff < 5.5f) //ignore large jumps, like when DIMSeqObject preempts the cannon into position
                    ) {
                        stopSoundLoop = FALSE;

                        if (objData->soundHandleSilo == 0) {
#ifdef CANNON_DEBUG
                            recomp_printf("Silo retreat sound playing!\n");
#endif
                            objData->soundHandleSilo = dll_amSfx->Play(self, SOUND_809_Mechanical_Ratcheting_Loop, MAX_VOLUME, NULL, NULL, 0, NULL);
                        }
                    }

                    //Adjust volume/pitch
                    if (objData->soundHandleSilo) {
                        f32 tValueSpeed = yDiff;
                        if (gUpdateRateF != 0.0f) {
                            tValueSpeed /= gUpdateRateF;
                        }
                        tValueSpeed /= 5.0f;
                        if (tValueSpeed > 1.0f) {
                            tValueSpeed = 1.0f;
                        } else if (tValueSpeed < 0.0f) {
                            tValueSpeed = 0.0f;
                        }

                        dll_amSfx->SetPitch(objData->soundHandleSilo, 0.8f + 0.4f*tValueSpeed);
                        dll_amSfx->SetVol(objData->soundHandleSilo, lerp_float(tValueSpeed, 0, MAX_VOLUME));
                    }
                }

                objData->prevY = self->srt.transl.y;
            }
            
            if (stopSoundLoop && (objData->soundHandleSilo)) {
#ifdef CANNON_DEBUG
                recomp_printf("Silo retreat sound stopped.\n");
#endif
                dll_amSfx->Stop(objData->soundHandleSilo);
                objData->soundHandleSilo = 0;
            }
        }
    }

    //Play a voice line when distracted
    {
        //Message the CannonClaw to play a voice clip
        if (hasCannonClaw && (objData->prevDistracted == FALSE) && (objData->distracted == TRUE)) {
            objSendMesgManyNearby(OBJ_CannonClaw, 40.0f, 0, self, CannonClaw_FLAG_10_Distracted, 0);
        }
    }
}

/* Hatch-related fixes, and opening a message queue (so the CannonClaw can react to successful shots). */
RECOMP_PATCH void DIMCannon_obj_Setup(Object* self, DIMCannon_Setup* objSetup, s32 reset) {
    DIMCannon_Data* objData;

    objInitMesgQueue(self, 4);

    if (self->id == OBJ_DIMCannonBall) {
        //Cannonball
        DIMCannon_setupCannonBall(self, (DIMCannonBall_Setup*)objSetup);
    } else {
        //Cannon

        objSetPriority(self, OBJPRIORITY_MOBILE_MAP);
        objData = self->data;

        self->unkAF |= ARROW_FLAG_8_No_Targetting;
        self->animCallback = DIMCannon_animCallback;
        self->srt.yaw = objSetup->yaw << 8;

        dModGfxDLL = dllLoad(DLL_ID_137, 1);
        
        if (mainGetBits(objSetup->gamebitCannonClawDead)) {
            objData->interactLockTimer = 60;
            objData->state = DIMCannon_STATE_5_Idle;
        } else if (mainGetBits(objSetup->gamebitCannonClawAboard)) {
            objData->state = DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile;

            //@recomp: open the hatch, since the CannonClaw's emerged from it
            if (objSetup->gamebitSiloCoverOpen != NO_GAMEBIT) {
                mainSetBits(objSetup->gamebitSiloCoverOpen, TRUE);
            }

            //@recomp: listen for messages
            objInitMesgQueue(self, 1);
        }

        //@recomp: store Y
        objData->prevY = self->srt.transl.y;

        //@recomp: find hatch
        DIMCannon_findHatchCover(self, 100.0f);
    }

    objAddObjectType(self, OBJTYPE_Baddie);
    self->stateFlags |= OBJSTATE_UPDATE_DISABLED;
}

/**  
  * Fix the LockIcon appearing when the cannon can't be interacted with yet.
  * Block A_BUTTON when interacting, so the player doesn't do an attack while mounting the cannon.
  * Reset firing cooldowns on exiting the silo, so CannonClaw can't retain a near-zero cooldown on entering the silo and then snipe the player as they pop up.
  * Ease barrel joint back towards neutral position when idle.
  * Play sounds.
  */
RECOMP_PATCH void DIMCannon_obj_Control(Object* self) {
    DIMCannon_Data* objData;
    DIMCannon_Setup* objSetup;
    Object* player;
    Object* sidekick;
    f32 animSpeed;
    u32 distracted;
    Object* cannonPtr;

    objSetup = (DIMCannon_Setup*)self->setup;
    sidekick = objGetSidekick();
    
    if (self->id == OBJ_DIMCannonBall) {
        DIMCannon_tickCannonBall(self);
        return;
    }
    
    objData = self->data;

    //Handle LockIcon
    if (self->unkAF & ARROW_FLAG_8_No_Targetting) {
        if (
            (objData->state == DIMCannon_STATE_5_Idle) && //@recomp: check whether the player can actually interact with the cannon
            (objData->interactLockTimer <= 0) &&          //@recomp: check whether the player can actually interact with the cannon
            mainGetBits(objSetup->gamebitCannonClawDead)
        ) {
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        }
    }

    //@recomp: store state at beginning of tick
    objData->prevState = objData->state;
    objData->prevDistracted = objData->distracted;

    //@recomp: reverse cooldown when CannonClaw isn't about to fire
    DIMCannon_reverseAimSoundBuildup(self);

    //Handle being distracted
    distracted = FALSE;
    if (sidekick != NULL) {
        objData->distracted = ((DLL_ISidekick*)sidekick->dll)->vtbl->func24(sidekick);
        if (objData->state != DIMCannon_STATE_6_Retreated_into_Silo) {
            ((DLL_ISidekick*)sidekick->dll)->vtbl->enable_command(sidekick, Sidekick_Command_INDEX_2_Distract);
        } else {
            distracted = objData->distracted;
            if (distracted) {
                ((DLL_ISidekick*)sidekick->dll)->vtbl->func21(sidekick, 0, NULL);
                distracted = objData->distracted = FALSE;
            }
        }
    } else {
        objData->distracted = distracted;
    }
    
    //Choose the CannonClaw's target (pick the sidekick when distracted, or else the player - if they're not on a vehicle)
    if ((distracted = objData->distracted)) {
        objData->targetObj = sidekick;
    } else {
        player = objGetPlayer();
        if (((DLL_210_Player*)player->dll)->vtbl->get_vehicle(player)) {
            objData->targetObj = NULL;
        } else {
            objData->targetObj = player;
        }
    }
    
    //Return to idle animation when recoil animation ends
    if ((self->curModAnimId == 1) && (self->animProgress >= 1.0f)) {
        objAnimSet(self, 0, 0, 0);
    }
    
    self->srt.flags &= ~OBJFLAG_INVISIBLE;
    
    switch (objData->state) {
    case DIMCannon_STATE_5_Idle:
        //No gunner, player can mount cannon

        if (objData->interactLockTimer > 0) {
            objData->interactLockTimer -= gUpdateRate;
        } else if (self->unkAF & ARROW_FLAG_1_Interacted) {
            cannonPtr = self;
            gDLL_2_Camera->vtbl->change_camera_module(DLL_ID_CAMCANNON, TRUE, 0, sizeof(&cannonPtr), &cannonPtr, 50, Cam_Ease_All);
            objData->state = DIMCannon_STATE_3_Controlled_by_Player;
            gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);
            objData->interactLockTimer = 60;
            self->unkAF |= ARROW_FLAG_8_No_Targetting;

            //@recomp: block A_BUTTON, so the player doesn't attack while mounting the cannon
            joyDisableButtons(0, A_BUTTON);
        }

        objData->fire = FALSE;
        objData->fireCooldown = 0;
        objData->enemyAimCooldown = 0;
        break;
    case DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile:
        //CannonClaw aims, but doesn't fire

        DIMCannon_aimCannonClaw(self, objData->targetCoords.x, objData->targetCoords.y, objData->targetCoords.z, objData->targetDistSq);
        
        if (mainGetBits(objSetup->gamebitCannonClawDead)) {
            //Check if the CannonClaw died
            objData->state = DIMCannon_STATE_5_Idle;
        } else if (objData->targetObj && 
            (mainGetBits(objSetup->gamebitCannonClawTruce) == FALSE) && 
            (vec3DistanceXZSquared(&self->globalPosition, &objData->targetObj->globalPosition) < (objSetup->hostileRange * SQ(500.0f) / SQ(10)))
        ) {
            //Become hostile if the target comes into range and the truce gamebit isn't set
            objData->state = DIMCannon_STATE_1_Controlled_by_CannonClaw_Aim_Hostile;
        }

        objData->fire = FALSE;

        //@recomp: don't zero out cooldowns here, since it causes the cannon to fire as soon as the CannonClaw exits the silo
        // objData->fireCooldown = 0;
        // objData->enemyAimCooldown = 0;
        break;
    case DIMCannon_STATE_1_Controlled_by_CannonClaw_Aim_Hostile:
        if (mainGetBits(objSetup->gamebitCannonClawDead)) {
            objData->state = DIMCannon_STATE_5_Idle;
        } else if (mainGetBits(objSetup->gamebitCannonClawTruce)) {
            objData->state = DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile;
        } else if (objData->targetObj != NULL) {
            objData->targetCoords.x = objData->targetObj->srt.transl.x;
            objData->targetCoords.y = objData->targetObj->srt.transl.y;
            objData->targetCoords.z = objData->targetObj->srt.transl.z;
            
            if (objData->fireCooldown > 0) {
                objData->fireCooldown -= gUpdateRate;
            }
            
            if (objData->enemyAimCooldown > 0) {
                objData->enemyAimCooldown -= gUpdateRate;
            }
            
            objData->targetDistSq = vec3DistanceXZSquared(&self->globalPosition, &objData->targetObj->globalPosition);
            
            if ((objData->targetDistSq < SQ(objSetup->rangeSiloRetreat)) && (objData->distracted == FALSE)) {
                //If the target comes close while the CannonClaw isn't distracted, set a gamebit so the cannon can retreat into its silo
                sidekick = objGetSidekick();
                if (sidekick != NULL) {
                    ((DLL_ISidekick*)sidekick->dll)->vtbl->func21(sidekick, 0, 0);
                }

                mainSetBits(objSetup->gamebitSiloEnter, TRUE);
                objData->state = DIMCannon_STATE_6_Retreated_into_Silo;
            } else {
                DIMCannon_aimCannonClaw(self, objData->targetCoords.x, objData->targetCoords.y, objData->targetCoords.z, objData->targetDistSq);
                DIMCannon_fireWhenReady(self);

                //Become nonhostile when the target's out of firing range
                if ((objData->targetDistSq > ((objSetup->hostileRange * SQ(510.0f)) / SQ(10))) ||
                    (((DLL_210_Player*)player->dll)->vtbl->func66(player, 1) == 0) || //@recomp: ignore the player if they're in a sequence
                    (((DLL_210_Player*)player->dll)->vtbl->get_health(player) <= 0)   //@recomp: ignore the player if they're dead
                ) {
                    objData->state = DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile;
                }
            }            
        } else {
            objData->state = DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile;
        }

        break;
    case DIMCannon_STATE_6_Retreated_into_Silo:
        if (objData->targetObj) {
            objData->targetCoords.x = objData->targetObj->srt.transl.x;
            objData->targetCoords.y = objData->targetObj->srt.transl.y;
            objData->targetCoords.z = objData->targetObj->srt.transl.z;
            objData->targetDistSq = vec3DistanceXZSquared(&self->globalPosition, &objData->targetObj->globalPosition);
            
            if (objData->targetDistSq > ((objSetup->hostileRange * SQ(300.0f)) / SQ(10))) {
                mainSetBits(objSetup->gamebitSiloExit, TRUE);
                objData->state = DIMCannon_STATE_1_Controlled_by_CannonClaw_Aim_Hostile;

                //@recomp: reset cooldown, so the cannon can't fire immediately as it's exiting the silo and catch players off-guard
                objData->fireCooldown = 100;
                objData->fire = FALSE;
            }
        }
        break;
    case DIMCannon_STATE_2_Controlled_by_CannonClaw_Idle:
        if ((objData->targetDistSq < SQ(objSetup->rangeSiloRetreat)) && (objData->distracted == FALSE)) {
            sidekick = objGetSidekick();
            if (sidekick != NULL) {
                ((DLL_ISidekick*)sidekick->dll)->vtbl->func21(sidekick, 0, 0);
            }

            mainSetBits(objSetup->gamebitSiloEnter, TRUE);
            objData->state = DIMCannon_STATE_6_Retreated_into_Silo;
        }
        break;
    }

    SeqJoint* barrelJoint;
    barrelJoint = (SeqJoint*)objExpr_func_80034804(self, 0);

    //Handle animation
    {
        //Use a faster animSpeed during the recoil animation
        if ((self->curModAnimId == 0) || (self->curModAnimId != 1)) {
            animSpeed = 0.01f;
        } else {
            animSpeed = 0.025f;
        }
        objAnimAdvance(self, animSpeed, gUpdateRateF, 0);

        //@recomp: ease barrel joint towards neutral position when idle
        if (objData->state == DIMCannon_STATE_5_Idle) {
            s32 angle = -barrelJoint->pitch;
            angle -= 600 * gUpdateRate;
            
            if (angle > M_45_DEGREES) {
                angle = M_45_DEGREES;
            }
            if (angle < 0) {
                angle = 0;
            }

            angle = -angle;
            angle -= (u16)(barrelJoint->pitch);
            CIRCLE_WRAP(angle);            
            barrelJoint->pitch += (angle >> 3) * gUpdateRate; //@recomp: fix framerate dependency
        }
    }

    //@recomp: audio/shake
    switch (objData->state) {
    case DIMCannon_STATE_1_Controlled_by_CannonClaw_Aim_Hostile:
    case DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile:
        DIMCannon_handleSoundsAndFX(self, DIMCannon_simulateCannonClawStickX(self), barrelJoint);
        break;
    default:
        DIMCannon_handleSoundsAndFX(self, 0, barrelJoint);
        break;
    }
}

/* Free soundHandles */
RECOMP_PATCH void DIMCannon_obj_Free(Object* self, s32 onlySelf) {
    if (self->id == OBJ_DIMCannonBall) {
        DIMCannon_freeCannonBall(self);
    } else {
        dllFree(dModGfxDLL);

        //@recomp: free soundhandles
        DIMCannon_stopSoundLoops(self);
    }
    
    objFreeObjectType(self, OBJTYPE_Baddie);
}

/* Fix framerate dependencies, allow dismounting cannon with B, play animations, add sounds. */
RECOMP_PATCH int DIMCannon_animCallback(Object* self, Object* overrideObj, AnimObj_Data* animData, s8 prevCallbackValue) {
    DIMCannon_Data* objData;
    DIMCannon_Setup* objSetup;
    SeqJoint* barrelJoint;
    /* RECOMP */
    s32 stickX = 0;

    animData->unk62 = 0;
    animData->unk7A &= ~(0x400 | 0x200 | 8);
    
    objData = self->data;

    //@recomp: store state at beginning of tick
    objData->prevState = objData->state;

    barrelJoint = (SeqJoint*)objExpr_func_80034804(self, 0);
    
    //@recomp: Handle LockIcon
    if (self->unkAF & ARROW_FLAG_8_No_Targetting) {
        //@recomp: check whether the player can actually interact with the cannon
        if ((objData->state == DIMCannon_STATE_5_Idle) && (objData->interactLockTimer <= 0)) {
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        }
    }

    switch (objData->state) {
    case DIMCannon_STATE_3_Controlled_by_Player:
        if (objData->interactLockTimer > 0) {
            objData->interactLockTimer -= gUpdateRate;
        } else {      
            s32 angle;
            
            angle = -barrelJoint->pitch;

            stickX = joyGetStickX(0);
            if (stickX) {
                self->srt.yaw -= stickX * 2 * gUpdateRate; //@recomp: fix framerate dependency

                //Get stickX's magnitude
                if (stickX < 0) {
                    stickX = -stickX;
                }
            }

            if (objData->fireCooldown > 0) {
                objData->fireCooldown -= gUpdateRate;
            }
    
            if (objData->enemyAimCooldown > 0) {
                objData->enemyAimCooldown -= gUpdateRate;
            }
            
            if ((joyGetButtons(0) & A_BUTTON) && (objData->fireCooldown <= 0)) {
                angle += 400 * gUpdateRate; //@recomp: fix framerate dependency
            } else {
                angle -= 600 * gUpdateRate; //@recomp: fix framerate dependency
            }
            
            if (angle > M_45_DEGREES) {
                angle = M_45_DEGREES;
            }
            if (angle < 0) {
                angle = 0;
            }
            
            if ((joyGetReleased(0) & A_BUTTON) && (objData->fireCooldown <= 0)) {
                objData->fire = TRUE;
            }
            DIMCannon_fireWhenReady(self);
            
            // @recomp: allow exiting the cannon with either the Z or B buttons
            // (players often get confused about how to dismount it with just Z)
            if (joyGetPressed(0) & (Z_TRIG | B_BUTTON)) {
                gDLL_2_Camera->vtbl->change_camera_module(DLL_ID_CAMNORMAL, FALSE, 1, 0, NULL, 0, Cam_Ease_All);
                objData->state = DIMCannon_STATE_5_Idle;
                objData->interactLockTimer = 60;
                animData->unk9D |= 4;
                // self->unkAF &= ~ARROW_FLAG_8_No_Targetting; //@recomp: don't clear this before the cannon can be interacted with
            }
            
            angle = -angle;
            angle -= (u16)(barrelJoint->pitch);
            CIRCLE_WRAP(angle);            
            barrelJoint->pitch += angle >> 2;
        }
        break;
    case DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile:
        //Calculate simulated stickX for the CannonClaw during sequences
        stickX = DIMCannon_simulateCannonClawStickX(self);
        
        objData->fireCooldown = CANNON_CLAW_SHOT_ANTICIPATE;
        /* FALLTHROUGH */
    default:
        self->srt.flags &= ~OBJFLAG_INVISIBLE;
        if (animData->lastMessage == 1) {
            objSetup = (DIMCannon_Setup*)self->setup;
            mainSetBits(objSetup->gamebitSiloCoverOpen, 1);
        }
        
        animData->lastMessage = 0;
        if (objData->state != DIMCannon_STATE_6_Retreated_into_Silo) {
            objData->state = DIMCannon_STATE_4_Controlled_by_CannonClaw_Aim_Nonhostile;
        }

        //@recomp: reverse cooldown when CannonClaw isn't about to fire
        DIMCannon_reverseAimSoundBuildup(self);

        break;
    } 

    //@recomp: advance animations as intended (there was a bug where it only advanced in control, and not in this animCallback)
    {
        f32 animSpeed;
        if ((self->curModAnimId == 0) || (self->curModAnimId != 1)) {
            animSpeed = 0.01f;
        } else {
            animSpeed = 0.025f;
        }
        objAnimAdvance(self, animSpeed, gUpdateRateF, 0);
    }

    DIMCannon_handleSoundsAndFX(self, stickX, barrelJoint);

    return 0;  
}

/* Fix framerate dependencies. */
RECOMP_PATCH void DIMCannon_aimCannonClaw(Object* self, f32 x, f32 y, f32 z, f32 targetDist) {
    s32 pad[5];
    s32 dPitchAngle;
    s32 aimFinished;
    SeqJoint* barrelJoint;
    s32 dYaw;
    f32 aimVectorY;
    f32 targetDiffY;
    s32 aimShift;
    f32 aimVectorX;
    f32 sqAimVectorY;
    f32 distance;
    DIMCannon_Data* objData;
    
    objData = self->data;

    //Don't adjust aim if the CannonClaw fired recently
    if (objData->enemyAimCooldown > 0) {
        return;
    }

    //Calculate the barrel pitch angle needed for the cannon to hit its target, and animate towards it 
    {
        barrelJoint = (SeqJoint*)objExpr_func_80034804(self, 0);
        
        sqAimVectorY = 2500.0f;

        //Get actual target distance (distance is passed as square)
        targetDist = sqrtf(targetDist);

        aimVectorX = targetDist * 2.2f;
        sqAimVectorY = SQ(sqAimVectorY) - SQ(aimVectorX);
        
        aimFinished = FALSE;
        if (sqAimVectorY >= 0) {
            dPitchAngle = mathAtan2f(aimVectorX, sqrtf(sqAimVectorY)) >> 1;
        } else {
            dPitchAngle = M_45_DEGREES;
            aimFinished = TRUE;
        }
        
        targetDiffY = (objData->muzzleCoords.y - y) - 10.0f;
        if (targetDiffY > 0.0f) {
            aimShift = -M_1_DEGREE;
        } else {
            aimShift = M_1_DEGREE;
        }
        
        while (aimFinished == FALSE) {
            f32 aimCoefficient = 4.0f;

            aimVectorY = mathSinfInterp(dPitchAngle) * 50.0f;
            aimVectorX = SQ(aimVectorY) - ((aimCoefficient * -1.1f) * targetDiffY);

            if (SQ(aimVectorY) >= aimCoefficient * -1.1f * targetDiffY) {
                // f32 temp1 = -1.1f;
                // f32 temp2 = -1.1f;

                aimVectorX = sqrtf(aimVectorX);
                // if (aimVectorY) {}
                // if ((temp1 + temp2) != 0.0f) { 
                // }
            }

            distance = mathCosfInterp(dPitchAngle) * 50.0f * aimVectorX;
            
            dPitchAngle += aimShift;
            
            if ((distance > targetDist) && (aimShift > 0)) {
                aimFinished = TRUE;
            }
            
            if ((distance < targetDist) && (aimShift < 0)) {
                aimFinished = TRUE;
            }
            
            if (dPitchAngle > M_45_DEGREES) {
                dPitchAngle = M_45_DEGREES;
                aimFinished = TRUE;
            } else if (dPitchAngle < 0) {
                dPitchAngle = 0;
                aimFinished = TRUE;
            }
        }

        dPitchAngle = -dPitchAngle;
        dPitchAngle -= (barrelJoint->pitch & 0xFFFF);
        CIRCLE_WRAP(dPitchAngle);
        
        //Ease barrel joint towards goal angle
        barrelJoint->pitch += (dPitchAngle >> 3) * gUpdateRate; //@recomp: fix framerate dependency
    }

    //Calculate the yaw needed for the cannon to hit its target, animate towards it, and decide whether to fire
    {
        //Get the angle between the cannon and the target
        x -= self->srt.transl.x;
        z -= self->srt.transl.z;
        dYaw = ((s16)mathAtan2f(x, z)) - (self->srt.yaw & 0xFFFF);
        CIRCLE_WRAP(dYaw);

        if (dYaw > M_45_DEGREES/2) {
            dYaw = M_45_DEGREES/2;
        }
        if (dYaw < -M_45_DEGREES/2) {
            dYaw = -M_45_DEGREES/2;
        }

        // if (aimVectorX) {}
        
        //Fire if the cannon's yaw is within ~11 degrees of aiming directly at the target
        if ((M_45_DEGREES/4 > dYaw) && (dYaw > -M_45_DEGREES/4)) {
            objData->fire = TRUE;
        }
        
        //Don't fire if the target is too close
        if (objData->targetDistSq < SQ(100)) {
            objData->fire = FALSE;
        }
        
        self->srt.yaw += (dYaw >> 3) * gUpdateRate; //@recomp: fix framerate dependency
    }
}

/* Send a message to the CannonClaw, so they can recoil if they fired the cannon. */
RECOMP_PATCH void DIMCannon_fireWhenReady(Object* self) {
    DIMCannonBall_Setup* shotSetup;
    Object* shot;
    s16* angle;
    DIMCannon_Data* objData;
    DIMCannon_Setup* objSetup;

    objData = self->data;
    objSetup = (DIMCannon_Setup*)self->setup;
    
    if (objData->fire && (objData->fireCooldown <= 0)) {
        //@recomp: send a message to the CannonClaw
        objSendMesgManyNearby(OBJ_CannonClaw, 40.0f, 0, self, CannonClaw_FLAG_1_Cannon_Fired, 0);

        //@recomp: play blast sound when firing
        if (DIMCannon_configUseBasicSounds() == TRUE) {
            dll_amSfx->Play(self, SOUND_96_Cannon, MAX_VOLUME, NULL, NULL, 0, NULL);
        }

        angle = objExpr_func_80034804(self, 0);
        
        shotSetup = (DIMCannonBall_Setup*)objAllocSetup(sizeof(DIMCannonBall_Setup), OBJ_DIMCannonBall);
        // shotSetup->base.loadFlags = objSetup->base.loadFlags;
        // shotSetup->base.fadeFlags = objSetup->base.fadeFlags;
        // shotSetup->base.loadDistance = objSetup->base.loadDistance;
        // shotSetup->base.fadeDistance = objSetup->base.fadeDistance;

        //@recomp: show faraway cannonballs
        {
            shotSetup->base.loadFlags = OBJSETUP_LOAD_LEVEL;
            shotSetup->base.fadeFlags = OBJSETUP_FADE_MANUAL;
            shotSetup->base.loadDistance = 0xFF;
            shotSetup->base.fadeDistance = 0xFF;
        }

        shotSetup->base.x = objData->muzzleCoords.x;
        shotSetup->base.y = objData->muzzleCoords.y;
        shotSetup->base.z = objData->muzzleCoords.z;
        shotSetup->yaw = self->srt.yaw >> 8;
        shotSetup->velocityY = mathSinfInterp(*angle) * 50.0f;
        shotSetup->velocityX = mathCosfInterp(*angle) * 50.0f;
        
        shot = objSetupObject((ObjSetup*)shotSetup, 5, self->mapID, -1, NULL);
        shot->unkC4 = self;
        
        objData->fire = FALSE;
        objData->enemyAimCooldown = 50;
        
        if (objData->state == DIMCannon_STATE_3_Controlled_by_Player) {
            objData->fireCooldown = 100;
        } else {
            objData->fireCooldown = mathRnd(objSetup->cooldownMin, objSetup->cooldownMax);
        }
        
        objAnimSet(self, 1, 0.2f, 0); //@recomp: snappier recoil
    }
}

/* Fix cannon explosions being invisible */
RECOMP_PATCH void DIMCannon_createCannonBallExplosion(Object* self) {
    DIMCannonBall_Setup* objSetup;
    DIMExplosion_Setup* boomSetup;

    // objSetup = (DIMCannonBall_Setup*)self->data;

    boomSetup = (DIMExplosion_Setup*)objAllocSetup(sizeof(DIMExplosion_Setup), OBJ_DIMExplosion);
    // boomSetup->base.loadFlags = objSetup->base.loadFlags;
    // boomSetup->base.byte6 = objSetup->base.byte6;
    // boomSetup->base.byte5 = objSetup->base.byte5;
    // boomSetup->base.fadeDistance = objSetup->base.fadeDistance;

    //@recomp: show faraway explosions
    {
        boomSetup->base.loadFlags = OBJSETUP_LOAD_LEVEL;
        boomSetup->base.fadeFlags = OBJSETUP_FADE_MANUAL;
        boomSetup->base.loadDistance = 0xFF;
        boomSetup->base.fadeDistance = 0xFF;
    }

    boomSetup->base.x = self->srt.transl.x;
    boomSetup->base.y = self->srt.transl.y;
    boomSetup->base.z = self->srt.transl.z;
    objSetupObject((ObjSetup*)boomSetup, 5, self->mapID, -1, self->parent);
}

/* Send a message to the cannon when the cannonball hits the player/sidekick */
RECOMP_PATCH void DIMCannon_tickCannonBall(Object* self) {
    ObjectHitInfo* objHits;
    s32 pad;
    DIMCannonBall_Data* objData;
    u8 useMoreSounds = 
    
    //Apply gravity and move
    self->velocity.y += -0.022f * gUpdateRateF;
    objMove(self, self->velocity.x * gUpdateRateF, self->velocity.y * gUpdateRateF, self->velocity.z * gUpdateRateF);

    //Handle colliding with objects (ignoring the parent cannon object)
    objHits = self->objhitInfo;
    if (objHits != NULL) {
        func_80026128(self, 5, 1, 0);
        if ((objHits->unk48 != NULL) && (objHits->unk48 != self->unkC4)) {
            //@recomp: message CannonClaw if the cannonball struck the player/sidekick (NOTE: DIMCannon as sender, since it helps with distance checks)
            if (DIMCannon_configUseFancySounds() == TRUE) {
                if (objHits->unk48->controlNo == OBJCONTROL_Player || objHits->unk48->controlNo == OBJCONTROL_Sidekick) {
                    objSendMesgManyNearby(OBJ_CannonClaw, 40.0f, 0, self->unkC4, CannonClaw_FLAG_2_Hit_Player_or_Sidekick, 0);
                }
            }

            DIMCannon_createCannonBallExplosion(self);
            objFreeObject(self);
        }
    }
    
    //Handle colliding with terrain
    if (self->objhitInfo->unk9D != 0) {
        DIMCannon_createCannonBallExplosion(self);
        objFreeObject(self);
    }
    
    //Unload after 20 seconds
    self->unkDC += gUpdateRate;
    if (self->unkDC > 20 * 60) {
        objFreeObject(self);
    }
    
    objData = self->data;
    
    //Align model with velocity vector
    self->srt.pitch = mathAtan2f(self->velocity.y, sqrtf(SQ(self->velocity.x) + SQ(self->velocity.z)));
    
    //Create modGfx
    if (objData->createModGfx) {
        dModGfxDLL->vtbl->func0(self, 2, 0, 0x10002, -1, 0);
        objData->createModGfx = FALSE;
    }
}

/* Extend DIMCannon_Data struct */
RECOMP_PATCH s32 DIMCannon_obj_GetDataSize(Object* self, s32 offsetAddr) {
    if (self->id == OBJ_DIMCannonBall) {
        return sizeof(DIMCannonBall_Data);
    } else {
        return sizeof(DIMCannon_Data); //@recomp: extend struct
    }
}
