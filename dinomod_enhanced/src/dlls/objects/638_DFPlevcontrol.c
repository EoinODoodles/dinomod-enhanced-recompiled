#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/dll.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/segment_1460.h"

#include "dlls/objects/638_DFPlevcontrol.h"

#include "recomp/dlls/objects/638_DFPlevcontrol_recomp.h"

extern int DFP_LevelControl_anim_callback(Object* self, Object *arg1, AnimObj_Data *arg2, s8 arg3);

extern Texture* dTexElectricity;

/** Removes progress-breaking debug code that automatically granted the player CloudRunner Fortress' SpellStone (originally by MusicalProgrammer) */
RECOMP_PATCH void DFP_LevelControl_setup(Object* self, ObjSetup *setup, s32 arg2) {
    u8 mapSetup;

    obj_add_object_type(self, OBJTYPE_10);
    dTexElectricity = tex_load_deferred(1132);
    self->animCallback = DFP_LevelControl_anim_callback;
    gDLL_29_Gplay->vtbl->set_map_setup(self->mapID, 1);
    mapSetup = gDLL_29_Gplay->vtbl->get_map_setup(self->mapID);

    switch (mapSetup) { 
        case 0:
            func_80000860(self, self, 261, 0);
            func_80000860(self, self, 262, 0);
            func_80000860(self, self, 263, 0);
            gDLL_29_Gplay->vtbl->set_obj_group_status(
                MAP_DESERT_FORCE_POINT_TEMPLE_TOP, DFPT_ObjGroup2_Bottom_BigDoor, 1);
            break;
        case 1:
            func_80000860(self, self, 415, 0);
            // main_set_bits(BIT_SpellStone_CRF, 1); //@recomp: don't set gamebit
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
    self->stateFlags |= OBJSTATE_PRINT_DISABLED | OBJSTATE_UPDATE_DISABLED;
}
