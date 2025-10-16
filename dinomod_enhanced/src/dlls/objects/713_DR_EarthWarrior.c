#include "modding.h"

#include "common.h"
#include "sys/objanim.h"
#include "game/gametexts.h"
#include "dlls/objects/214_animobj.h"

#include "recomp/dlls/objects/713_DR_EarthWarrior_recomp.h"

typedef struct {
    u8 _unk0[0xA58 - 0x0];
    u16 unkA58;
    u8 _unkA5A[0xA62 - 0xA5A];
    s8 unkA62;
    s8 unkA63;
} DLL713_Data;

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
