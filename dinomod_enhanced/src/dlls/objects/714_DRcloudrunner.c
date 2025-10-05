#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/rand.h"
#include "sys/objects.h"
#include "game/objects/object_id.h"
#include "sys/map.h"
#include "functions.h"

#include "dlls/objects/745_DR_Cage.h"
#include "recomp/dlls/_asm/714_recomp.h"

typedef struct {
s32 unk0;
s8 unk4[0x25B - 4];
s8 unk25B;
s8 unk25C[0x272 - 0x25C];
s8 unk272;
s8 unk273[0x910 - 0x273];
s16 unk910;
s8 unk912[0x920 - 0x912];
u8 unk920;
} DRCloudRunnerState;

typedef struct {
ObjCreateInfo base;
s16 unk18;
s16 unk1A;
s16 unk1C;
s16 unk1E;
} DRCloudRunnerCreateInfo;

// Stops the imprisoned CloudRunner from immediately disappearing through the floor in Dragon Rock (originally by MusicalProgrammer)
RECOMP_PATCH s32 dll_714_func_1968(Object* self, DRCloudRunnerState* state, s32 arg2) {
    DRCloudRunnerState* state2;
    u32 new_var;
    DRCloudRunnerCreateInfo* createInfo;
    Object* parent;

    //@recomp: switch off CloudRunner's gravity while in cage
    state->unk0 = 0x200000;

    createInfo = (DRCloudRunnerCreateInfo*)self->createInfo;
    if (state->unk272) {
        func_800267A4(self);
        state->unk25B = 0;
        return 0;
    }
    
    state2 = self->state;
    
    // Randomly call out to player while in cage 
    // (Object parent hierarchy: DR_Cage -> DRPerch -> DR_CloudRunner)
    if (rand_next(0, 120) == 0) {
        parent = (Object*)self->unk0xc4; //DRPerch
        if (parent) {
            parent = (Object*)parent->unk0xc4; //DR_Cage
            if (parent && (parent->id == OBJ_DR_Cage)) {
                gDLL_3_Animation->vtbl->func17(rand_next(0, 1), self, -1);
            }
        }
    } else {
        parent = (Object*)self->unk0xc4; //DRPerch
        if (parent) {
            parent = (Object*)parent->unk0xc4; //DR_Cage
            if (parent && (parent->id == OBJ_DR_Cage) && 
                ((DLL_745_DR_Cage*)parent->dll)->vtbl->func0(parent)) {
                gDLL_3_Animation->vtbl->func17(2, self, -1);
            }
        }
    }
    
    new_var = 4;
    if (get_gplay_bitstring(createInfo->unk1E)) {
        self->unk0xc4 = NULL;
        state2->unk920 = (((new_var * (state2->unk910 > 0)) * 4) & 0x10) | (state2->unk920 & 0xFFEF);
        return 3;
    }
    
    return 0;
}
