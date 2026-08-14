#include "dll.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "modding.h"
#include "recompconfig.h"
#include "sys/envfx.h"
#include "sys/lfx.h"
#include "sys/main.h"

#include "recomp/dlls/objects/701_KT_RexLevel_recomp.h"
#include "sys/map_enums.h"
#include "sys/objects.h"

//TEMPORARY DEFINES
#define KT_RexLevel_obj_Setup KT_RexLevel_setup
#define KT_RexLevel_obj_Control KT_RexLevel_control
#define KT_RexLevel_obj_Free KT_RexLevel_free
#define KT_RexLevel_obj_GetDataSize KT_RexLevel_get_data_size
#define sPrevFightProgress _bss_0
//END OF TEMPORARY DEFINES

/*0x0*/ extern s32 sPrevFightProgress;

typedef struct {
/*00*/ f32 unk0;
} KT_RexLevel_Data;

/* Check if the fight was already finished */
RECOMP_PATCH void KT_RexLevel_obj_Setup(Object* self, ObjSetup* setup, s32 reset) {
    KT_RexLevel_Data* objdata = self->data;

    envfxAction(self, self, 0x18E, 0);
    envfxAction(self, self, 0x18F, 0);
    lfxAction(self, self, 0x1FD, 0, 0, 0);
    lfxAction(self, self, 0x1FE, 0, 0, 0);

    //@recomp: check if the boss fight was already completed
    if (mainGetBits(BIT_SpellStone_WC)) {
        //Change the map to Act 2 (hiding boss battle objects)
        gDLL_29_Gplay->vtbl->set_act(MAP_BOSS_KLANADACK, 2);
        return;
    } else {
        //Otherwise, change the map to Act 1 for the boss fight
        gDLL_29_Gplay->vtbl->set_act(MAP_BOSS_KLANADACK, 1);
    }

    gDLL_5_AMSEQ2->vtbl->set(self, 0xD5, 0, 0, 0);

    mainSetBits(BIT_572_KT_FightProgress, 0);
    mainSetBits(BIT_56E, 1);
    mainSetBits(BIT_KT_Player_In_Segment_2, 1);
    mainSetBits(BIT_KT_Player_In_Segment_1, 1);
    objdata->unk0 = 600.0f;

    mainSetBits(BIT_55A, 1);
    mainSetBits(BIT_54A, 2);
    mainSetBits(BIT_54E, 2);
    mainSetBits(BIT_552, 1);
    mainSetBits(BIT_556, 1);

    self->unkDC = 0;
}
