#include "PR/ultratypes.h"
#include "game/objects/object_id.h"
#include "modding.h"

#include "game/objects/object.h"
#include "game/gamebits.h"
#include "recomputils.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "dll.h"

#include "recomp/dlls/_asm/269_recomp.h"

#define sideload_control dll_269_control //TODO: remove once decomp updated
typedef struct {
    ObjSetup base;
    s16 gamebitUnlocked;    //GamebitID determining whether the sidekick is unlocked (i.e. Kyte rescued)
    u8 sidekickIndex;       //Whether this should load Tricky/Kyte (see `SideLoad_Indices`)
} SideLoad_Setup;

typedef struct {
    u8 loaded;              //Boolean: whether the sidekick has been loaded by the SideLoad object
} SideLoad_Data;

typedef enum {
    Sidekick_Index_Tricky,
    Sidekick_Index_Kyte
} SideLoad_Indices;

RECOMP_PATCH void sideload_control(Object* self) {
    SideLoad_Data* objData = self->data;
    SideLoad_Setup* objSetup = (SideLoad_Setup*)self->setup;
    s16 dSidekickObjIDs[] = {OBJ_Tricky, OBJ_Kyte};
    s16 gamebit;
    Object* player;
    Object* sidekick;
    
    //@recomp: store player and sidekick for additional checks
    if ((player = get_player()) == NULL) {
        return;
    }
    sidekick = get_sidekick();

    //@recomp: add a special case for SwapStone Hollow, 
    //stopping Tricky from loading and falling into the well via the surface's SideLoad object
    if (self->mapID == MAP_SWAPSTONE_HOLLOW) {
        if ((player->srt.transl.y < -845.0f) && (main_get_bits(BIT_Tricky_Learned_Flame) == FALSE)) {
            if (sidekick) {
                obj_destroy_object(sidekick);
            }
            return;
        }
    }

    if ((sidekick == FALSE) && (objData->loaded == FALSE) && 
        ((gamebit = objSetup->gamebitUnlocked, gamebit == NO_GAMEBIT) || main_get_bits(gamebit))
    ) {
        objData->loaded = TRUE;
        func_80023894(self, dSidekickObjIDs[objSetup->sidekickIndex]);
    }
}
