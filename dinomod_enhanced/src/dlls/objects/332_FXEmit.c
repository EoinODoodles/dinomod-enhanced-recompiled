#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "configs.h"

#include "common.h"
#include "game/objects/setups/118_FXEmit.h"

#include "recomp/dlls/objects/332_FXEmit_recomp.h"

#define USE_DEFAULT_SPEED 0x7F

typedef struct {
/*00*/ f32 activationRange;
/*04*/ f32 translateX;
/*08*/ s16 bank;                //where the effect is stored 
                                //0: partfx DLL (#17)
                                //1) modgfx DLLs
                                //2) projgfx DLLs
/*0A*/ s16 indexInBank;         //index of the effect within its bank
/*0C*/ s16 defaultFXIndex;      //only used if fxCount <= 0, always set to 0
/*0E*/ s16 fxRate;              //when positive: how many times the emitter runs per tick
                                //when negative: number of frames to waits between single emit
/*10*/ u16 _unk10;              //unused
/*12*/ s16 randomDelay;
/*14*/ s16 toggleGamebit;
/*16*/ s16 disableGamebit;
/*18*/ s16 disabled;
/*1A*/ s16 intervalTimer;
/*1C*/ u8 animCallbackRotate;   //switches particle rotation on/off when used in sequences
} FXEmit_Data;

extern void FXEmit_emit(Object *self);

/** Prevent default FXEmit particle (small red flare) from appearing on 1st frame of FXEmit being loaded
  * (Fixed by preventing the "control" function from continuing with the rest of its tick if the FXEmit is switched off) */
RECOMP_PATCH void FXEmit_control(Object* self) {
    FXEmit_Data* objdata;
    Vec3f vectorToPlayer;
    s32 _pad;
    FXEmit_Setup* setup;
    Object* player;
    int hideFlashes = configs_GetHideDefaultFXEmits();

    objdata = (FXEmit_Data*)self->data;
    setup = (FXEmit_Setup*)self->setup;
    
    if (objdata->randomDelay) {
        objdata->randomDelay -= (s16)gUpdateRateF;
        if (objdata->randomDelay < 0) {
            objdata->randomDelay = 0;
        }
    } else {
        self->srt.transl.x += self->velocity.x * gUpdateRateF;
        self->srt.transl.y += self->velocity.y * gUpdateRateF;
        self->srt.transl.z += self->velocity.z * gUpdateRateF;
        self->globalPosition.x = self->srt.transl.x;
        self->globalPosition.y = self->srt.transl.y;
        self->globalPosition.z = self->srt.transl.z;

        player = objGetPlayer();
        if (!player || !setup) {
            return;
        }

        //(Optional) Activate at intervals, playing sound effect
        if (setup->interval && (setup->interval != 0xFF)) {
            if (objdata->intervalTimer <= 0) {
                objdata->disabled = FALSE;
                objdata->intervalTimer = setup->interval * 100;
                if (setup->intervalSoundID) {
                    // @recomp: Don't play interval sfx if not enabled
                    if ((objdata->toggleGamebit == NO_GAMEBIT || mainGetBits(objdata->toggleGamebit))
                            && (objdata->disableGamebit == NO_GAMEBIT || mainGetBits(objdata->disableGamebit) == FALSE)) {
                        dll_amSfx->Play(self, setup->intervalSoundID, MAX_VOLUME, NULL, NULL, 0, NULL);
                    }
                }
            } else {
                objdata->disabled = TRUE;
            }
            objdata->intervalTimer -= gUpdateRate;
        }

        //Handle rotations
        if (setup->yawSpeed == USE_DEFAULT_SPEED) {
            self->srt.yaw += gUpdateRate * 10;
        } else {
            self->srt.yaw += setup->yawSpeed * gUpdateRate * 100;
        }

        if (setup->pitchSpeed == USE_DEFAULT_SPEED) {
            self->srt.pitch += gUpdateRate * 10;
        } else {
            self->srt.pitch += setup->pitchSpeed * gUpdateRate * 100;
        }

        if (setup->rollSpeed == USE_DEFAULT_SPEED) {
            self->srt.roll += gUpdateRate * 10;
        } else {
            self->srt.roll += setup->rollSpeed * gUpdateRate * 100;
        }

        //Bail if not enabled
        if (((objdata->toggleGamebit != NO_GAMEBIT) && mainGetBits(objdata->toggleGamebit) == FALSE) 
            || objdata->disabled) {
          return;
        }

        //Check if should be disabled (@bug?: continues with this tick when disabled, causing 1 frame flash)
        if (objdata->disableGamebit != NO_GAMEBIT && mainGetBits(objdata->disableGamebit)) {
            objdata->disabled = TRUE;
        }
        if (setup->interval == 0xFF) {
            objdata->disabled = TRUE;
        }

        if ((objdata->fxRate >= 0) || ((objdata->fxRate < 0) && (self->unkDC <= 0))) {
            VECTOR_SUBTRACT(self->globalPosition, player->globalPosition, vectorToPlayer);
            if (objdata->fxRate == 0) {
                objdata->disabled = TRUE;
            }

            //@recomp: fix FX flashing up for a single frame
            if (hideFlashes && objdata->disabled && (setup->interval != 0xFF) && (objdata->bank == 0)){
                return;
            }

            if (VECTOR_MAGNITUDE(vectorToPlayer) <= objdata->activationRange || objdata->activationRange == 0.0f) {
                FXEmit_emit(self);
            }

            self->unkDC = -objdata->fxRate;
            return;
        }
        if (objdata->fxRate < 0) {
            if (self->unkDC > 0) {
                self->unkDC -= gUpdateRate;
            }
        }
    }
}
