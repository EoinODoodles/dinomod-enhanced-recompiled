#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "common.h"

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
/*0E*/ s16 fxCount;             //how many times to spawn the effect
/*10*/ u16 _unk10;              //unused
/*12*/ s16 randomDelay;
/*14*/ s16 toggleGamebit;
/*16*/ s16 disableGamebit;
/*18*/ s16 disabled;
/*1A*/ s16 intervalTimer;
/*1C*/ u8 animCallbackRotate;   //switches particle rotation on/off when used in sequences
} FXEmit_Data;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 activationRange;
/*19*/ s8 bank;
/*1A*/ s16 indexInBank;
/*1C*/ s16 fxCount;
/*1E*/ s16 toggleGamebit;
/*20*/ s16 disableGamebit;
/*22*/ s8 roll;
/*23*/ s8 pitch;
/*24*/ s8 yaw;
/*25*/ s8 rollSpeed;        //animation speed for particle's roll   (0x7F for a slower default speed)
/*26*/ s8 pitchSpeed;       //animation speed for particle's pitch  (0x7F for a slower default speed)
/*27*/ s8 yawSpeed;         //animation speed for particle's yaw    (0x7F for a slower default speed)
/*28*/ u8 flagConfig;       //chooses between 4 partfx flag presets
/*29*/ u8 interval;         //FXEmit activates repeatedly, at multiples of 100 frames 
                            //interval = 0 skips interval behaviour, 0xFF disables FXEmit
/*2A*/ s16 intervalSoundID; //soundID to play during interval activation
} FXEmit_Setup;

extern void FXEmit_emit(Object *self);

/** Prevent default FXEmit particle (small red flare) from appearing on 1st frame of FXEmit being loaded
  * (Fixed by preventing the "control" function from continuing with the rest of its tick if the FXEmit is switched off) */
RECOMP_PATCH void FXEmit_control(Object* self) {
    FXEmit_Data* objdata;
    Vec3f vectorToPlayer;
    s32 _pad;
    FXEmit_Setup* setup;
    Object* player;
    int hideFlashes = recomp_get_config_u32("fxemit_hide_default");

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

        player = get_player();
        if (!player || !setup) {
            return;
        }

        //(Optional) Activate at intervals, playing sound effect
        if (setup->interval && (setup->interval != 0xFF)) {
            if (objdata->intervalTimer <= 0) {
                objdata->disabled = FALSE;
                objdata->intervalTimer = setup->interval * 100;
                if (setup->intervalSoundID) {
                    gDLL_6_AMSFX->vtbl->play(self, setup->intervalSoundID, MAX_VOLUME, NULL, NULL, 0, NULL);
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
        if (((objdata->toggleGamebit != NO_GAMEBIT) && main_get_bits(objdata->toggleGamebit) == FALSE) 
            || objdata->disabled) {
          return;
        }

        //Check if should be disabled (@bug?: continues with this tick when disabled, causing 1 frame flash)
        if (objdata->disableGamebit != NO_GAMEBIT && main_get_bits(objdata->disableGamebit)) {
            objdata->disabled = TRUE;
        }
        if (setup->interval == 0xFF) {
            objdata->disabled = TRUE;
        }

        //@recomp: return if switched off
        if (hideFlashes && objdata->disabled){
            return;
        }

        if ((objdata->fxCount >= 0) || ((objdata->fxCount < 0) && (self->unkDC <= 0))) {
            vectorToPlayer.f[0] = self->globalPosition.x - player->globalPosition.x;
            vectorToPlayer.f[1] = self->globalPosition.y - player->globalPosition.y;
            vectorToPlayer.f[2] = self->globalPosition.z - player->globalPosition.z;
            if (objdata->fxCount == 0) {
                objdata->disabled = TRUE;
            }

            //@recomp: return if switched off
            if (hideFlashes && objdata->disabled){
                return;
            }

            if (VECTOR_MAGNITUDE(vectorToPlayer) <= objdata->activationRange || objdata->activationRange == 0.0f) {
                FXEmit_emit(self);
            }

            self->unkDC = -objdata->fxCount;
            return;
        }
        if (objdata->fxCount < 0) {
            if (self->unkDC > 0) {
                self->unkDC -= gUpdateRate;
            }
        }
    }
}
