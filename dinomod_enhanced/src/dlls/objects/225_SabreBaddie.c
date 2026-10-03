#include "modding.h"
#include "recomputils.h"

#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/rand.h"
#include "dll.h"

#include "recomp/dlls/objects/225_SabreBaddie_recomp.h"

typedef enum {
    SabreBaddie_LSTATE_0,
    SabreBaddie_LSTATE_1,
    SabreBaddie_LSTATE_2,
    SabreBaddie_LSTATE_3,
    SabreBaddie_LSTATE_4,
    SabreBaddie_LSTATE_5,
    SabreBaddie_LSTATE_6,
    SabreBaddie_LSTATE_7
} SabreBaddie_LogicStates;

// Allows the Test of Character to be completed by changing the flag set when the phantom's health is low (originally by jeebs2kx)
RECOMP_PATCH s32 SabreBaddie_logicState1(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie = self->data;
    
    if (fsa->hitpoints <= 0) {
        return FSA_NEXTSTATE_SYNC(SabreBaddie_LSTATE_4);
    }
    
    if (fsa->hitpoints < 5) {
        mainSetBits(0x5B2, 1); //@recomp: flagID changed
        fsa->hitpoints = 1;
        return FSA_NEXTSTATE_SYNC(SabreBaddie_LSTATE_2);
    }
    
    if (fsa->unk33A != 0) {
        if (fsa->hitpoints < mathRnd(2, 4)) {
            return FSA_NEXTSTATE_SYNC(SabreBaddie_LSTATE_3);
        } else {
            baddie->unk3B6 = 300;
            return FSA_NEXTSTATE_SYNC(SabreBaddie_LSTATE_7);
        }
    }
    
    return 0;
}
