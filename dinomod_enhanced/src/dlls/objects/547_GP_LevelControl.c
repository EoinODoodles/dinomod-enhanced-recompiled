#include "modding.h"

#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "dll.h"

#include "recomp/dlls/objects/547_GP_LevelControl_recomp.h"

typedef struct {
/*00*/ f32 heatCutsceneTimer;
/*04*/ u8 unk4; // ?
/*05*/ u8 mapID;
} GP_LevelControl_Data;

// Sets down GPbonfire's kindling automatically, allowing game progression to continue (originally by jeebs2kx)
RECOMP_PATCH void GP_LevelControl_setup(Object *self, ObjSetup *setup, s32 arg2) {
    GP_LevelControl_Data *objdata;

    objdata = self->data;
    objdata->unk4 = 0;

    switch (gDLL_29_Gplay->vtbl->get_map_setup(self->mapID)) {
    case 1:
        main_set_bits(BIT_GP_Sharpclaw_Jetbike_Cutscene2, 1);
        break;
    case 3:
        main_set_bits(BIT_GP_Sharpclaw_Jetbike_Cutscene2, 1);
        break;
    }

    main_set_bits(BIT_GP_Bonfire_Kindling_Placed, 1); //@recomp: change initial flag set
    objdata->mapID = 0xFF;
}
