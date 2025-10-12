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
#include "recomp/dlls/objects/714_DR_CloudRunner_recomp.h"

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
} DRCloudRunner_Data;

typedef struct {
ObjSetup base;
s16 unk18;
s16 unk1A;
s16 unk1C;
s16 unk1E;
} DRCloudRunner_Setup;

// Stops the imprisoned CloudRunner from immediately disappearing through the floor in Dragon Rock (originally by MusicalProgrammer)
RECOMP_PATCH s32 dll_714_func_1968(Object* self, DRCloudRunner_Data* objdata, s32 arg2) {
    DRCloudRunner_Data* objdata2;
    u32 new_var;
    DRCloudRunner_Setup* setup;
    Object* parent;

    //@recomp: switch off CloudRunner's gravity while in cage
    objdata->unk0 = 0x200000;

    setup = (DRCloudRunner_Setup*)self->setup;
    if (objdata->unk272) {
        func_800267A4(self);
        objdata->unk25B = 0;
        return 0;
    }
    
    objdata2 = self->data;
    
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
    if (main_get_bits(setup->unk1E)) {
        self->unk0xc4 = NULL;
        objdata2->unk920 = (((new_var * (objdata2->unk910 > 0)) * 4) & 0x10) | (objdata2->unk920 & 0xFFEF);
        return 3;
    }
    
    return 0;
}
