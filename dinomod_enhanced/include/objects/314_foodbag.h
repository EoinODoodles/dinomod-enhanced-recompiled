#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

int Foodbag_will_collected_food_be_stored(Object* foodbag, s32 foodType);
s16 Foodbag_get_food_type_from_objectID(Object* foodObj);
