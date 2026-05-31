#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "sys/map_enums.h"

#include "recomp/dlls/objects/547_GP_LevelControl_recomp.h"

typedef struct {
/*00*/ f32 heatCutsceneTimer;
/*04*/ u8 unk4; // ?
/*05*/ u8 mapID;
} GP_LevelControl_Data;

typedef struct {
    s16 play;
    s16 played;
} SequenceGamebits; //TODO: move to sequence-related util file?

//@recomp: for checking the pillars' object group is loaded when relevant bits are set
static SequenceGamebits GP_ShrinePillar_Gamebits[3] = {
    {BIT_GP_ShrinePillar_Rises_1, BIT_GP_ShrinePillar_Raised_1},
    {BIT_GP_ShrinePillar_Rises_2, BIT_GP_ShrinePillar_Raised_2},
    {BIT_GP_ShrinePillar_Rises_3, BIT_GP_ShrinePillar_Raised_3},
};

/** Make sure object group 5 is loaded when needed */
static void ensure_pillars_load(){
    //Check if pillars' object group is already loaded
    if (gDLL_29_Gplay->vtbl->get_obj_group_status(MAP_GOLDEN_PLAINS, 5)){
        return;
    }

    //Load the object group if any of the pillars' gamebits are set
    for (s32 i = 0; i < 3; i++){
        if (main_get_bits(GP_ShrinePillar_Gamebits[i].play) || main_get_bits(GP_ShrinePillar_Gamebits[i].played)){
            gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_GOLDEN_PLAINS, 5, 1);
            return;
        }
    }
}

/** - Sets down GPbonfire's kindling automatically, allowing game progression to continue (originally by jeebs2kx)
  * - Makes sure GP_ShrinePillars' object group is loaded when relevant 
  */
RECOMP_PATCH void GP_LevelControl_setup(Object *self, ObjSetup *setup, s32 arg2) {
    GP_LevelControl_Data *objdata;

    objdata = self->data;
    objdata->unk4 = 0;

    switch (gDLL_29_Gplay->vtbl->get_act(self->mapID)) {
    case 1:
        main_set_bits(BIT_GP_Sharpclaw_Jetbike_Cutscene2, 1);
        break;
    case 3:
        main_set_bits(BIT_GP_Sharpclaw_Jetbike_Cutscene2, 1);
        break;
    }

    main_set_bits(BIT_GP_Bonfire_Kindling_Placed, 1); //@recomp: change initial flag set
    objdata->mapID = 0xFF;

    //@recomp: GP_ShrinePillar checks
    ensure_pillars_load();
}
