#include "modding.h"
#include "recomputils.h"

#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/rand.h"
#include "dll.h"

#include "recomp/dlls/objects/225_SabreBaddie_recomp.h"

typedef struct {
/*000*/ s8 unk0[0x33A - 0];
/*33A*/ s8 unk33A;
/*33B*/ s8 unk33B[0x348 - 0x33B];
/*348*/ s8 unk348; //health?
/*349*/ s8 unk349[0x3B6 - 0x349];
/*3B6*/ s16 unk3B6;
} SabreBaddieState;

// Allows the Test of Character to be completed by changing the flag set when the phantom's health is low (originally by jeebs2kx)
RECOMP_PATCH s32 dll_225_func_1F38(Object* self, SabreBaddieState* state, s32 arg2) {
    SabreBaddieState* state2;

    state2 = self->state;
    
    if (state->unk348 <= 0) {
        return 5;
    }
    
    if (state->unk348 < 5) {
        main_set_bits(0x5B2, 1); //@recomp: flagID changed
        state->unk348 = 1;
        return 3;
    }
    
    if (state->unk33A) {
        if (state->unk348 < rand_next(2, 4)) {
            return 4;
        }
        state2->unk3B6 = 0x12C;
        return 8;
    }
    return 0;
}
