#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "game/objects/object.h"
#include "sys/camera.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "dlls/objects/793_BWLog.h"

#include "recomp/dlls/objects/419_DFdockpoint_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 yaw;
/*19*/ s8 spawnLogDisabled;
} DFdockpoint_Setup;

#define LOG_LOAD_DISTANCE 50*8
#define LOG_UNLOAD_DISTANCE (LOG_LOAD_DISTANCE + 40)

// Fix a bug where the dockpoint could create a rapidly loading/unloading log
RECOMP_PATCH void DFdockpoint_control(Object *self) {
    DFdockpoint_Setup *setup;
    BWLog_Setup *logsetup;
    s32 logCount;
    /* RECOMP */
    Object* player;

    setup = (DFdockpoint_Setup*)self->setup;

    //Do nothing if the dockpoint doesn't create a log
    if (setup->spawnLogDisabled) {
        return;
    }

    //Create a log if none exists
    /* @bug: doesn't first check if player's outside the log's unload distance, 
       so a log will be rapidly created/deleted until the player comes into the log's load range */
    obj_get_all_of_type(OBJTYPE_11, &logCount);
    if (logCount != 0) {
        return;
    }

    //@recomp: don't create a log if it'll immediately be unloaded
    if (!(player = get_player())) {
        return;
    }
    if (vec3_distance_squared(&self->globalPosition, &player->srt.transl) >= SQ(LOG_UNLOAD_DISTANCE)) {
        return;
    }

    logsetup = obj_alloc_setup(sizeof(BWLog_Setup), OBJ_BWLog);
    logsetup->base.quarterSize = sizeof(BWLog_Setup)/4;
    logsetup->base.loadFlags = OBJSETUP_LOAD_MAIN;
    logsetup->base.loadDistance = LOG_LOAD_DISTANCE/8;
    logsetup->base.fadeFlags = OBJSETUP_FADE_MAIN;
    logsetup->base.fadeDistance = 45;
    logsetup->base.x = self->srt.transl.x;
    logsetup->base.y = self->srt.transl.y;
    logsetup->base.z = self->srt.transl.z;
    logsetup->yaw = setup->yaw;

    obj_create((ObjSetup*)logsetup, 
        OBJINIT_STANDALONE | OBJINIT_FLAG4, 
        self->mapID, 
        -1, 
        self->parent
    );
}
