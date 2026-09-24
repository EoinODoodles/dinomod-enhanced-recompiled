#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "sys/rand.h"

#include "common_objsetups.h"

#include "recomp/dlls/objects/266_sfxplayer_recomp.h"

// #define DEBUG_FADES

//TEMPORARY DEFINES
#define sfxPlayer_obj_Setup sfxplayer_setup
#define sfxPlayer_obj_Control sfxplayer_control
#define sfxPlayer_obj_GetDataSize sfxplayer_get_data_size
//END OF TEMPORARY DEFINES

typedef struct {
/*00*/ f32 innerDistanceSq;
/*04*/ f32 outerDistanceSq;
/*08*/ u8 prevGamebitValue;
/*0C*/ u32 soundHandle;
/* RECOMP */
/*10*/ s16 fadeTimer;
/*12*/ u8 fadeFlags;
} SfxPlayer_Data;

typedef enum {
    FADE_IN_DONE = 1,
    FADE_OUT_DONE = 2,
    STOP_SOUND = 4
} FadeFlags;

RECOMP_PATCH void sfxPlayer_obj_Setup(Object* self, SfxPlayer_Setup* objSetup, s32 reset) {
    SfxPlayer_Data* objData;
    SfxPlayer_Setup* setup;

    objData = self->data;
    setup = (SfxPlayer_Setup*)self->setup;

    objData->prevGamebitValue = mainGetBits(objSetup->gamebit);

    //Set up prevGamebitValue so looping sounds start immediately if their gamebit has the correct value
    if (setup->flags & SfxPlayer_FLAG_1_Looping_Sound) {
        //Start out playing looping sound if its gamebit is unset
        if (objData->prevGamebitValue == FALSE && setup->flags & SfxPlayer_FLAG_4_Play_if_Gamebit_Unset) {
            objData->prevGamebitValue = TRUE;

        //@recomp: Add option to start out playing looping sound if its gamebit is already set
        } else if ((setup->flags & SfxPlayer_CUSTOMFLAG_40_Play_if_Gamebit_Set_on_Setup) &&
            (objData->prevGamebitValue == TRUE && setup->flags & SfxPlayer_FLAG_2_Play_if_Gamebit_Set)
        ) {
            objData->prevGamebitValue = FALSE;
        }
    }        

    self->stateFlags |= OBJSTATE_UPDATE_DISABLED;

    if (setup->flags & SfxPlayer_CUSTOMFLAG_20_Double_Radius) { //@recomp: add option to double activation distance
        objData->innerDistanceSq = objSetup->halfRadius * 4;
    } else {
        objData->innerDistanceSq = objSetup->halfRadius * 2;
    }

    objData->outerDistanceSq = objData->innerDistanceSq + 10.0f;

    objData->innerDistanceSq = SQ(objData->innerDistanceSq);
    objData->outerDistanceSq = SQ(objData->outerDistanceSq);
}

/* Add some extra features: 
  - Optional volume adjustment
  - Optional pitch adjustment
  - Optional fade in for looped sounds
  - Optional fade out for looped sounds
*/
RECOMP_PATCH void sfxPlayer_obj_Control(Object* self) {
    #define PLAY_BY_GAMEBIT (SfxPlayer_FLAG_2_Play_if_Gamebit_Set | SfxPlayer_FLAG_4_Play_if_Gamebit_Unset)
    #define PLAY_IF_UNSET ((flags & SfxPlayer_FLAG_4_Play_if_Gamebit_Unset) == SfxPlayer_FLAG_4_Play_if_Gamebit_Unset)
    SfxPlayer_Data* objData;
    SfxPlayer_Setup* setup;
    u8 gamebitValue;
    f32 playerDistanceSq;
    u8 flags;
    /* RECOMP */
    u8 volume;

    setup = (SfxPlayer_Setup*)self->setup;
    objData = self->data;

    gamebitValue = FALSE;
    if (setup->gamebit) {
        gamebitValue = mainGetBits(setup->gamebit);
    }

    //@recomp: get optional volume/pitch adjustments
    volume = setup->volume ? setup->volume : MAX_VOLUME;

    flags = setup->flags;

    if (flags & SfxPlayer_FLAG_1_Looping_Sound) {
        //LOOPING SOUND

        if (setup->gamebit != NO_GAMEBIT) {
            //Play by gamebit
            if ((gamebitValue != objData->prevGamebitValue) && (flags & PLAY_BY_GAMEBIT)) {
                if ((gamebitValue != PLAY_IF_UNSET) != 0) {
                    //Start the looping sound when the gamebit is set (or when it's unset, if the opposite mode is being used)
                    if (objData->soundHandle == 0) {
                        objData->soundHandle = dll_amSfx->Play(self, setup->soundID, volume, NULL, 0, 0, 0); //@recomp: variable volume
                    }

                    //@recomp: optionally adjust pitch
                    if (setup->pitch != 0) {
                        dll_amSfx->SetPitch(objData->soundHandle, (f32)(0x40 + setup->pitch) / 0x40);
                    }

                    //@recomp: clear stop flag, and set up fade in
                    objData->fadeFlags &= ~(STOP_SOUND | FADE_IN_DONE);
                } else if (objData->soundHandle) {
                    //Stop the looping sound when the gamebit is unset (or when it's unset, if the opposite mode is being used)
                    objData->fadeFlags |= STOP_SOUND;
                }
            }
        } else if (flags & PLAY_BY_GAMEBIT) { //NOTE: one of these flags must be set, even though it doesn't check gamebits in this mode
            //Play by distance
            playerDistanceSq = vec3DistanceSquared(&self->globalPosition, &objGetPlayer()->globalPosition);

            //Start sound when inside inner radius, stop sound when leaving outer radius
            if (playerDistanceSq < objData->innerDistanceSq) {
                if (objData->soundHandle == 0) {
                    dll_amSfx->Play(self, setup->soundID, volume, &objData->soundHandle, 0, 0, 0); //@recomp: variable volume

                    //@recomp: optionally adjust pitch
                    if (setup->pitch != 0) {
                        dll_amSfx->SetPitch(objData->soundHandle, (f32)(0x40 + setup->pitch) / 0x40);
                    }

                }

                //@recomp: clear stop flag, and set up fade in
                objData->fadeFlags &= ~(STOP_SOUND | FADE_IN_DONE);
            } else if ((objData->outerDistanceSq < playerDistanceSq) && objData->soundHandle) {
                objData->fadeFlags |= STOP_SOUND;               
            }
        }

        //@recomp: handle optionally fading looped sounds in
        if ((flags & SfxPlayer_CUSTOMFLAG_10_Fade) && objData->soundHandle && !(objData->fadeFlags & (FADE_IN_DONE | STOP_SOUND))) {
            objData->fadeTimer += gUpdateRate;
            if (objData->fadeTimer < setup->fadeDuration) {
                f32 volumeFactor;
                if (setup->fadeDuration != 0) {
                    volumeFactor = (f32)(objData->fadeTimer) / (f32)setup->fadeDuration;
                } else {
                    volumeFactor = 1.0f;
                }
                if (volumeFactor > 1.0f) {
                    volumeFactor = 1.0f;
                } else if (volumeFactor < 0.0f) {
                    volumeFactor = 0.0f;
                }
#ifdef DEBUG_FADES
                recomp_printf("%x Fading in: %d (handle %d)\n", self->setup->uID, (u8)(volumeFactor * volume), objData->soundHandle);
#endif
                dll_amSfx->SetVol(objData->soundHandle, (u8)(volumeFactor * volume));
            } else {
                objData->fadeFlags |= FADE_IN_DONE;
                objData->fadeTimer = setup->fadeDuration;
            }
        }

        //@recomp: handle sound fadeout/stop
        if (objData->fadeFlags & STOP_SOUND) {
            if ((setup->flags & SfxPlayer_CUSTOMFLAG_10_Fade) == FALSE) {
                dll_amSfx->Stop(objData->soundHandle);
                objData->soundHandle = 0;
                objData->fadeFlags &= ~STOP_SOUND;
            } else {
                objData->fadeTimer -= gUpdateRate;
                if (objData->fadeTimer > 0) {
                    f32 volumeFactor;
                    if (setup->fadeDuration != 0) {
                        volumeFactor = (f32)(objData->fadeTimer) / (f32)setup->fadeDuration;
                    } else {
                        volumeFactor = 1.0f;
                    }
                    if (volumeFactor > 1.0f) {
                        volumeFactor = 1.0f;
                    } else if (volumeFactor < 0.0f) {
                        volumeFactor = 0.0f;
                    }
#ifdef DEBUG_FADES
                    recomp_printf("%x Fading out: %d (handle %d)\n", self->setup->uID, (u8)(volumeFactor * volume), objData->soundHandle);
#endif
                    dll_amSfx->SetVol(objData->soundHandle, (u8)(volumeFactor * volume));
                } else {
                    objData->fadeTimer = 0;

                    dll_amSfx->Stop(objData->soundHandle);
                    objData->soundHandle = 0;
                    objData->fadeFlags &= ~(STOP_SOUND | FADE_IN_DONE);
                }
            }
        }
    } else {
        //ONE-SHOT SOUND

        if (flags & SfxPlayer_FLAG_8_Play_At_Random_Inside_Radius) {
            //Play by distance (randomly)
            playerDistanceSq = vec3DistanceSquared(&self->globalPosition, &objGetPlayer()->globalPosition);

            //Random chance of playing sound if the player is inside inner radius
            if (!mathRnd(0, 300) && (playerDistanceSq < objData->innerDistanceSq)) {
                dll_amSfx->Play(self, setup->soundID, volume, NULL, 0, 0, 0); //@recomp: variable volume
            }
        } else if (gamebitValue != objData->prevGamebitValue) {
            //Play by gamebit

            if ((gamebitValue == TRUE) && (flags & SfxPlayer_FLAG_2_Play_if_Gamebit_Set)) {
                //Play sound if gamebit is set
                dll_amSfx->Play(self, setup->soundID, volume, NULL, 0, 0, 0); //@recomp: variable volume
            } else if ((gamebitValue == FALSE) && (flags & SfxPlayer_FLAG_4_Play_if_Gamebit_Unset)) {
                //Play sound if gamebit isn't set
                dll_amSfx->Play(self, setup->soundID, volume, NULL, 0, 0, 0); //@recomp: variable volume
            }
        }
    }

    objData->prevGamebitValue = gamebitValue;
}

/* Extend objData */
RECOMP_PATCH u32 sfxPlayer_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(SfxPlayer_Data);
}
