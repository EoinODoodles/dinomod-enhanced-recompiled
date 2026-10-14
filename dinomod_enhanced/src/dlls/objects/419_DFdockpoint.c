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

#include "objects/419_DFdockpoint.h"
#include "objects/793_BWLog.h"

#include "recomp/dlls/objects/419_DFdockpoint_recomp.h"

#define LOG_LOAD_DISTANCE 50*8
#define LOG_UNLOAD_DISTANCE (LOG_LOAD_DISTANCE + 40)

// Fix a bug where the dockpoint could create a rapidly loading/unloading log
RECOMP_PATCH void DFdockpoint_control(Object* self) {
    DFdockpoint_Setup* setup;
    BWlog_Setup* logsetup;
    s32 logCount;
    /* RECOMP */
    Object* player;
    Object* log;

    setup = (DFdockpoint_Setup*)self->setup;

    //Do nothing if the dockpoint doesn't create a log
    if (setup->spawnLogDisabled) {
        return;
    }

    //Create a log if none exists
    /* @bug: doesn't first check if player's outside the log's unload distance, 
       so a log will be rapidly created/deleted until the player comes into the log's load range */
    objGetAllOfType(OBJTYPE_Vehicle, &logCount);
    if (logCount != 0) {
        return;
    }

    //@recomp: don't create a log if it'll immediately be unloaded
    if (!(player = objGetPlayer())) {
        return;
    }
    if (vec3DistanceSquared(&self->globalPosition, &player->srt.transl) >= SQ(LOG_UNLOAD_DISTANCE)) {
        return;
    }

    logsetup = objAllocSetup(sizeof(BWlog_Setup), OBJ_BWLog);
    logsetup->setup.quarterSize = sizeof(BWlog_Setup)/4;
    logsetup->setup.loadFlags = OBJSETUP_LOAD_MAIN;
    logsetup->setup.loadDistance = LOG_LOAD_DISTANCE/8;
    logsetup->setup.fadeFlags = OBJSETUP_FADE_MAIN;
    logsetup->setup.fadeDistance = 45;
    logsetup->setup.x = self->srt.transl.x;
    logsetup->setup.y = self->srt.transl.y;
    logsetup->setup.z = self->srt.transl.z;
    logsetup->startRotation = setup->yaw;

    //@recomp: store the dockpoint's options on the log's Setup struct, for reference
    logsetup->dockpointOptions = setup->options;

    //@recomp: store the log's pointer to a var
    log = objSetupObject((ObjSetup*)logsetup, 
        OBJINIT_STANDALONE | OBJINIT_FLAG4, 
        //@recomp: add an option for the log not to inherit the dockpoint's mapID
        (setup->options & DFdockpoint_OPTION_1_Create_Log_Without_MapID) ? -1 : self->mapID, 
        -1, 
        self->parent
    );

    //@recomp: fade in the log
    if (log) {
        log->opacity = 1;
    }
}
