#include "modding.h"
#include "recomputils.h"

#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "dll.h"
#include "dlls/objects/common/sidekick.h"

#include "recomp/dlls/objects/460_CClevcontrol_recomp.h"

typedef struct {
/*00*/ u8 keyUsed;
/*01*/ u8 canCheckKeyUsed;
} CClevcontrol_Data;

RECOMP_PATCH void CClevcontrol_setup(Object *self, ObjSetup *objsetup, s32 arg2) {
    CClevcontrol_Data *objdata = self->data;

    if (!main_get_bits(BIT_CC_Rescued_Kyte)) {
        main_set_bits(BIT_Kyte_Flight_Curve, 2);
    } else {
        objdata->keyUsed = TRUE;
    }

    if (main_get_bits(BIT_CC_Kyte_Pulled_All_Four_Levers) &&
       !main_get_bits(BIT_CC_Gate_Opened)) {
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, 5, 1);
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, 14, 1);
    }

    //@recomp: restore ForceField Spell if it was lost due to dying/reset
    if ((main_get_bits(BIT_Played_Seq_022F_CC_Lightfoot_Gives_Spellpage) == TRUE) &&
        (main_get_bits(BIT_Spell_Forcefield) == FALSE)
    ) {
        main_set_bits(BIT_Spell_Forcefield, TRUE);
    }

    self->stateFlags |= OBJSTATE_UPDATE_DISABLED;
}
