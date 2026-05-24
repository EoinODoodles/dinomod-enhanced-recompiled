#include "recomputils.h"
#include "player_util.h"

#include "game/objects/object_id.h"
#include "sys/objects.h"
#include "dll.h"
#include "dlls/engine/18_objfsa.h"
#include "dlls/engine/54_pickup.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/objects/210_player_recomp.h"

extern s16 _data_98[];
extern f32 _data_6F8;

/** Switches the player back to their basic walking animations (weapon stowed) */
void playerUtil_use_walk_anims(Object* player){
    Player_Data* objData;
    if (!player){
        return;
    }

    objData = player->data;
    if (!objData){
        return;
    }

    objData->unk3C4 = &_data_6F8;
    objData->modAnims = _data_98;
}

/** Forces the player out of carrying an Object, and into their default standing state */
void playerUtil_stop_carrying(Object* player){
    Player_Data* objData;
    if (!player){
        return;
    }

    objData = player->data;
    if (!objData){
        return;
    }

    if (objData->unk868 != NULL) {
        gDLL_54_pickup->vtbl->drop(objData->unk868, objData->unk868->data);
        playerUtil_use_walk_anims(player);
    }
}

/**
  * Returns TRUE if the player is currently walking/standing/turning, or FALSE if they're in some other animState
  */
int playerUtil_is_player_standing_or_walking(Object* player){
    Player_Data* objData;
    ObjFSA_Data* fsa;
    if (!player){
        return FALSE;
    }

    objData = player->data;
    if (!objData){
        return FALSE;
    }

    fsa = &objData->unk0;

    return (fsa->animState == PLAYER_ASTATE_Standing || 
            fsa->animState == PLAYER_ASTATE_Turning_On_Spot || 
            fsa->animState == PLAYER_ASTATE_Walking);
}

/**
  * Clear references to a specific collected Object.
  */
void playerUtil_clear_collected_object(Object* player, Object* collected) {
    Player_Data* objData;

    if (player == NULL || collected == NULL) {
        return;
    }

    objData = player->data;
    if (objData == NULL) {
        return;
    }

    //Clear references to a specific collected Object
    if (objData->unk708 == collected) {
        objData->unk708 = NULL;
        objData->unk8A9 = FALSE;
    }
}
