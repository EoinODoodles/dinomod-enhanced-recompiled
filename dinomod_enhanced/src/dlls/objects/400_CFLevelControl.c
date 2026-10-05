#include "dlls/engine/29_gplay.h"
#include "modding.h"
#include "recomputils.h"

#include "common_objsetups.h"

#include "sys/main.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "sys/objprint.h"
#include "game/gamebits.h"
#include "sys/dll.h"
#include "sys/objtype.h"
#include "sys/gfx/projgfx.h"
#include "dlls/engine/53_movelib.h"
#include "dlls/objects/373_CFCloudBaby.h"
#include "dlls/objects/common/cf_can_unload.h"
#include "macros.h"
#include "dll.h"

#include "recomp/dlls/objects/400_CFLevelControl_recomp.h"

#define UID_CFGuardian_Dungeon 0x2A4F

#define CREATE_POINT_ID_Guardian_Courtyard1 14
// custom create points:
#define CREATE_POINT_ID_Guardian_Cell 30
#define CREATE_POINT_ID_Guardian_BeforeWindLift 31
#define CREATE_POINT_ID_Guardian_WindLift 32

enum CFGuardianState {
    CFGUARDIAN_STATE_InCell = 0,
    CFGUARDIAN_STATE_WaitingToBeFreed = 1,
    CFGUARDIAN_STATE_LeavingCell = 2, // walking through prison
    CFGUARDIAN_STATE_WaitingAtWindLift = 3,
    CFGUARDIAN_STATE_WaitingForWindLiftPower = 4,
    CFGUARDIAN_STATE_5 = 5, // unused
    CFGUARDIAN_STATE_InWindLift = 6,
    CFGUARDIAN_STATE_LeavingWindLift = 7, // leaving top of wind lift
    CFGUARDIAN_STATE_Courtyard_WaitingForKyte = 8, // waiting for kyte to be freed
    CFGUARDIAN_STATE_Courtyard_WaitingForCloudBaby = 9, // waiting for cloud baby rescue
    CFGUARDIAN_STATE_WalkingToRaceArea = 10, // walk to drained water
    CFGUARDIAN_STATE_Vanish = 11, // vanish (after reaching drained water)
    CFGUARDIAN_STATE_WaitingForRaceCompletion = 12,
    CFGUARDIAN_STATE_WaitingForSpellStone = 13,
    CFGUARDIAN_STATE_NoOp_SpellStoneActivated = 14,
    CFGUARDIAN_STATE_NoOp_Vanished = 15
};

// movelib_func_1C0C but without the arbitrary ID range check. dinomod uses create point IDs > 27
static s32 recomp_findCustomCreatePoint(s32 id, SRT* transform) {
    s32 uid = gDLL_26_Curves->vtbl->func_218C(id);
    if (uid >= 0) {
        CurveSetup* curve = gDLL_26_Curves->vtbl->func_39C(uid);
        transform->transl.x = curve->pos.x;
        transform->transl.y = curve->pos.y;
        transform->transl.z = curve->pos.z;
        transform->yaw = curve->unk2C << 8;
        return 1;
    }

    return 0;
}

static void recomp_moveGuardian(CFGuardian_Setup* setup, s32 map, s32 uid, s32 createPointID) {
    SRT dst;
    if (!recomp_findCustomCreatePoint(createPointID, &dst)) {
        recomp_eprintf("Failed to find CFGuardian Dungeon create point %d!\n", createPointID);
        return;
    }

    s8 yaw8 = dst.yaw >> 8;
    if (setup->base.x != dst.transl.x || setup->base.y != dst.transl.y || setup->base.z != dst.transl.z 
            || setup->yaw8 != yaw8) {
        setup->base.x = dst.transl.x;
        setup->base.y = dst.transl.y;
        setup->base.z = dst.transl.z;
        setup->yaw8 = yaw8;

        mapSaveObject(&setup->base, map, dst.transl.x, dst.transl.y, dst.transl.z);

        // HACK: if CFGuardian already spawned, don't let his control/update funcs run until he's reloaded.
        //       this prevents the DLL from having any side effects while in the incorrect location.
        Object* obj = objGetObjectByUID(uid);
        if (obj != NULL) {
            objDisable(obj);
        }

        // HACK: reload CFGuardian if they were already loaded (CFLevelControl will re-enable his objgroup as needed)
        dll_gplay->set_obj_group_status(MAP_CLOUDRUNNER_DUNGEON, 3, 0);

        recomp_printf("Moved CFGuardian 0x%X to %f,%f,%f (create point %d)\n", 
            uid, dst.transl.x, dst.transl.y, dst.transl.z, createPointID);
    } else {
        recomp_printf("CFGuardian already in the correct spot\n");
    }
}

/** 
  * Because CFGuardian saves their position to the savefile, and that positions saved that way are *always* persisted
  * when the player saves the game, the position of their objsetup may not match where they should be given the current
  * state of the game if the player were to return to a save point or restart point. This hook checks where they should
  * actually be when the level first loads and moves them if necessary.
 */
RECOMP_HOOK_RETURN_DLL(CFLevelControl_obj_Setup) void recomp_CFLevelControl_resetGuardianPos(void) {
    SRT transform;

    // IMPORTANT: this patch relies on the fact that the dungeon map is always loaded with the rest of CF!
    CFGuardian_Setup* setup = (CFGuardian_Setup*)mapFindObjSetup(UID_CFGuardian_Dungeon, NULL, NULL, NULL, NULL);
    if (setup == NULL) {
        recomp_eprintf("Failed to find CFGuardian Dungeon setup!\n");
        return;
    }

    s32 state = mainGetBits(BIT_CFGuardian_State);
    switch (state) {
    case CFGUARDIAN_STATE_InCell:
    case CFGUARDIAN_STATE_WaitingToBeFreed:
    case CFGUARDIAN_STATE_LeavingCell:
        // Should start in the cell
        recomp_moveGuardian(setup, MAP_CLOUDRUNNER_DUNGEON, UID_CFGuardian_Dungeon, CREATE_POINT_ID_Guardian_Cell);
        break;
    case CFGUARDIAN_STATE_WaitingAtWindLift:
        // Should be right before the wind lift
        recomp_moveGuardian(setup, MAP_CLOUDRUNNER_DUNGEON, UID_CFGuardian_Dungeon, CREATE_POINT_ID_Guardian_BeforeWindLift);
        break;
    case CFGUARDIAN_STATE_WaitingForWindLiftPower:
        // If the WindLifts are already powered, we need to start the guardian *in* in the windlift since
        // the cutscene that normally moves him into it won't play.
        if (mainGetBits(BIT_CRF_WindLifts_Powered) != 0) {
            recomp_moveGuardian(setup, MAP_CLOUDRUNNER_DUNGEON, UID_CFGuardian_Dungeon, CREATE_POINT_ID_Guardian_WindLift);
        } else {
            recomp_moveGuardian(setup, MAP_CLOUDRUNNER_DUNGEON, UID_CFGuardian_Dungeon, CREATE_POINT_ID_Guardian_BeforeWindLift);
        }
        break;
    case CFGUARDIAN_STATE_InWindLift:
        // Should be in the wind lift
        recomp_moveGuardian(setup, MAP_CLOUDRUNNER_DUNGEON, UID_CFGuardian_Dungeon, CREATE_POINT_ID_Guardian_WindLift);
        break;
    case CFGUARDIAN_STATE_LeavingWindLift:
    case CFGUARDIAN_STATE_Courtyard_WaitingForKyte:
    case CFGUARDIAN_STATE_Courtyard_WaitingForCloudBaby:
        // Should be in the courtyard, next to the pillar
        recomp_moveGuardian(setup, MAP_CLOUDRUNNER_DUNGEON, UID_CFGuardian_Dungeon, CREATE_POINT_ID_Guardian_Courtyard1);
        break;
    }
}

RECOMP_HOOK_RETURN_DLL(CFLevelControl_obj_Control) void recomp_CFLevelControl_guardianSeqObjGroupToggle(void) {
    // Enable CFGuardian objgroup while the water draining seq plays, otherwise he will not show up.
    // This must run after CFLevelControl_doDistBasedObjGroupToggling to work correctly.
    if (mainGetBits(BIT_CRF_Throne_Room_Quest_Complete) != 0 && mainGetBits(BIT_Played_Seq_02B2_CF_Courtyard_Water_Drains) == 0) {
        dll_gplay->set_obj_group_status(MAP_CLOUDRUNNER_DUNGEON, 3, 1);
    }
}
