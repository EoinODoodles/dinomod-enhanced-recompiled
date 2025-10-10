#include "modding.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "game/gamebits.h"
#include "dlls/objects/214_animobj.h"
#include "game/objects/object.h"
#include "sys/controller.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/objanim.h"
#include "functions.h"
#include "types.h"
#include "dll.h"

#include "recomp/dlls/objects/507_SHqueenearthwalker_recomp.h"

typedef struct {
    u8 questProgress;
    u8 eatenWhiteMushrooms;
    u8 unk2;
    u8 unk3;
} SHqueenearthwalker_State;

RECOMP_PATCH void SHqueenearthwalker_update(Object* self) {
    SHqueenearthwalker_State* state;
    s32 prevQuestProgress;

    state = self->state;
    prevQuestProgress = state->questProgress;
    self->unk0xaf &= ~8;
    if (self->curModAnimId != 1) {
        func_80023D30(self, 1, 0.0f, 0);
    }
    func_80024108(self, 0.005f, delayByte, 0);
    switch (state->questProgress) {

    case 1:
        state->questProgress = 2;
        break;
    case 2:
        if (self->unk0xaf & 1) {
            set_button_mask(0, 0x8000);
            gDLL_3_Animation->vtbl->func17(1, self, -1);
            main_set_bits(BIT_SH_Move_Thorntail_Blocking_Hollow_Log, 1);
            state->questProgress = 3;
        }
        break;
    case 3:
        if (self->unk0xaf & 4) {
            if (gDLL_1_UI->vtbl->func7(BIT_Inventory_White_Mushrooms) != 0) {
                set_button_mask(0, 0x8000);
                state->eatenWhiteMushrooms += main_get_bits(BIT_Inventory_White_Mushrooms);
                // @recomp: Require all ten white mushrooms instead of just one. (originally by MusicalProgrammer)
                if (state->eatenWhiteMushrooms < 10) {
                    gDLL_3_Animation->vtbl->func17(3, self, -1);
                } else {
                    state->questProgress = 4U;
                    gDLL_30_Task->vtbl->mark_task_completed(0xB);
                    main_set_bits(BIT_SH_Move_Thorntail_Blocking_Swapstone, 1);
                }
                main_set_bits(BIT_Inventory_White_Mushrooms, 0);
                main_set_bits(BIT_SH_Queen_EW_White_Mushrooms_Eaten, state->eatenWhiteMushrooms);
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
        state->questProgress = 7;
        break;
    case 7:
        break;
    default:
        state->questProgress = 1;
        break;
    }
    if (prevQuestProgress != state->questProgress) {
        main_set_bits(BIT_SH_Queen_EW_Quest_Progress, state->questProgress);
    }
}
