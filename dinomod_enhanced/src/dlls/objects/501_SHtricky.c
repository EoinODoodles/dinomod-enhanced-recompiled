#include "modding.h"

#include "PR/ultratypes.h"
#include "dlls/objects/common/sidekick.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "dll.h"

#include "recomp/dlls/objects/501_SHtricky_recomp.h"

typedef struct {
    u8 unk0;
} SHtricky_State;

RECOMP_PATCH void SHtricky_create(Object* self, ObjCreateInfo* createInfo, s32 arg2) {
    SHtricky_State *state;

    state = (SHtricky_State*)self->state;
    
    // @recomp: Stop Tricky from repeating: "That's my mom!" when arriving in SwapStone Hollow (originally by MusicalProgrammer)
    if (get_gplay_bitstring(0xA1) != 0) {
        state->unk0 = 3;
    } else {
        set_gplay_bitstring(0xC3, 0);
        state->unk0 = 0;
    }

    self->unk0xb0 |= 0x2000;
}
