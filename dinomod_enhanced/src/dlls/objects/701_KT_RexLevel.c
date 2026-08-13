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
/* RECOMP */
/*04*/ u8 bossAlreadyFinished;
/*05*/ u8 deleteObjectsTicks;
} KT_RexLevel_Data;

/* Check if the fight was already finished */
RECOMP_PATCH void KT_RexLevel_obj_Setup(Object* self, ObjSetup* setup, s32 reset) {
    KT_RexLevel_Data* objdata = self->data;

    //@recomp: check if the boss fight was already completed
    objdata->bossAlreadyFinished = mainGetBits(BIT_SpellStone_WC);
    if (objdata->bossAlreadyFinished) {
        return;
    }

    envfxAction(self, self, 0x18E, 0);
    envfxAction(self, self, 0x18F, 0);
    lfxAction(self, self, 0x1FD, 0, 0, 0);
    lfxAction(self, self, 0x1FE, 0, 0, 0);
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

/* Unloads any objects used by the boss battle, for use when revisiting the empty room afterwards. 
 * TODO: maybe objectGroups could be a tidier way of doing this? It'd require a lot of edits to the map's objects though.
*/
static void KT_RexLevel_unloadBossObjects(Object* self) {
    Object** objects;
    s32 count;
    s32 i;
    Object* obj;

    for (objects = objGetObjects(&i, &count); i < count; i++) {
        obj = objects[i];
        if (obj == NULL || obj->mapID != MAP_BOSS_KLANADACK) {
            continue;
        }

        switch (obj->id) {
        case OBJ_KT_Rex:
        case OBJ_KT_Fallingrocks:
        case OBJ_KT_RexSequences:
        case OBJ_KT_RexFloorSwit:
        case OBJ_KT_Lazerwall:
        case OBJ_KT_Lazerlight:
            objFreeObject(obj);
            break;
        }
    }
}

/* When revisiting, unload all the boss objects */
RECOMP_PATCH void KT_RexLevel_obj_Control(Object* self) {
    s32 ktFightProgress;
    /* RECOMP */
    KT_RexLevel_Data* objData = self->data;

    //@recomp: check if the battle's already complete
    if (objData->bossAlreadyFinished) {
        KT_RexLevel_unloadBossObjects(self);

        //Run for a few ticks, then stop
        objData->deleteObjectsTicks++;
        if (objData->deleteObjectsTicks >= 3) {
            self->stateFlags |= OBJSTATE_CONTROL_DISABLED;
        }
    }

    if (self->unkDC == 0) {
        mainSetBits(BIT_55E, 1);
        self->unkDC = 1;
    }

    ktFightProgress = mainGetBits(BIT_572_KT_FightProgress);
    if (sPrevFightProgress != (ktFightProgress ^ 0)) {
        if (ktFightProgress & 1) {
            // KTrex is doing a full charge around the arena
            mainSetBits(BIT_54A, 0);
            mainSetBits(BIT_54E, 0);
            mainSetBits(BIT_552, 0);
            mainSetBits(BIT_556, 0);
        } else {
            // KTrex is back to his normal state, raise floor switches again
            mainSetBits(BIT_55C, 1);
            dll_amSfx->Play(self, SOUND_699_KT_RaisingFloorSwitches, MAX_VOLUME, NULL, NULL, 0, NULL);
        }
    }

    if (mainGetBits(BIT_55C)) {
        if (mainGetBits(BIT_55A)) {
            mainSetBits(BIT_54A, 2);
            mainSetBits(BIT_54E, 2);
            mainSetBits(BIT_552, 1);
            mainSetBits(BIT_556, 1);
        } else if (mainGetBits(BIT_55B)) {
            mainSetBits(BIT_54A, 1);
            mainSetBits(BIT_54E, 1);
            mainSetBits(BIT_552, 2);
            mainSetBits(BIT_556, 2);
        }
        mainSetBits(BIT_55C, 0);
    }

    sPrevFightProgress = ktFightProgress;
}

/* When revisiting the room after the boss battle, don't change music when the LevelControl unloads */
RECOMP_PATCH void KT_RexLevel_obj_Free(Object* self, s32 onlySelf) {
    KT_RexLevel_Data* objData = self->data;

    if (objData->bossAlreadyFinished == FALSE) {
        gDLL_5_AMSEQ2->vtbl->set(self, 0xD6, 0, 0, 0);
        gDLL_5_AMSEQ2->vtbl->free(self, 0xD5, 0, 0, 0);
    }
}

/* Extend data */
RECOMP_PATCH u32 KT_RexLevel_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(KT_RexLevel_Data);
}
