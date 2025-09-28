#include "modding.h"
#include "recomputils.h"

#include "sys/dll.h"
#include "sys/objects.h"

#include "recomp/dlls/_asm/782_recomp.h"

typedef struct {
s8 unk0[0x276 - 0];
s8 unk276;
} WCPushBlockState;

typedef struct {
/*00*/ ObjCreateInfo base;
/*18*/ u8 unk18;
/*19*/ s8 modelIndex;
/*1A*/ s16 unk1A;
} WCPushBlockCreateInfo;

//Prevents crash when the Sun Blocks loads in Walled City (originally by MusicalProgrammer)
RECOMP_PATCH void dll_782_func_18(Object* self, WCPushBlockCreateInfo* createInfo, s32 arg2) {
    WCPushBlockState* state = self->state;
    ObjectHitInfo* hitInfo; //@recomp
    
    self->unk_0x36 = 0;
    self->modelInstIdx = createInfo->modelIndex;

    //@recomp
    hitInfo = self->objhitInfo;
    hitInfo->unk_0xa0 = createInfo->modelIndex;

    state->unk276 = createInfo->unk1A;
}
