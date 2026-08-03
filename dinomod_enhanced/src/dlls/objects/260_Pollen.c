#include "game/objects/object_id.h"
#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "dlls/objects/237_WG_PollenCannon.h"
#include "dlls/objects/260_Pollen.h"
#include "dlls/objects/261_PollenFragment.h"
#include "sys/main.h"
#include "sys/objects.h"

#include "recomp/dlls/objects/260_Pollen_recomp.h"

extern void Pollen_create_fragments(Object* self);

/* Avoid crashes when the Pollen's PollenCannon unloads before it does */
RECOMP_PATCH void Pollen_control(Object* self) {
    Object* pollenCannon;
    Baddie *pollenCannonBaddie;
    Pollen_Data* objData;
    u32 index;
    s32 count;
    PollenCannonUnk3F4* cannonObjData;

    objData = self->data;
    
    //@recomp: NULL checks
    pollenCannon = self->unkC4;
    pollenCannonBaddie = (pollenCannon != NULL) ? pollenCannon->data : NULL;
    cannonObjData = (pollenCannonBaddie != NULL) ? pollenCannonBaddie->objdata : NULL;
    
    if (objData->unk12 != 0) {
        objData->unk12--;
    }
    
    //Apply gravity and move
    self->velocity.y -= 0.045f * gUpdateRateF;
    objMove(self, self->velocity.x * gUpdateRateF, self->velocity.y * gUpdateRateF, self->velocity.z * gUpdateRateF);
    
    //Objhits
    func_80026128(self, 0x15, 1, 0);
    func_80026940(self, 7);
    func_8002674C(self);

    //React to hitting the player or sidekick
    if ((self->objhitInfo != NULL) && //NULL check
        self->objhitInfo->unk48 && 
        ((objGetPlayer() == self->objhitInfo->unk48) || (objGetSidekick() == self->objhitInfo->unk48))
    ) {
        camUseShake();
        camSetShakeOffset(1.0f);
        dll_amSfx->Play(self, SOUND_AFF_Gas_Disperse_Burst, MAX_VOLUME, NULL, NULL, 0, NULL);
        self->opacity = 0;
        func_800267A4(self);
    }

    //Create gas particles
    index = 3;
    while (index--) {
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_4BA, NULL, 1, -1, NULL);
    }
    
    //Destroy self after colliding
    if (self->opacity == 0) {
        //@recomp: NULL checks
        if ((pollenCannon != NULL) && (cannonObjData != NULL)) {
            if (pollenCannon->id == OBJ_WG_PollenCannon) {
                index = cannonObjData->unk6_0;
                while (index--) {
                    if (self == cannonObjData->unk8[index]) {
                        cannonObjData->unk8[index] = cannonObjData->unk8[--cannonObjData->unk6_0];
                    }
                } 
            }
        }
        
        Pollen_create_fragments(self);
        dll_amSfx->Play(self, SOUND_722_Impact_Wobble, 0x40, NULL, NULL, 0, NULL);
        objFreeObject(self);
    }
}
