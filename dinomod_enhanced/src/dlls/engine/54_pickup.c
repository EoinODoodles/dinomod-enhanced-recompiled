#include "modding.h"

#include "PR/ultratypes.h"
#include "dlls/engine/54_pickup.h"
#include "dlls/objects/210_player.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object.h"
#include "sys/joypad.h"

#include "recomp/dlls/engine/54_pickup_recomp.h"

RECOMP_PATCH s32 pickup_should_pickup(Object* obj, Object* player, Pickup* pickup) {
    s32 shouldPickup = FALSE;

    if (((obj->unk78[obj->unkD4].flags & 0xF) == 6) 
            && (obj->unkAF & ARROW_FLAG_1_Interacted) 
            && (obj->unkE0 == 0)) {
        // @recomp: Don't allow pickups while in a windlift
        Player_Data* playerData = player->data;
        if (playerData->unk808 == 0.0f) {
            pickup->unk0 = 0;
            joyDisableButtons(0, A_BUTTON);
            shouldPickup = TRUE;
        }
    }

    return shouldPickup;
}
