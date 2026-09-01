#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "dlls/objects/210_player.h"
#include "game/objects/object_id.h"
#include "game/gamebits.h"
#include "sys/gfx/modgfx.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objmsg.h"

#include "recomp/dlls/_asm/788_recomp.h"

//TEMPORARY DEFINES
#define WCSunTempleLaser_obj_Control dll_788_obj_Control

#define dModGfxDLL data_0

#define BIT_WC_Moon_Temple_Hazards_Deactivated 0x338
#define BIT_468_Forcefield_Spell_Taking_Damage 0x468
#define SOUND_2BB_Laser_Stop_Hiss 0x2BB
#define SOUND_9F9_Laser_Startup_Hiss 0x9F9
#define SOUND_9FA_Laser_Whir_Loop 0x9FA
#define SOUND_228_Laser_Zap 0x228
#define PARTICLE_28B 0x28B
//END OF TEMPORARY DEFINES

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 mode;
    s16 offIntervalDuration;
    s16 firingDuration;
    s16 gamebitEnabled;
} WCSunTempleLaser_Setup;

typedef struct {
    Texture* texLaser;
    f32 timerDelay;
    f32 timerFiring;
    f32 timerOffInterval;
    u32 soundHandle;
    u8 _unk14[0x2C - 0x14];
    f32 whirVolume;
    f32 _unk30;
    u8 laserIsActive;
    u8 skipStateCheck;
    s8 hurtRange;
    s8 playerZapCooldown;
    s16 timerStateCheckDelay;
    s16 timerDelayThreshold;
    SRT playerKnockbackDir;
    u8 state;
    u8 unkPlayerMessage;
    u8 beamStartHissPlayed;
    u8 flags;
} WCSunTempleLaser_Data;

typedef enum {
    WCSunTempleLaser_Flag_Firing = 1
} WCSunTempleLaser_Flags;

typedef enum {
    WCSunTempleLaser_STATE_0_Off,
    WCSunTempleLaser_STATE_1_Started,
    WCSunTempleLaser_STATE_2_Stopping
} WCSunTempleLaser_States;

typedef enum {
    WCSunTempleLaser_MODE_Always_On = 0,    //Sun-themed, doesn't cycle on/off
    WCSunTempleLaser_MODE_Timed_Blue = 1,   //Moon-themed
    WCSunTempleLaser_MODE_Timed_Yellow = 2  //Sun-themed
} WCSunTempleLaser_Modes;

/*0x0*/ extern DLL_IModgfx* dModGfxDLL;

RECOMP_PATCH void WCSunTempleLaser_obj_Control(Object* self) {
    WCSunTempleLaser_Setup* objSetup;
    WCSunTempleLaser_Data* objData;
    s32 pad1[3];
    f32 sin;
    f32 cos;
    f32 worldOriginInObjectSpaceZ;
    f32 minusCos;
    f32 distance;
    f32 worldOriginInObjectSpaceX;
    s32 effectIdx;
    s32 pad2;
    Object* player;
    s32 i;
    f32 range;
    f32 radius;
    f32 dy;
    s32 pad3[2];
    f32 sp34;

    objSetup = (WCSunTempleLaser_Setup*)self->setup;
    objData = self->data;
    objData->timerDelay -= gUpdateRateF;
    
    if ((objSetup->mode != WCSunTempleLaser_MODE_Always_On) && (mainGetBits(BIT_WC_Moon_Temple_Hazards_Deactivated))) {
        mainSetBits(objSetup->gamebitEnabled, FALSE);
    }
    
    //Laser's State Machine (starting/stopping/waiting)
    if (mainGetBits(objSetup->gamebitEnabled)) {
        if (objData->flags & WCSunTempleLaser_Flag_Firing) {
            if (objSetup->mode != WCSunTempleLaser_MODE_Always_On) {
                objData->timerFiring -= gUpdateRateF;
            }
            
            if (objData->timerFiring < 0.0f) {
                //Switch off
                objData->timerOffInterval = objSetup->offIntervalDuration;
                objData->flags &= ~WCSunTempleLaser_Flag_Firing;
                
                if (objData->soundHandle != 0) {
                    dll_amSfx->Stop(objData->soundHandle);
                    objData->soundHandle = 0;
                }
                
                gDLL_14_Modgfx->vtbl->func5(self);
                dll_amSfx->Play(self, SOUND_2BB_Laser_Stop_Hiss, MAX_VOLUME, NULL, NULL, 0, NULL);
            } else {
                //Firing
                if (objData->timerDelay < 0.0f) {
                    objData->timerDelay = 275.0f;
                    objData->state = WCSunTempleLaser_STATE_0_Off;
                } else if (objData->timerDelay < objData->timerDelayThreshold) {
                    if (objData->state == WCSunTempleLaser_STATE_0_Off) {
                        //Start laser whirring sound loop
                        if (objData->soundHandle == 0) {
                            dll_amSfx->Play(self, SOUND_9FA_Laser_Whir_Loop, MAX_VOLUME, &objData->soundHandle, NULL, 0, NULL);
                        }
                        
                        //Play a hiss sound as the beam starts firing
                        if (objData->beamStartHissPlayed == FALSE) {
                            objData->beamStartHissPlayed = TRUE;
                            dll_amSfx->Play(self, SOUND_9F9_Laser_Startup_Hiss, MAX_VOLUME, NULL, NULL, 0, NULL);
                        }
                        
                        objData->state = WCSunTempleLaser_STATE_1_Started;
                        
                        //Create modGfx
                        if (dModGfxDLL != NULL) {
                            effectIdx = (objSetup->mode == WCSunTempleLaser_MODE_Timed_Blue) ? 13 : 15;
                            dModGfxDLL->vtbl->func0(self, effectIdx, NULL, 0x10004, -1, NULL);
                        }
                    }
                    
                    if (objData->timerDelay < 140.0f) {
                        if (objData->state == WCSunTempleLaser_STATE_1_Started) {
                            objData->state = WCSunTempleLaser_STATE_2_Stopping;
                            if (dModGfxDLL != NULL) {
                                effectIdx = (objSetup->mode == WCSunTempleLaser_MODE_Timed_Blue) ? 13 : 16;
                                dModGfxDLL->vtbl->func0(self, effectIdx, NULL, 0x10004, -1, NULL);
                            }
                        }
                    } else if (objData->whirVolume <= 1.0f) {
                        objData->whirVolume += 0.052f * gUpdateRateF;
                    }
                }
            }
        } else if (objSetup->mode != WCSunTempleLaser_MODE_Always_On) {
            //Laser off, wait out the interval between blasts
            objData->timerOffInterval -= gUpdateRateF;
            if (objData->timerOffInterval <= 0.0f) {
                objData->timerFiring = objSetup->firingDuration;
                objData->flags |= WCSunTempleLaser_Flag_Firing;
                objData->state = WCSunTempleLaser_STATE_0_Off;
                objData->beamStartHissPlayed = FALSE;
                objData->timerDelay = 0.0f;
                objData->whirVolume = 0.0f;
            }
        } else {
            objData->flags |= WCSunTempleLaser_Flag_Firing;
        }
    } else {
        objData->state = WCSunTempleLaser_STATE_0_Off;
        objData->beamStartHissPlayed = FALSE;
        objData->timerOffInterval = 0.0f;
        objData->timerDelay = 0.0f;
        objData->whirVolume = 0.0f;
    }
    
    //Handle laser whir volume
    if (objData->laserIsActive) {
        if (objData->soundHandle != 0) {
            dll_amSfx->SetVol(objData->soundHandle, (s8) (objData->whirVolume * 127.0f));
        }
    } else {
        //@recomp: stop whirring sound when laser switches off (by gamebit)
        if (objData->soundHandle) {
            dll_amSfx->Stop(objData->soundHandle);
            objData->soundHandle = 0;
        }
    }
    
    //Determining if the laser should hurt the player
    {
        sin = mathSinfInterp(self->srt.yaw);
        cos = mathCosfInterp(self->srt.yaw);
        worldOriginInObjectSpaceZ = -((self->srt.transl.x * sin) + (self->srt.transl.z * cos));
        
        player = objGetPlayer();
        
        objData->playerZapCooldown -= gUpdateRate;
        if (objData->playerZapCooldown < 0) {
            objData->playerZapCooldown = 0;
        }
        
        //Check the state var to determine if the laser is active, but only during the first second of the laser being active?
        {
            if (objData->laserIsActive) {
                objData->timerStateCheckDelay += gUpdateRate;
                if (objData->timerStateCheckDelay > 60) {
                    objData->timerStateCheckDelay = 60;
                    objData->skipStateCheck = TRUE;
                }
            }
            
            if (objData->skipStateCheck == FALSE) {
                objData->laserIsActive = objData->state & (WCSunTempleLaser_STATE_1_Started | WCSunTempleLaser_STATE_2_Stopping);
            } else {
                objData->laserIsActive = TRUE;
            }
        }
        
        //Check the laser's gamebit and its firing flag
        if ((mainGetBits(objSetup->gamebitEnabled) == FALSE) || (objData->flags & WCSunTempleLaser_Flag_Firing) == FALSE) {
            objData->laserIsActive = FALSE;
        }

        //Don't hurt the player if they were zapped recently, or if the laser's off
        if (player == NULL || objData->playerZapCooldown != 0 || objData->laserIsActive == FALSE) {
            return;
        }

        //Check if the player is close enough to be zapped by the laser
        range = objData->hurtRange + 5.0f;
        dy = player->srt.transl.y - self->srt.transl.y;
        if ((dy < range) && (-(range + 25.0f) < dy)) {

            //Check the player's position along the laser's objectSpace X axis (i.e. distance perpendicular to the beam)
            minusCos = -cos; 
            worldOriginInObjectSpaceX = -((self->srt.transl.x * minusCos) + (self->srt.transl.z * sin)); 
            distance = (player->srt.transl.x * minusCos) + (sin * player->srt.transl.z) + worldOriginInObjectSpaceX;

            if ((range > distance) && (distance > -range)) {

                //Check the player's position along the laser's objectSpace Z axis (i.e. distance along the beam)
                distance = (player->srt.transl.x * sin) + (cos * player->srt.transl.z) + worldOriginInObjectSpaceZ;
                if ((0.0f < distance) && (distance < 170.0f)) {
                    //Zap the player if they're not using the Forcefield Spell
                    if (((DLL_210_Player*)player->dll)->vtbl->func50(player) != BIT_Spell_Forcefield) {
                        dll_amSfx->Play(self, SOUND_228_Laser_Zap, MAX_VOLUME, NULL, NULL, 0, NULL);
                        
                        for (i = 0; i < 4; i++) {
                            gDLL_17_partfx->vtbl->spawn(objGetPlayer(), PARTICLE_28B, NULL, 4, -1, NULL);
                        }
                        
                        distance = ((player->prevLocalPosition.x * minusCos) + (sin * player->prevLocalPosition.z)) + worldOriginInObjectSpaceX;
                        if (distance < 0.0f) {
                            radius = -20.0f;
                        } else {
                            radius = 20.0f;
                        }

                        objData->playerKnockbackDir.transl.x = player->srt.transl.x + (minusCos * radius);
                        objData->playerKnockbackDir.transl.z = player->srt.transl.z + (sin * radius);
                        
                        if ((objData->unkPlayerMessage == 0) || (objData->unkPlayerMessage == 1)) {
                            //NOTE: casting an SRT* to an Object*, but the player code only reads the SRT portion when handling this message, so it's safe!
                            objSendMesg(player, 0x60003, (Object*)&objData->playerKnockbackDir, NULL);
                        }
                        
                        objData->playerZapCooldown = 20;
                    } else {
                        //Or if they are using the Forcefield Spell, have it change colour to show it's shielding damage
                        mainSetBits(BIT_468_Forcefield_Spell_Taking_Damage, TRUE);
                    }     
                }
            }
        }
    }
}
