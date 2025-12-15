#include "modding.h"

#include "PR/ultratypes.h"
#include "dlls/objects/common/sidekick.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "dll.h"

#include "recomp/dlls/objects/501_SHtricky_recomp.h"

typedef struct {
    u8 unk0;
} SHtricky_Data;

RECOMP_PATCH void SHtricky_setup(Object* self, ObjSetup* setup, s32 arg2) {
    SHtricky_Data *objdata;

    objdata = (SHtricky_Data*)self->data;
    
    // @recomp: Stop Tricky from repeating: "That's my mom!" when arriving in SwapStone Hollow (originally by MusicalProgrammer)
    if (main_get_bits(BIT_A1)) {
        objdata->unk0 = 3;
    } else {
        main_set_bits(BIT_Play_Seq_00D1, 0);
        objdata->unk0 = 0;
    }

    self->unkB0 |= 0x2000;
}

RECOMP_PATCH void SHtricky_control(Object *self) {
    SHtricky_Data *objdata;
    Object *sidekick;

    objdata = (SHtricky_Data*)self->data;
    sidekick = get_sidekick();
    //@recomp: prevent crash if Tricky isn't loaded
    if (sidekick == NULL){
        return;
    }

    switch (objdata->unk0) {
    case 0:
        if (main_get_bits(BIT_Play_Seq_00D1)) {
            gDLL_3_Animation->vtbl->func17(3, sidekick, -1);
            objdata->unk0 = 1;
        } 
        break;
    case 1:
        objdata->unk0 = 2;
        break;
    case 2:
        ((DLL_ISidekick*)sidekick->dll)->vtbl->func22(sidekick, self);
        objdata->unk0 = 3;
        break;
    case 3:
        break;
    }
}
