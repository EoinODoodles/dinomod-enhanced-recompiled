#include "modding.h"

#include "PR/ultratypes.h"
#include "game/objects/object.h"
#include "recomputils.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "dll.h"

#include "recomp/dlls/objects/637_VFP_SpellPlace_recomp.h"

// size:0x6
typedef struct {
    s16 unk0;
    s16 unk2;
    s8 unk4;
} VFP_SpellPlace_State;

RECOMP_PATCH void VFP_SpellPlace_do_act1(Object* self) {
    VFP_SpellPlace_State* state;
    s16 bits2;
    s16 bits1;

    state = (VFP_SpellPlace_State*)self->state;
    
    bits2 = get_gplay_bitstring(state->unk2);
    bits1 = get_gplay_bitstring(state->unk0);
    
    if ((bits1 == 0) && (bits2 != 0)) {
        self->unk0xaf &= ~0x8;
        
        // @recomp: Accept DIM's activated SpellStone instead of the unactivated one
        if ((bits2 != 0) && (gDLL_1_UI->vtbl->func7(0x22B) != 0)) {
            set_gplay_bitstring(state->unk0, 1);
            state->unk4 = 1;
            self->unk0xaf |= 8;
        }
    }
}
