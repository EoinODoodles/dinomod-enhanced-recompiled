#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "sys/main.h"
#include "sys/dll.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "functions.h"

#include "recomp/dlls/_asm/638_recomp.h"

extern s32 dll_638_func_62C(Object* self, s32 arg1, s32 arg2, s32 arg3);

extern Texture* _data_0;

/** Removes progress-breaking debug code that automatically granted the player CloudRunner Fortress' SpellStone (originally by MusicalProgrammer) */
RECOMP_PATCH void dll_638_func_18(Object* self, s32 arg1, s32 arg2) {
    u8 mapSetup;

    obj_add_object_type(self, 0xA);
    _data_0 = queue_load_texture_proxy(0x46C);
    self->unk0xbc = (void*)dll_638_func_62C;
    gDLL_29_Gplay->vtbl->func_139C(self->mapID, 1);
    mapSetup = gDLL_29_Gplay->vtbl->func_143C(self->mapID);

    switch (mapSetup) { 
        case 0:
            func_80000860(self, self, 261, 0);
            func_80000860(self, self, 262, 0);
            func_80000860(self, self, 263, 0);
            gDLL_29_Gplay->vtbl->func_16C4(0x32, 2, 1);
            break;
        case 1:
            func_80000860(self, self, 415, 0);
            // set_gplay_bitstring(0x2E8, 1); //@recomp: don't set flag
            break;
        case 2:
            func_80000860(self, self, 415, 0);
            set_gplay_bitstring(0x83A, 1);
            set_gplay_bitstring(0x777, 1);
            break;
        case 3:
            func_80000860(self, self, 415, 0);
            set_gplay_bitstring(0x7BD, 1);
            set_gplay_bitstring(0x777, 1);
            break;
    }
    self->unk0xb0 |= 0x6000;
}
