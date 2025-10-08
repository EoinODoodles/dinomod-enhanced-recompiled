#include "modding.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "dlls/objects/214_animobj.h"
#include "game/objects/object.h"
#include "sys/controller.h"
#include "sys/main.h"
#include "sys/map.h"
#include "functions.h"
#include "types.h"
#include "dll.h"

#include "recomp/dlls/objects/507_SHqueenearthwalker_recomp.h"

typedef struct {
    u8 unk0;
    u8 unk1;
    u8 unk2;
    u8 unk3;
} SHqueenearthwalker_State;

RECOMP_PATCH void SHqueenearthwalker_update(Object* self) {
    SHqueenearthwalker_State* state;
    s32 sp28;

    state = self->state;
    sp28 = state->unk0;
    self->unk0xaf &= ~8;
    if (self->curModAnimId != 1) {
        func_80023D30(self, 1, 0.0f, 0);
    }
    func_80024108(self, 0.005f, delayByte, 0);
    switch (state->unk0) {

    case 1:
        state->unk0 = 2;
        break;
    case 2:
        if (self->unk0xaf & 1) {
            set_button_mask(0, 0x8000);
            gDLL_3_Animation->vtbl->func17(1, self, -1);
            set_gplay_bitstring(0xBF, 1);
            state->unk0 = 3;
        }
        break;
    case 3:
        if (self->unk0xaf & 4) {
            if (gDLL_1_UI->vtbl->func7(0x66D) != 0) {
                set_button_mask(0, 0x8000);
                state->unk1 += get_gplay_bitstring(0x66D);
                // @recomp: Require all ten white mushrooms instead of just one. (originally by MusicalProgrammer)
                if (state->unk1 < 10) {
                    gDLL_3_Animation->vtbl->func17(3, self, -1);
                } else {
                    state->unk0 = 4U;
                    gDLL_30_Task->vtbl->mark_task_completed(0xB);
                    set_gplay_bitstring(0x8D4, 1);
                }
                set_gplay_bitstring(0x66D, 0);
                set_gplay_bitstring(0xC2, state->unk1);
            } else if (self->unk0xaf & 1) {
                set_button_mask(0, 0x8000);
                gDLL_3_Animation->vtbl->func17(4, self, -1);
            }
        }
        break;
    case 4:
        gDLL_3_Animation->vtbl->func17(2, self, -1);
        break;
    case 5:
        gDLL_3_Animation->vtbl->func17(6, self, -1);
        break;
    case 6:
        gDLL_3_Animation->vtbl->func17(7, self, -1);
        state->unk0 = 7;
        break;
    case 7:
        break;
    default:
        state->unk0 = 1;
        break;
    }
    if (sp28 != state->unk0) {
        set_gplay_bitstring(0xB0, state->unk0);
    }
}
