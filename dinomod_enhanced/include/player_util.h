#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

void playerUtil_use_walk_anims(Object* player);
void playerUtil_stop_carrying(Object* player);
int playerUtil_is_player_standing_or_walking(Object* player); 
void playerUtil_clear_collected_object(Object* player, Object* collected); 
