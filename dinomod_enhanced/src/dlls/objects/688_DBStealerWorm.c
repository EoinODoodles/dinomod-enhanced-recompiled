#include "modding.h"

#include "dll.h"
#include "dlls/engine/33_BaddieControl.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/rand.h"

#include "recomp/dlls/objects/688_DBstealerworm_recomp.h"

typedef struct {
    f32 resetTimer;
    f32 resetTimeThreshold;
    f32 roarSoundTimer;
    f32 roarSoundInterval;
    u32 flags: 8;
    u32 turnFlags : 8;
    u32 unusedBit : 1;
    u32 flagChangeTarget : 1;
    Object* stolenEgg;
    u8 targetIdx;
    f32 aggroCounter;
} DBStealerWorm_DataActual;

typedef enum {
    DBStealerWorm_ASTATE_0_Pop_Out_of_Ground,
    DBStealerWorm_ASTATE_1_Burst_Into_Ground,
    DBStealerWorm_ASTATE_2_Bite_Attack,
    DBStealerWorm_ASTATE_3_Stand_Still,
    DBStealerWorm_ASTATE_4_Stand_and_Spit,
    DBStealerWorm_ASTATE_5_Hit,
    DBStealerWorm_ASTATE_6_Dying,
    DBStealerWorm_ASTATE_7_Run_to_Object,
    DBStealerWorm_ASTATE_8_Pick_Up_Egg,
    DBStealerWorm_ASTATE_9_Throw_Egg
} DBStealerWorm_AnimStates;

typedef enum {
    DBStealerWorm_LSTATE_0_Top,
    DBStealerWorm_LSTATE_1_Hit,
    DBStealerWorm_LSTATE_2_Dying,
    DBStealerWorm_LSTATE_3_Dead,
    DBStealerWorm_LSTATE_4_Dormant,
    DBStealerWorm_LSTATE_5_Engage
} DBStealerWorm_LogicStates;

extern u32 dBattleSounds[];

/*
    Fix a bug where worm's chirping could play repeatedly on successive frames, potentially crashing the game.
*/
RECOMP_PATCH void DBStealerWorm_func_BA0(Object* self, Baddie* baddie, ObjFSA_Data* fsa) {
    DBStealerWorm_DataActual* objData;
    Object* target;
    Vec3f d;
    f32 playerDistance;
    f32 searchDistance;
    Object* player;
    Baddie_Setup* objSetup;

    objData = baddie->objdata;
    searchDistance = 100.0f;
    objSetup = (Baddie_Setup*)self->setup;

    target = gDLL_33_BaddieControl->vtbl->func17(self, fsa, baddie->unk3E2, M_180_DEGREES);
    if (target == NULL) {
        target = objGetNearestTypeTo(OBJTYPE_38, self, &searchDistance);
    }
    
    if ((target != NULL) && !(baddie->unk3B0 & 4)) {
        gDLL_33_BaddieControl->vtbl->func9(self, fsa, &baddie->unk34C, baddie->unk39E, NULL, 0, 0, 8, -1);
        fsa->unk33D = 0;
        
        fsa->target = target;

        if (baddie->fsa.target) {}

        baddie->unk3B6 = TRUE;
        return;
    }

    objData->turnFlags = 0;
    
    if (objData->resetTimer > 0.0f) {
        if ((fsa->logicState != DBStealerWorm_LSTATE_3_Dead) || (baddie->unk3B0 & 1)) {
            if (objData->resetTimer >= objData->resetTimeThreshold) {
                gDLL_33_BaddieControl->vtbl->func9(self, fsa, &baddie->unk34C, baddie->unk39E, NULL, 0, 0, 8, -1);
                objData->resetTimer = 0.0f;
                fsa->hitpoints = objSetup->quarterHitpoints * 4;
                self->srt.transl.x = objSetup->base.x;
                self->srt.transl.y = objSetup->base.y;
                self->srt.transl.z = objSetup->base.z;
                self->srt.yaw = objSetup->unk2A << 8;
                return;
            }
            
            objData->resetTimer += gUpdateRateF;
        }
    } else {
        player = objGetPlayer();
        if (player != NULL) {
            d.f[0] = player->globalPosition.f[0] - self->globalPosition.f[0];
            d.f[1] = player->globalPosition.f[1] - self->globalPosition.f[1];
            d.f[2] = player->globalPosition.f[2] - self->globalPosition.f[2];
            playerDistance = sqrtf(SQ(d.f[0]) + SQ(d.f[1]) + SQ(d.f[2]));
        } else {
            playerDistance = 10000.0f;
        }
        
        /* @bug: roarSoundTimer's value can increase way beyond roarSoundInterval's value while the player is distant.
            This causes the roar sound to play every tick when the player approaches, until roarSoundInterval's 
            value catches up with and overtakes roarSoundTimer's value. */
        if ((objData->roarSoundTimer > objData->roarSoundInterval) && (playerDistance < 400.0f)) {
            dll_amSfx->Play(self, dBattleSounds[1], 0x1E, NULL, NULL, 0, NULL);
            
            //@recomp: fix sound timer bug
            objData->roarSoundTimer = 0;
            objData->roarSoundInterval = mathRnd(50, 250);
        }
        
        objData->roarSoundTimer += gUpdateRateF;
    }
}
