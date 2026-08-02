#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "dlls/engine/33_BaddieControl.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "sys/rand.h"

#include "recomp/dlls/objects/222_SnowWormSmall_recomp.h"

typedef struct {
    f32 soundTimer;
    f32 soundTimerThreshold;
    u8 flags;
    u8 battleFlags;
} SnowWormSmall_DataActual;

extern u32 dBattleSounds[];

/*
    Fix a bug where worm's chirping could play repeatedly on successive frames, potentially crashing the game.
    This was originally fixed by MusicalProgrammer by removing the sound call, 
    but this new version of the fix instead fixes the sound timer's logic.
*/
RECOMP_PATCH void SnowWormSmall_func_D0C(Object* self, Baddie* baddie, ObjFSA_Data* fsa) {
    SnowWormSmall_DataActual* objData;
    Object* target;
    Vec3f d;
    Object* player;
    f32 distance;

    objData = baddie->objdata;
    
    target = gDLL_33_BaddieControl->vtbl->func17(self, fsa, baddie->unk3E2, M_180_DEGREES);
    if ((target != NULL) && !(baddie->unk3B0 & 4)) {
        gDLL_33_BaddieControl->vtbl->func9(self, fsa, &baddie->unk34C, baddie->unk39E, NULL, 0, 0, 8, -1);
        fsa->unk33D = 0;
        fsa->target = target;
        baddie->unk3B6 = 1;
        return;
    }
    
    player = objGetPlayer();
    if (player != NULL) {
        d.f[0] = player->globalPosition.x - self->globalPosition.x;
        d.f[1] = player->globalPosition.y - self->globalPosition.y;
        d.f[2] = player->globalPosition.z - self->globalPosition.z;
        distance = sqrtf(SQ(d.f[0]) + SQ(d.f[1]) + SQ(d.f[2]));
    } else {
        distance = 10000.0f;
    }
    
    //Play sound when nearby 
    /* @bug: soundTimer's value can increase way beyond soundTimerThreshold's value while the player is distant.
        This causes the chirp sound to play every tick when the player approaches, until soundTimerThreshold's 
        value catches up with and overtakes soundTimer's value. */
    if ((objData->soundTimer > objData->soundTimerThreshold) && (distance < 400.0f)) {
        dll_amSfx->Play(self, dBattleSounds[1], 0x1E, NULL, NULL, 0, NULL);

        //@recomp: fix bug where the sound can play in rapid succession
        objData->soundTimer = 0;
        objData->soundTimerThreshold = mathRnd(50, 250);
    }
    objData->soundTimer += gUpdateRateF;
}
