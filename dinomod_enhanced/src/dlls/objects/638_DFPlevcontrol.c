#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/dll.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "functions.h"

#include "recomp/dlls/objects/638_DFPlevcontrol_recomp.h"

extern s32 dll_638_func_62C(Object* self, s32 arg1, s32 arg2, s32 arg3);

extern Texture* _data_0;

/** Removes progress-breaking debug code that automatically granted the player CloudRunner Fortress' SpellStone (originally by MusicalProgrammer) */
RECOMP_PATCH void dll_638_setup(Object* self, ObjSetup *setup, s32 arg2) {
    u8 mapSetup;

    obj_add_object_type(self, 0xA);
    _data_0 = queue_load_texture_proxy(0x46C);
    self->unk0xbc = (void*)dll_638_func_62C;
    gDLL_29_Gplay->vtbl->set_map_setup(self->mapID, 1);
    mapSetup = gDLL_29_Gplay->vtbl->get_map_setup(self->mapID);

    switch (mapSetup) { 
        case 0:
            func_80000860(self, self, 261, 0);
            func_80000860(self, self, 262, 0);
            func_80000860(self, self, 263, 0);
            gDLL_29_Gplay->vtbl->set_obj_group_status(0x32, 2, 1);
            break;
        case 1:
            func_80000860(self, self, 415, 0);
            // main_set_bits(BIT_SpellStone_CRF, 1); //@recomp: don't set flag
            break;
        case 2:
            func_80000860(self, self, 415, 0);
            main_set_bits(BIT_SpellStone_BWC, 1);
            main_set_bits(BIT_Spell_Grenade, 1);
            break;
        case 3:
            func_80000860(self, self, 415, 0);
            main_set_bits(BIT_SpellStone_KP, 1);
            main_set_bits(BIT_Spell_Grenade, 1);
            break;
    }
    self->unk0xb0 |= 0x6000;
}
