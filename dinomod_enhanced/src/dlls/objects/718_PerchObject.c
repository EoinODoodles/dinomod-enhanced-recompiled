#include "modding.h"

#include "common.h"
#include "PR/ultratypes.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/objects/718_PerchObject_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 interactionDistance;
/*19*/ u8 unk19;
/*1A*/ u16 kyteFlightGroup; //curve group Kyte should traverse when using "Find" command
/*1C*/ s16 unk1C;
/*1E*/ u8 useDistance3D; //affects how player-to-perch distance is calculated (2D X/Z vs. full 3D)
} PerchObject_Setup;

typedef struct {
    s8 stateIndex;
    CurveSetup* curveSetup;
} PerchObject_Data;

/** Fixes a crash in Cape Claw's perches for Kyte (originally by MusicalProgrammer) */
RECOMP_PATCH int perchObject_anim_callback(Object* self, Object* animObj, AnimObj_Data* animObjData, s8 arg3) {
    Object* kyte;
    Object* player;
    PerchObject_Data* objData;
    PerchObject_Setup* objSetup;

    objSetup = (PerchObject_Setup*)self->setup;

    //@recomp: bail if the PerchObject's curveSetup is missing
    objData = self->data;
    if (!objData->curveSetup){
        return 0;
    }

    //@recomp: bail if Kyte is missing
    kyte = get_sidekick();
    if (!kyte){
        return 0;
    }

    //@recomp: bail if Krystal is missing
    player = get_player();
    if (!player){
        return 0;
    }

    if (vec3_distance_squared(&get_player()->globalPosition, (Vec3f*)&(objData->curveSetup)->pos.x) <= (objSetup->interactionDistance * objSetup->interactionDistance)) {
        ((DLL_Unknown*)kyte->dll)->vtbl->func[14].withTwoArgs((s32)kyte, 1);
        if (gDLL_1_cmdmenu->vtbl->was_this_item_used(Sidekick_Command_INDEX_1_Find)) {
            main_set_bits(BIT_Kyte_Flight_Curve, objSetup->kyteFlightGroup);
        }
    }
    return 0;
}
