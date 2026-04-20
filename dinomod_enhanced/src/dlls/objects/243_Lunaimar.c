#include "modding.h"

#include "dlls/engine/18_objfsa.h"
#include "dlls/engine/33_BaddieControl.h"
#include "dlls/objects/common/sidekick.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objanim.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "dll.h"
#include "unktypes.h"

#include "recomp/dlls/objects/243_Lunaimar_recomp.h"

typedef struct {
    f32 unk0;
    f32 unk4;
    f32 unk8;
    f32 unkC;
    u8 unk10;
    s16 unk12;
    Object *unk14;
} Lunaimar_ActualData;


extern ObjFSA_StateCallback sAnimStateCallbacks[6];
extern ObjFSA_StateCallback sLogicStateCallbacks[6];

extern void dll_243_func_11C0(Object *self, Baddie *baddie, ObjFSA_Data *fsa);

RECOMP_PATCH void dll_243_func_C44(Object *self, Baddie *baddie, ObjFSA_Data *fsa) {
    Lunaimar_ActualData *objdata;
    Object *sidekick;
    Vec3f delta;
    f32 sidekickDistance;
    TextureAnimator *texAnimator;

    objdata = (Lunaimar_ActualData*)baddie->objdata;
    sidekick = get_sidekick();
    texAnimator = func_800348A0(self, 0, 0);
    objdata->unk12 += 0x1000;
    texAnimator->frame = (s32) ((fsin16_precise(objdata->unk12) + 1.0f) * 127.0f);

    // @recomp: Sidekick null check
    if (sidekick != NULL) {
        delta.f[0] = sidekick->globalPosition.x - self->globalPosition.x;
        delta.f[1] = sidekick->globalPosition.y - self->globalPosition.y;
        delta.f[2] = sidekick->globalPosition.z - self->globalPosition.z;
        sidekickDistance = sqrtf(SQ(delta.f[0]) + SQ(delta.f[1]) + SQ(delta.f[2]));
    }
    if (sidekick != NULL && 
        ((DLL_ISidekick*)sidekick->dll)->vtbl->func24(sidekick) != 0 && 
        (sidekickDistance < baddie->unk3E2)
    ) {
        baddie->unk3B2 |= 4;
    } else {
        baddie->unk3B2 &= ~0x4;
    }

    if (baddie->unk3B2 & 4) {
        fsa->target = sidekick;
    } else {
        fsa->target = get_player();
    }

    dll_243_func_11C0(self, baddie, fsa);
    gDLL_33_BaddieControl->vtbl->func10(self, fsa, 0.0f, -1);
    gDLL_18_objfsa->vtbl->turn_to_target(self, fsa, gUpdateRateF, 5);
    baddie->unk3AC = self->unkC0;
    self->unkC0 = NULL;
    gDLL_18_objfsa->vtbl->tick(self, fsa, gUpdateRateF, gUpdateRateF, sAnimStateCallbacks, sLogicStateCallbacks);
    self->unkC0 = baddie->unk3AC;
}
