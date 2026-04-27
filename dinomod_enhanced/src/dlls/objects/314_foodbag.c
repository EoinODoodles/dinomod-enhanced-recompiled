#include "macros.h"
#include "modding.h"
#include "recomputils.h"
#include "dll_util.h"

#include "PR/ultratypes.h"
#include "game/gamebits.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "sys/dll.h"
#include "dll.h"
#include "dlls/engine/29_gplay.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/common/foodbag.h"

#include "engine/56_putdown.h"
#include "objects/314_foodbag.h"

#include "recomp/dlls/objects/314_foodbag_recomp.h"

#define NO_FOOD_TYPE -1

extern FoodbagItem foodbag_items[];

extern s16 food_anim_gamebitIDs[11];
extern s16 food_anim_objectIDs[10];

/** Checks whether a particular food item will be stored in the player's food bag */
int Foodbag_will_collected_food_be_stored(Object* foodbag, s32 foodType) {
    Foodbag_Data* objData = foodbag->data;

    if (!foodbag || foodType == NO_FOOD_TYPE) {
        return FALSE;
    }
    
    //If the player is using the "Eat First" option and their health isn't full, the food will be eaten immediately
    if ((objData->eatFirst && (*objData->playerHealth < *objData->playerHealthMax)) || 
        (objData->capacity == 0)
    ) {
        return FALSE;
    }
    
    //Otherwise, check whether the bag has room to store the food
    if (putdown_can_food_be_added(foodType, objData->capacity, objData->bagSlots, foodbag_items)) {
        return TRUE;
    }

    return FALSE;
}

/** Returns the FoodType associated with a particular food Object */
s16 Foodbag_get_food_type_from_objectID(Object* foodObj) {
    u32 i;

    if (!foodObj) {
        return NO_FOOD_TYPE;
    }

    switch (foodObj->id) {
    case OBJ_applePickup:
        //Only Red Apples are obtainable currently (Apple pickups only seem to have one modelIdx)
        switch (foodObj->modelInstIdx) {
        default:
        case 0:
            return FOOD_Red_Apple;
        // case 1:
        //     return FOOD_Green_Apple;
        // case 2:
        //     return FOOD_Brown_Apple;
        }
        break;
    // case OBJ_fish:
    //     //Fish don't have a regular collectable Object currently (only obtained with net)
    //     break;
    case OBJ_meatPickup: //Energy Eggs
        return FOOD_Dino_Egg;
    case OBJ_beanPickup:
        //Only Red Beans are obtainable currently (Bean pickups only seem to have one modelIdx)
        switch (foodObj->modelInstIdx) {
        default:
        case 0:
            return FOOD_Red_Bean;
        // case 1:
        //     return FOOD_Brown_Bean;
        // case 2:
        //     return FOOD_Blue_Bean;
        }
        break;
    }

    return NO_FOOD_TYPE;
}
