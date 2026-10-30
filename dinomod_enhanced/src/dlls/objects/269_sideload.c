#include "modding.h"
#include "recomputils.h"

#include "common_objsetups.h"

#include "PR/ultratypes.h"
#include "game/objects/object_id.h"
#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "dll.h"

#include "recomp/dlls/objects/269_sideload_recomp.h"

//TEMPORARY DEFINES
#define sideload_obj_Control sideload_control
//END OF TEMPORARY DEFINES

typedef struct {
    u8 loaded;              //Boolean: whether the sidekick has been loaded by this SideLoad object
} SideLoad_Data;

typedef enum {
    Sidekick_Index_Tricky,
    Sidekick_Index_Kyte
} SideLoad_Indices;

RECOMP_PATCH void sideload_obj_Control(Object* self) {
    SideLoad_Data* objData = self->data;
    SideLoad_Setup_Dinomod* objSetup = (SideLoad_Setup_Dinomod*)self->setup;
    s16 dSidekickObjIDs[] = {OBJ_Tricky, OBJ_Kyte};
    s16 gamebit;
    Object* player;
    Object* sidekick;
    
    //@recomp: store player and sidekick for additional checks
    player = objGetPlayer();
    if (player == NULL) {
        return;
    }
    sidekick = objGetSidekick();

    /* @recomp: add a special case for SwapStone Hollow, 
       stopping Tricky from loading and falling into the well via the surface's SideLoad objects.

       TODO: maybe rework this as general-purpose new options/features for the sideload?
       Could have an optional player minY and/or maxY, and an optional player mapID filter! 
       This would require extending the objSetup (maybe with a new controlNo too), so it might
       be easiest to implement as a new Object entirely ("SideLoadPlus"?) 
       */
    if (self->mapID == MAP_SWAPSTONE_HOLLOW) {
        #define SWAPSTONE_WELL_Y -845.0f

        //Don't load the sidekick if the player's yCoords are at well height
        if ((self->globalPosition.y >= SWAPSTONE_WELL_Y) && (player->srt.transl.y < SWAPSTONE_WELL_Y)) {
            return;
        }

        //Don't load the sidekick if the player's mapID is in the well
        if (mapWorldXZToMapID(player->globalPosition.x, player->globalPosition.z) == MAP_SWAPSTONE_HOLLOW_2) {
            return;
        }
    }

    //Return if the sidekick is currently loaded
    if (sidekick) {
        return;
    }
    
    //Do nothing more if the object loaded the sidekick previously
    if (objData->loaded && 
        (objSetup->options & SideLoad_OPTION_Repeatable) == FALSE //@recomp: add a way to continue loading the sidekick
    ) {
        return;
    }

    //Load the sidekick if they're unlocked (or no unlock gamebit is specified)
    gamebit = objSetup->gamebitUnlocked;
    if ((gamebit == NO_GAMEBIT) || mainGetBits(gamebit)) {
        objData->loaded = TRUE;
        objLoadSidekick(self, dSidekickObjIDs[objSetup->sidekickIndex]);
    }
}
