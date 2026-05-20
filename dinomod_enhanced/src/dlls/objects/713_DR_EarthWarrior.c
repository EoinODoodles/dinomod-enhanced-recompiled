#include "modding.h"

#include "common.h"
#include "game/gametexts.h"
#include "sys/gfx/animseq.h"
#include "sys/objanim.h"
#include "sys/segment_1050.h"

#include "recomp/dlls/objects/713_DR_EarthWarrior_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 rotation;
/*19*/ s8 unk19;
} DRearthwalk_Setup;

typedef struct {
    u8 _unk0[0xA58 - 0x0];
    u16 unkA58;
    u8 _unkA5A[0xA62 - 0xA5A];
    s8 unkA62;
    s8 unkA63;
} DLL713_Data;

/** Allow a summoned Earth Warrior to be ridden if he was already brought to the surface. */
RECOMP_HOOK_DLL(dll_713_setup) void dll_713_setup_hook(Object* self, DRearthwalk_Setup* setup, s32 arg2) {
    if (main_get_bits(BIT_656)) {
        setup->unk19 = 0;
    }
}

//Stops the shackled EarthWalker from going dark too early when entering tunnels (originally by MusicalProgrammer)
RECOMP_PATCH s32 dll_713_func_32EC(Object* self, u8 arg1) {
    DLL713_Data* objData;

    objData = self->data;
    switch (arg1) {
        case 5:
            objData->unkA58 |= 0x80;
            objData->unkA62 = 0xA;
            //Set Mind Read text
            gDLL_22_Subtitles->vtbl->func_21C0(self->id, GAMETEXT_0CF_DR_Mind_Read_messages_4);
            break; //@recomp: don't fallthrough
        case 1:
            objData->unkA62 = 2;
            func_80000450(self, self, 0x22C, 0, 0, 0);
            break;
        case 2:
            func_80000450(self, self, 0x22E, 0, 0, 0);
            objData->unkA58 &= 0xFF7F;
            //Set Mind Read text
            gDLL_22_Subtitles->vtbl->func_21C0(self->id, GAMETEXT_0D0_DR_Mind_Read_messages_5);
    }

    return 1;
}
