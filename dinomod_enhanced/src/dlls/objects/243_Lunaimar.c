#include "modding.h"

#include "dlls/engine/18_objfsa.h"
#include "dlls/engine/33.h"
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

extern void dll_243_func_11C0(Object *self, DLL33_Data *arg1, ObjFSA_Data *fsa);

RECOMP_PATCH void dll_243_func_C44(Object *self, DLL33_Data *arg1, ObjFSA_Data *fsa) {
    Lunaimar_ActualData *objdata;
    Object *sidekick;
    Vec3f sp44;
    f32 sp40;
    s32 *sp3C;

    objdata = (Lunaimar_ActualData*)arg1->unk3F4;
    sidekick = get_sidekick();
    sp3C = func_800348A0(self, 0, 0);
    objdata->unk12 += 0x1000;
    *sp3C = (s32) ((fsin16_precise(objdata->unk12) + 1.0f) * 127.0f);
    // @recomp: Sidekick null check
    if (sidekick != NULL) {
        sp44.f[0] = sidekick->positionMirror.x - self->positionMirror.x;
        sp44.f[1] = sidekick->positionMirror.y - self->positionMirror.y;
        sp44.f[2] = sidekick->positionMirror.z - self->positionMirror.z;
        sp40 = sqrtf(SQ(sp44.f[0]) + SQ(sp44.f[1]) + SQ(sp44.f[2]));
    }
    if (sidekick != NULL && ((DLL_ISidekick*)sidekick->dll)->vtbl->func24(sidekick) != 0 && (sp40 < (f32) arg1->unk3E2)) {
        arg1->unk3B2 |= 4;
    } else {
        arg1->unk3B2 &= ~0x4;
    }
    if (arg1->unk3B2 & 4) {
        fsa->target = sidekick;
    } else {
        fsa->target = get_player();
    }
    dll_243_func_11C0(self, arg1, fsa);
    gDLL_33->vtbl->func10(self, fsa, 0.0f, -1);
    gDLL_18_objfsa->vtbl->func11(self, fsa, gUpdateRateF, 5);
    arg1->unk3AC = self->unkC0;
    self->unkC0 = NULL;
    gDLL_18_objfsa->vtbl->tick(self, fsa, gUpdateRateF, gUpdateRateF, sAnimStateCallbacks, sLogicStateCallbacks);
    self->unkC0 = arg1->unk3AC;
}
